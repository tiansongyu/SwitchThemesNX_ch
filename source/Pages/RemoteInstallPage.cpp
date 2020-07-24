#include "RemoteInstallPage.hpp"
#include "../ViewFunctions.hpp"
#ifdef __SWITCH__
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif
#include "../SwitchTools/PayloadReboot.hpp"

using namespace std;

RemoteInstallPage::~RemoteInstallPage()
{
	StopSocketing();
}

RemoteInstallPage::RemoteInstallPage() : 
lblInfo("你可以用主题远程安装工具从你的电脑直接安装一个主题，到  'Theme build' 选项卡，点击  '远程安装…''"),
lblConfirm("按 A 安装，按 B 取消."),
BtnStart("开始远程安装.....")
{
	Name = "远程安装";
}

void RemoteInstallPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font30);
	if (entry)
	{
		entry->Render();
		ImGui::TextWrapped(lblConfirm.c_str());
	}
	else 
	{
		ImGui::TextWrapped(lblInfo.c_str());
		if (ImGui::Button(BtnStart.c_str()))
		{
			if (sock >= 0)
				StopSocketing();
			else
				StartSocketing();
		}
		PAGE_RESET_FOCUS;
		ImGui::TextWrapped("将菜单保持在此页面上不要动，否则将不会执行命令");
		ImGui::Checkbox("自动安装并重新启动", &AutoInstall);
	}
	ImGui::PopFont();
	Utils::ImGuiCloseWin();
}

void RemoteInstallPage::StartSocketing()
{
#if __SWITCH__
	if (sock != -1)
		return;
	
	int err;
	struct sockaddr_in temp;
	
	sock=socket(AF_INET,SOCK_STREAM,0);
	if (sock < 0)
	{
		Dialog("Couldn't start socketing (socket error)");
		sock = -1;
		return;
	}
	temp.sin_family=AF_INET;
	temp.sin_addr.s_addr=INADDR_ANY;
	temp.sin_port=htons(5000);
	
	err=fcntl(sock,F_SETFL,O_NONBLOCK);
	const int optVal = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, sizeof(optVal));
	if (err)
	{
		Dialog("开启远程安装进程失败,请确认switch与电脑是否正确连接(fcntl error)");
		StopSocketing();
		return;
	}
	
	err=bind(sock,(struct sockaddr*) &temp,sizeof(temp));
	if (err)
	{
		Dialog("开启远程安装进程失败,请确认switch与电脑是否正确连接 (bind error)");
		StopSocketing();
		return;
	}
	
	err=listen(sock,1);
	if (err)
	{
		Dialog("开启远程安装进程失败,请确认switch与电脑是否正确连接 (listen error)");
		StopSocketing();
		return;
	}
	
	char hostname[128];
	err = gethostname(hostname, sizeof(hostname));
	if(err != 0)
	{
		Dialog("开启远程安装进程失败,请确认switch与电脑是否正确连接(gethostname error)");
		StopSocketing();
		return;
	}
	
#else
	sock = 66;
	const char* hostname = "F:\remoteFile.bin";
#endif
	BtnStart = ("IP: " + string(hostname) + " - 按下可停止");
}

void RemoteInstallPage::StopSocketing()
{
#if __SWITCH__
	if (curSock != -1)
		close(curSock);
	if (sock != -1)
		close(sock);
#endif
	curSock = -1;
	ThemeSize = 0;
	sock = -1;
	BtnStart = ("开始远程安装....");
}

void RemoteInstallPage::DialogError(const std::string &msg)
{
	Dialog("发生错误，请再试一次\n" + msg);
}

void RemoteInstallPage::SocketUpdate()
{	
	if (sock < 0) 
	{
		return;
	}	
#if __SWITCH__
	int size = -1;
	if (curSock == -1 && (curSock=accept(sock,0,0))!=-1)
	{
		u8 buf[12]; 
		memset(buf,0,sizeof(buf));
		if ((size=recv(curSock,buf,sizeof(buf),0)) < 0)
		{
			DialogError("(Couldn't read any data.)");
			StopSocketing();
			return;
		}
		else
		{			
			if (strncmp((char*)buf, "theme", 5) != 0)
			{
				DialogError("(Unexpected data received.)");
				StopSocketing();
				return;
			}
			memcpy(&ThemeSize, buf + 8, sizeof(u32));
			if (ThemeSize < 50 || ThemeSize > 2000000)
			{
				DialogError("(Invalid size: " + to_string(ThemeSize) + ")");
				StopSocketing();
				return;				
			}
			data.clear();
			data.reserve(ThemeSize);
		}		
	}
	if (ThemeSize && curSock != -1)
	{
		DisplayLoading("Loading...");
		u8 tmp[10];
		while ((size = recv(curSock,tmp,10,0)) > 0)
		{
			for (int i = 0; i < size; i++)
				data.push_back(tmp[i]);
		}
		if (data.size() == ThemeSize || size == 0 || (size == -1 && errno != EWOULDBLOCK)){
			if (data.size() != ThemeSize)
				DialogError("(Unexpected data count: " + to_string(size) + ")");
			else
			{
				write(curSock,"ok",2);
				entry = ThemeEntry::FromSARC(data);
				StopSocketing();
			}
		}
		return;
	}
#else
	if (filesystem::exists("F:/RemoteFile.bin"))
	{
		data = fs::OpenFile("F:/RemoteFile.bin");
		ThemeSize = data.size();
		entry = ThemeEntry::FromSARC(data);
		StopSocketing();
	}
#endif
}

void RemoteInstallPage::Update()
{
	if (entry)
	{
		if (KeyPressed(GLFW_GAMEPAD_BUTTON_A) || AutoInstall)
		{
			entry->Install(!AutoInstall);
			entry = nullptr;
			StopSocketing();
			
			if (AutoInstall)
			{
				if (PayloadReboot::Init())
					PayloadReboot::Reboot();
				else
					Dialog("初始化注入程序失败 !");
			}
			
			return;
		}
		else if (KeyPressed(GLFW_GAMEPAD_BUTTON_B))
		{
			entry = nullptr;			
			StopSocketing();
			return;			
		}
	}
	
	if (Utils::PageLeaveFocusInput()){
		Parent->PageLeaveFocus(this);
		return;
	}
	
	if (entry) return;
	
	if (sock >= 0)
		SocketUpdate();
}