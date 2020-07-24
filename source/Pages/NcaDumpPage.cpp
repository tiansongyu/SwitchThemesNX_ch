#include "NcaDumpPage.hpp"
#include "../ViewFunctions.hpp"
#include "../fs.hpp"
#include "../SwitchTools/hactool.hpp"
#include <filesystem>
#include "../Platform/Platform.hpp"
#include "../SwitchThemesCommon/NXTheme.hpp"

using namespace std;

NcaDumpPage::NcaDumpPage() : 
 dumpNca("备份默认主题")
{
	Name = "备份默认主题";
	guideText = ("要安装.nxtheme文件，首先需要备份默认主题 \n"
		"每当固件更改时，无论是更新还是降级，都需要这样做.\n"
		"当提取的版本与您的固件不匹配时，将提示您这样做.\n\n"
		"通常不需要手动提取，但如果遇到问题，可以在这里尝试这样做.");
}

void NcaDumpPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font30);

	ImGui::TextWrapped(guideText.c_str());
	if (ImGui::Button(dumpNca.c_str()))
	{
		PushFunction([]() {
			if ((gamepad.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] && gamepad.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER]))
			{
				DialogBlocking("输入超级密钥组合，只有主菜单NCA会被丢弃(不会被提取)");
				DisplayLoading("提取 NCA中...");
				if (fs::DumpHomeMenuNca())
					Dialog("主菜单NCA被提取，现在使用注入器来完成设置。\n如果您不是故意这样做的，请忽略此消息.");
				return;
			}
			if (!YesNoPage::Ask(
				"安装自定义主题之前，你需要首先提取(备份)主菜单，这个过程可能需要几秒钟，不要让你的机器进入睡眠模式，不要按下home键\n"
				"你想继续吗 ?")) return;
			fs::RemoveSystemDataDir();
			try
			{				
				if (hactool::ExtractHomeMenu())
					Dialog("已完成，默认主题被导出(备份成功)，现在可以安装新的nxtheme文件(主题)了!");
			}
			catch (std::runtime_error &err)
			{
				DialogBlocking("在备份默认主题时出错:  " + string(err.what()));
			}
		});
	}
	PAGE_RESET_FOCUS
	
	ImGui::PopFont();
	Utils::ImGuiCloseWin();
}

void NcaDumpPage::Update()
{	
	if (Utils::PageLeaveFocusInput()){
		Parent->PageLeaveFocus(this);
	}
}

void NcaDumpPage::CheckHomeMenuVer()
{
	if (!filesystem::exists(SD_PREFIX "/themes/systemData/ResidentMenu.szs"))
	{
		DialogBlocking("安装自定义主题之前，你需要首先备份默认主题，这个过程可能需要几秒钟，不要让你的机器进入睡眠模式，不要按下home键   按A开始");
		goto DUMP_HOMEMENU;
	}
	
	if (filesystem::exists(SD_PREFIX "/themes/systemData/ver.cfg"))
	{
		FILE *ver = fopen(SD_PREFIX "/themes/systemData/ver.cfg", "r");
		if (ver)
		{
			char str[50] = {0};
			fgets(str,49,ver);
			fclose(ver);
			string version(str);
			if (version != SystemVer) goto ASK_DUMP;
			else return;
		}
		else goto ASK_DUMP;
	}
	else if (HOSVer.major >= 7) goto ASK_DUMP;
	else fs::WriteHomeDumpVer();
	return;
	
ASK_DUMP:
	if (!YesNoPage::Ask("当前固件版本与所提取的主菜单不同，是否要再次提取主菜单?\n如果所提取的主菜单与已安装的主题不匹配，将会出错"))
	{
		DialogBlocking("你不会再看到这条消息，在出错的情况下，你可以手动从主菜单的 `备份默认主题`选项中备份默认主题");
		fs::WriteHomeDumpVer();
		return;
	}
	
DUMP_HOMEMENU:
	fs::RemoveSystemDataDir();
	try
	{
		hactool::ExtractHomeMenu();
	}
	catch (std::runtime_error &err)
	{
		DialogBlocking("在备份默认主题时出错: " + string(err.what()));
	}
}



