#include "SettingsPage.hpp"
#include "../ViewFunctions.hpp"
#include "../Platform/Platform.hpp"

using namespace std;

namespace Settings {
	bool UseAnimations = true;
	bool UseIcons = true;
	bool UseCommon = true;
};

SettingsPage::SettingsPage() 
{
	Name = "设置";
}

void SettingsPage::Render(int X, int Y)
{
	Utils::ImGuiSetupWin(Name.c_str(), X, Y, DefaultWinFlags & ~ImGuiWindowFlags_NoScrollbar);
	ImGui::SetWindowSize(ImVec2(SCR_W - (float)X - 30, SCR_H - (float)Y - 70));
	ImGui::PushFont(font25);

	ImGui::PushFont(font30);
	ImGui::TextUnformatted("主题 设置");
	ImGui::PopFont();
	ImGui::TextWrapped("这些设置只适用于安装nxthemes，并没有被保存，你必须每次启动这个应用程序时切换回原来的设置!");
	ImGui::Checkbox("启用动画效果", &Settings::UseAnimations);
	if (ImGui::IsItemFocused())
		ImGui::SetScrollY(0);
	ImGui::Checkbox("启用自定义图标", &Settings::UseIcons);
	ImGui::Checkbox("启用额外的布局(例如.common.szs)", &Settings::UseCommon);
	PAGE_RESET_FOCUS
	ImGui::NewLine();

	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void SettingsPage::Update()
{	
	if (Utils::PageLeaveFocusInput(true))
	{
		Parent->PageLeaveFocus(this);
		return;
	}
}









