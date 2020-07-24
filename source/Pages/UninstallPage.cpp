#include "UninstallPage.hpp"
#include "../ViewFunctions.hpp"
#include "../SwitchTools/PatchMng.hpp"

using namespace std;

UninstallPage::UninstallPage() : 
lblText("点击卸载，可卸载当前安装的主题。\n如果出现错误，可以按L+R 完全删除LayeredFS目录和主菜单补丁.")
{
	Name = "卸载主题";
}

void UninstallPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font30);

	ImGui::TextWrapped(lblText.c_str());

	ImGui::PushStyleColor(ImGuiCol_Button, u32(0x6B70000ff));
	if (ImGui::Button("卸载"))
	{
		bool FullUninstall = gamepad.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] && gamepad.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
		PushFunction([FullUninstall]() {
			if (!YesNoPage::Ask("你确定这样做吗 ?")) return;
			if (FullUninstall)
			{
				DisplayLoading("清除LayeredFS 文件夹中..");
				fs::UninstallTheme(true);
				PatchMng::RemoveAll();
				Dialog(
					"完成后，所有与主题相关的内容都被删除，重新启动主机查看更改.\n"
					"由于移除了主菜单补丁，请在安装任何主题之前重新启动这个自制程序."
				);
			}
			else
			{
				DisplayLoading("执行中...");
				fs::UninstallTheme(false);
				Dialog("已完成，所有已安装的主题都已删除，重新启动主机查看更改");
			}
		});
	}
	PAGE_RESET_FOCUS
	ImGui::PopStyleColor();

	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void UninstallPage::Update()
{	
	if (Utils::PageLeaveFocusInput()){
		Parent->PageLeaveFocus(this);
	}
}




