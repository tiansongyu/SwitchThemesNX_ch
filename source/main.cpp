#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <stdio.h>
#include <filesystem>
#include <stack>
#include <variant>

#include "UI/UIManagement.hpp"
#include "UI/UI.hpp"
#include "Platform/Platform.hpp"

#include "Pages/ThemePage.hpp"
#include "Pages/CfwSelectPage.hpp"
#include "Pages/UninstallPage.hpp"
#include "Pages/NcaDumpPage.hpp"
#include "Pages/TextPage.hpp"
#include "Pages/ExternalInstallPage.hpp"
#include "ViewFunctions.hpp"
#include "SwitchThemesCommon/SwitchThemesCommon.hpp"
#include "SwitchThemesCommon/NXTheme.hpp"
#include "Pages/RemoteInstallPage.hpp"
#include "Pages/SettingsPage.hpp"
#include "Pages/RebootPage.cpp"

#include "SwitchTools/PatchMng.hpp"

//#define DEBUG
using namespace std;

static inline void ImguiBindController()
{
	ImGuiIO& io = ImGui::GetIO();

	NAV_DOWN	= (KeyPressed(GLFW_GAMEPAD_BUTTON_DPAD_DOWN)	|| StickAsButton(1) > .3f	|| StickAsButton(3) > .3f );
	NAV_UP		= (KeyPressed(GLFW_GAMEPAD_BUTTON_DPAD_UP)		|| StickAsButton(1) < -.3f	|| StickAsButton(3) < -.3f);
	NAV_LEFT	= (KeyPressed(GLFW_GAMEPAD_BUTTON_DPAD_LEFT)	|| StickAsButton(0) < -.3f	|| StickAsButton(2) < -.3f);
	NAV_RIGHT	= (KeyPressed(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT)	|| StickAsButton(0) > .3f	|| StickAsButton(2) > .3f );

	io.NavInputs[ImGuiNavInput_DpadDown]	= NAV_DOWN	? 1.0f : 0;
	io.NavInputs[ImGuiNavInput_DpadUp]		= NAV_UP	? 1.0f : 0;
	io.NavInputs[ImGuiNavInput_DpadLeft]	= NAV_LEFT	? 1.0f : 0;
	io.NavInputs[ImGuiNavInput_DpadRight]	= NAV_RIGHT	? 1.0f : 0;

	io.NavInputs[ImGuiNavInput_Activate] = KeyPressed(GLFW_GAMEPAD_BUTTON_A);
	io.NavInputs[ImGuiNavInput_Cancel] = KeyPressed(GLFW_GAMEPAD_BUTTON_B);

	io.NavActive = true;
	io.NavVisible = true;
}

static bool IsRendering = false;
stack<IUIControlObj*> views;
bool doPopPage = false;

IUIControlObj *ViewObj = nullptr;

void PopPage()
{
	doPopPage = true;
}

static void _PopPage()
{
	doPopPage = false;
	delete views.top();
	views.pop();
	if (views.size() == 0)
	{
		//Dialog("Error: Can't pop last page");
		return;
	}
	ViewObj = views.top();
}

void PushPage(IUIControlObj* page) //All pages must be dynamically allocated
{
	views.push(page);
	ViewObj = page;
}

vector<function<void()>> DeferredFunctions;
void PushFunction(const std::function<void()>& fun)
{
	DeferredFunctions.push_back(fun);
}

static void ExecuteDeferredFunctions() 
{
	auto vec = DeferredFunctions;
	DeferredFunctions.clear();
	for (auto& fun : vec)
		fun();
}

void PushPageBlocking(IUIControlObj* page)
{
	if (IsRendering)
	{
		LOGf("Attempted to push a blocking page while rendering");
		PushFunction([page]() {PushPageBlocking(page); });
		return;
	}

	PushPage(page);
	while (AppMainLoop() && ViewObj)
	{
		PlatformGetInputs();
		ImguiBindController();
		PlatformImguiBinds();
		
		IUIControlObj* CurObj = ViewObj;

		IsRendering = true;
		UiStartFrame();
		CurObj->Render(0,0);
		UiEndFrame();
		IsRendering = false;

		if (CurObj == ViewObj)
			CurObj->Update();
		
		ExecuteDeferredFunctions();
		if (doPopPage)
		{
			_PopPage();
			break;
		}

		PlatformSleep(1 / 30.0f * 1000);
	}
}

void Dialog(const string &msg)
{
	PushPage(new DialogPage(msg));
}

//TODO less hacky way
void DialogBlocking(const string &msg)
{
	PushPageBlocking(new DialogPage(msg));
}

static inline void DisplayLoading(LoadingOverlay &&o)
{
	UiStartFrame();
	o.Render(0, 0);
	UiEndFrame();
}

void DisplayLoading(const string& msg)
{
	DisplayLoading(LoadingOverlay(msg));
}

void DisplayLoading(std::initializer_list<std::string> lines)
{
	DisplayLoading(LoadingOverlay(lines));
}

#ifdef DEBUG
double previousTime = glfwGetTime();
int frameCount = 0;
int fpsValue = 0;

static void calcFPS() 
{
	double currentTime = glfwGetTime();
	frameCount++;

	if (currentTime - previousTime >= 1.0)
	{
		fpsValue = frameCount;

		frameCount = 0;
		previousTime = currentTime;
	}
	ImGui::Text("FPS %d", fpsValue);
	for (int i = 0; i < 6; i++)
	{
		ImGui::Text("pad[%d] = %d %f %f ", i, (int)(StickAsButton(i) != 0), gamepad.axes[i], OldGamepad.axes[i]);
	}
}
#endif

static void MainLoop()
{
	while (AppMainLoop() && ViewObj)
	{
		PlatformGetInputs();
		ImguiBindController();
		PlatformImguiBinds();

		//A control may push a page either in the render or the update function.
		IUIControlObj* CurObj = ViewObj;

		IsRendering = true;
		UiStartFrame();		
		CurObj->Render(0,0);
#ifdef DEBUG
		calcFPS();
#endif
		UiEndFrame();
		IsRendering = false;

		if (CurObj == ViewObj)
			CurObj->Update();
		
		ExecuteDeferredFunctions();
		if (doPopPage)
			_PopPage();

		PlatformSleep(1 / 30.0f * 1000);
	}
}

class QuitPage : public IPage
{
	public:
		QuitPage()
		{
			Name = "退出";
		}	
		
		void Render(int X, int Y) override {}
		
		void Update() override
		{
			SetAppShouldClose();
		}
};

void ShowFirstTimeHelp(bool WelcomeScr)
{	
//these are shown from the last to the first
	Dialog("你可以在reddit的  /r/NXThemes   和Qcean Discord服务器(邀请码: cunjgb)上找到一些主题，你也可以在那里寻求帮助. \n\n"
"可以在 https://git.io/fpxAS 这个地址下载Windows应用程序制作你自己的主题\n"
"或者在 https://exelix11.github.io/SwitchThemeInjector/v2 这个网站使用在线制作器设计你自己的主题\n"
"\n"
"就这些啦,开始自定义主题吧  :)");
	Dialog(
	"自定义主题不能存储您的主机内存中，因为它们只安装在SDcard上. \n"
	"如果在安装主题后，你的机器不能启动，手动删除' '0100000000001000'和'0100000000001013'文件夹,它在'SDcard/<你的cfw文件夹>/contents(在reinx和sxos系统上是/titles)'.\n\n"
	"当您更改主机的固件(升级或降级)时，您必须首先卸载主题，因为在sd上安装的文件依赖于固件.\n"
	"如果您安装的固件支持主题，您可以在更新后将它们安装回去.\n\n"
	"固件9.0版本后的锁屏主题在所有CFWs上都不受支持，因为一些不能通过ip打补丁."
	);
	if (WelcomeScr)
		Dialog("欢迎使用主题安装器   " + VersionString + "!\n\n下一个页面包含了一些重要的信息，建议大家仔细阅读。\n下一个页面只会显示一次，你可以在 '参与人员' 选项卡再次阅读。" );
}

// Note that CfwFolder is set after the constructor of any page pushed before CheckCFWDir is called, CfwFolder shouldn't be used until the theme is actually being installed
static void CheckCFWDir()
{
	auto f = fs::SearchCfwFolders();
	if (f.size() != 1)
		PushPageBlocking(new CfwSelectPage(f));
}

static vector<string> GetArgsInstallList(int argc, char** argv)
{
	if (argc <= 1) return {};

	vector<string> Args(argc - 1);
	for (size_t i = 0; i < Args.size(); i++)
		Args[i] = string(argv[i + 1]);

	//Appstore-style args
	if (StrStartsWith(Args[0], "installtheme="))
	{
		string key = "installtheme=";
		string pathss;
		std::vector<std::string> paths;
		for (auto argvs : Args)
		{
			auto pos = argvs.find(key);
			size_t index;
			while (true)
			{
				index = argvs.find("(_)");
				if (index == std::string::npos) break;
				argvs.replace(index, 3, " ");
			}
			if (pos != std::string::npos)
				pathss = argvs.substr(pos + 13);

			if (!pathss.empty())
			{
				string path;
				stringstream stream(pathss);
				while (getline(stream, path, ',')) {
					paths.push_back(path);
				}
			}
		}
		return paths;
	}
	else //File args from nx-hbloader
	{
		vector<string> res;

		for (auto& s : Args)
			if (filesystem::exists(s) && filesystem::is_regular_file(s))
				res.push_back(move(s));

		return res;
	}
}	

std::string SystemVer = "";
static void SetupSysVer()
{
#if __SWITCH__
	setsysInitialize();
	SetSysFirmwareVersion firmware;
	auto res = setsysGetFirmwareVersion(&firmware);
	if (R_FAILED(res))
	{
		setsysExit();
		DialogBlocking("Could not get sys ver res=" + to_string(res));
		return;
	}
	HOSVer = { firmware.major,firmware.minor,firmware.micro };
	setsysExit();
#else 
	HOSVer = { 9,0,0 };
#endif
	if (HOSVer.major <= 5)
	{
		ThemeTargetToName = ThemeTargetToName5X;
		ThemeTargetToFileName = ThemeTargetToFileName6X; //The file names are the same
	}
	else //6.X
	{
		ThemeTargetToName = ThemeTargetToName6X;
		ThemeTargetToFileName = ThemeTargetToFileName6X;
	}
	SystemVer = to_string(HOSVer.major) + "." + to_string(HOSVer.minor) + "." + to_string(HOSVer.micro);
}

int main(int argc, char **argv)
{
	PlatformInit();

	if (!UIMNG::InitUI())
	{
		PlatformExit();
		return -1;
	}
	PlatformAfterInit();

	SetupSysVer();
	DisplayLoading("加载系统信息...");
	
	bool ThemesFolderExists = fs::CheckThemesFolder();
	NcaDumpPage::CheckHomeMenuVer();
	CheckCFWDir();

	const char* PatchErrorMsg = PatchMng::EnsureInstalled();
	if (!PatchErrorMsg && UseLowMemory) // if patches are fine, check if applet mode
		PatchErrorMsg = "目前运行在applet模式下，当从相册启动自制程序时，可用内存更少。\n\n本程序应该可以正常工作，但如果你遇到崩溃问题，尝试通过在主菜单启动，从主菜单打开游戏的同时按R 或者使用nsp前端程序启动.";

	if (
#ifdef __SWITCH__
		envHasArgv() &&
#endif
		argc > 1)
	{
		auto paths = GetArgsInstallList(argc,argv);
		if (paths.size() == 0)
			goto APP_QUIT;

		PushPage(new ExternalInstallPage(paths));	
		MainLoop();
		
		goto APP_QUIT;
	}	

	{
		TabRenderer *t = new TabRenderer();
		PushPage(t);
		
		if (!ThemesFolderExists)
			ShowFirstTimeHelp(true);

		TextPage* PatchFailedWarning = nullptr;
		if (PatchErrorMsg)
		{
			PatchFailedWarning = new TextPage("警告", PatchErrorMsg);
			t->AddPage(PatchFailedWarning);
		}

		DisplayLoading("加载主题列表...");
		auto ThemeFiles = fs::GetThemeFiles();
		ThemesPage *p = new ThemesPage(ThemeFiles);
		t->AddPage(p);
		UninstallPage *up = new UninstallPage();
		t->AddPage(up);
		NcaDumpPage *dp = new NcaDumpPage();
		t->AddPage(dp);
		RemoteInstallPage *rmi = new RemoteInstallPage();
		t->AddPage(rmi);
		SettingsPage *sf = new SettingsPage();
		t->AddPage(sf);
		CreditsPage *credits = new CreditsPage();
		t->AddPage(credits);
		RebootPage *reboot = new RebootPage();
		t->AddPage(reboot);
		QuitPage *q = new QuitPage();
		t->AddPage(q);
		
		MainLoop();
		
		if (PatchFailedWarning)
			delete PatchFailedWarning;
		delete p;
		delete up;
		delete dp;
		delete rmi;
		delete sf;
		delete credits;
		delete q;
	}
	
APP_QUIT:

	while (views.size() != 0)
	{
		delete views.top();
		views.pop();
	}

	UIMNG::ExitUI();
	PlatformExit();
	
    return 0;
}