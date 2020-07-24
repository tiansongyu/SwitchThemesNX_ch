#include "TextPage.hpp"
#include "../ViewFunctions.hpp"
#include "../UI/imgui/imgui_internal.h"

using namespace std;

TextPage::TextPage(const std::string& title, const std::string& text) :
	Text(text)
{
	Name = title;
	c_str = Text.c_str();
}

TextPage::TextPage(const char* title, const char* text) 
{
	Name = title;
	c_str = text;
}

void TextPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font25);
	ImGui::TextWrapped(c_str);
	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void TextPage::Update()
{
	Parent->PageLeaveFocus(this);
}

CreditsPage::CreditsPage() :
	creditsText("主题安装作者 exelix 汉化作者 invoker__qq " + VersionString + " - 核心版本." + SwitchThemesCommon::CoreVer +
		"\nhttps://github.com/exelix11/SwitchThemeInjector"+
		"\nhttps://ko-fi.com/exelix11\n\n"),
	creditsText2(
		"感谢以下成员:\n"
		"Syroot 提供了二进制链接库\n"
		"AboodXD 提供了Bntx主题修改器和sarc库\n"
		"shchmue 提供了解锁工具(Lockpick\n"
		"ScriesM 提供了hactool工具(底层库)\n"
		"每一个为 Atmosphere and libnx (自制软件底层库)作出贡献的人\n"
		"在github上提供了切换字体的工具的人\n"
		"Fincs 提供了 混合应用模板\n"
		"所有为DearImgui(一个跨平台UI库，本程序即使用DearImgui)作出贡献的人")
{
	Name = "参与人员";
}


extern void ShowFirstTimeHelp(bool WelcomeScr); //from main.cpp
void CreditsPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::SetCursorPosY(20);
	ImGui::PushFont(font30);
	ImGui::TextWrapped(creditsText.c_str());
	ImGui::PopFont();

	ImGui::PushFont(font25);
	ImGui::TextWrapped(creditsText2.c_str());

	if (ImGui::Button("显示初始化时的启动信息"))
		ShowFirstTimeHelp(false);
	PAGE_RESET_FOCUS
	//ImGui::SameLine();
	//if (ImGui::Button("Show licenses"))
	//{
	//	auto f = fs::OpenFile(ASSET("licenses.txt"));
	//	Dialog(string((char*)f.data(), f.size()));
	//}

	IsLayoutBlockingLeft = GImGui->NavId == ImGui::GetCurrentWindow()->GetID("Show licenses");

	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void CreditsPage::Update()
{
	if (Utils::PageLeaveFocusInput(!IsLayoutBlockingLeft))
		Parent->PageLeaveFocus(this);
}

