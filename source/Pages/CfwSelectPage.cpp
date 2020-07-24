#include "CfwSelectPage.hpp"
#include "../ViewFunctions.hpp"
#include "../UI/UIManagement.hpp"

using namespace std;

CfwSelectPage::CfwSelectPage(vector<string> &folders)
{
	Folders = folders;
	if (folders.size() == 0)
		Title = "找不到cfw文件夹。确保你的sd卡根目录中有\"atmosphere\"， \"reinx\"或\"sxos\"文件夹，因为有些cfws不会自动创建它。文件夹名称必须是小写的，不能有空格。\n如果您的cfw不支持在Github上打开问题。按+退出(前端开启需要用HOME键退出)";
	else 
		Title = "检测到多个cfw文件夹，您想使用哪个?";

}

CfwSelectPage::~CfwSelectPage()
{

}

static const int BtnWidth = 500;
static const int XCursorBtn = SCR_W / 2 - BtnWidth / 2;

void CfwSelectPage::Render(int X, int Y)
{
	Utils::ImGuiSetupWin("CfwSelectPage", 10, 10);
	ImGui::SetWindowSize({SCR_W - 20, SCR_H - 20});
	ImGui::SetWindowFocus();

	ImGui::PushFont(font40);
	ImGui::TextWrapped(Title.c_str());
	ImGui::PopFont();

	ImGui::PushFont(font30);
	ImGui::SetCursorPos({ (float)XCursorBtn, ImGui::GetCursorPosY() + 30 });

	int count = 0;
	for (const auto &e : Folders)
	{
		ImGui::SetCursorPosX((float)XCursorBtn);
		if (ImGui::Button(e.c_str(), { BtnWidth, 50 }))
		{
			fs::SetCfwFolder(e);
			PopPage();
		}
		count++;
	}

	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void CfwSelectPage::Update()
{
	if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_START])
		SetAppShouldClose();
}




