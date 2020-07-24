#include "ThemePage.hpp"
#include "../ViewFunctions.hpp"
#include <algorithm>
#include "../Platform/Platform.hpp"

#include "../UI/imgui/imgui_internal.h"

using namespace std;

ThemesPage::ThemesPage(const std::vector<std::string> &files) : 
lblPage(""),
NoThemesLbl(
	"没有找到主题，把你的主题复制到SD根目录的 /theme文件夹中，然后再试一次.\n"
	"确保SD卡中的主题文件夹里的文件名称都是小写\n你可以在谷歌或discord上请求帮助.")
{
	if (UseLowMemory)
		LimitLoad = 15;

	Name = "主题列表";
	ThemeFiles = files;
	lblCommands = CommandsTextNormal;
	std::sort(ThemeFiles.begin(), ThemeFiles.end());
	SetDir(SD_PREFIX "/themes");
}

ThemesPage::~ThemesPage()
{
	SetPage(-1);
}

void ThemesPage::SetDir(const string &dir)
{
	LastPageMap[CurrentDir] = tuple<int,int>(pageNum, menuIndex);

	CurrentDir = dir;
	if (!StrEndsWith(dir, "/"))
		CurrentDir += "/";
	
	CurrentFiles.clear();
	for (auto f : ThemeFiles)
		if (fs::GetPath(f) == CurrentDir)
			CurrentFiles.push_back(f);
	
	pageCount = CurrentFiles.size() / LimitLoad + 1;
	if (CurrentFiles.size() % LimitLoad == 0)
		pageCount--;

	pageNum = -1; //force setpage to reload the entries even if in the same page as the path changed
	if (LastPageMap.count(dir))
	{
		const auto& [num, index] = LastPageMap[dir];
		SetPage(num, index);
	}
	else SetPage(0);
}

void ThemesPage::SetPage(int num, int index)
{
	ImGui::NavMoveRequestCancel();
	if (pageNum != num || index != 0)
	{
		menuIndex = index;
		ResetScroll = true;
	}

	if (pageNum == num)	return;
	DisplayEntries.clear();
	
	size_t baseIndex = num * LimitLoad;
	if (num < 0 || baseIndex >= CurrentFiles.size())  
	{
		lblPage = (CurrentDir + " - 没有找到主题");
		pageNum = num;
		return;
	}
	
	//if (CurrentFiles.size() > 10)
	//	DisplayLoading("Loading...");
	
	int imax = CurrentFiles.size() - baseIndex;
	if (imax > LimitLoad) imax = LimitLoad;
	for (int i = 0; i < imax; i++)
		DisplayEntries.push_back(ThemeEntry::FromFile(CurrentFiles[baseIndex + i]));
	
	pageNum = num;
	auto LblPStr = CurrentDir + "  - 页面 " + to_string(num + 1) + "/" + to_string(pageCount);
	if (SelectedFiles.size() != 0)
		LblPStr = "("+ to_string(SelectedFiles.size()) + " 选中) " + LblPStr;
	lblPage = LblPStr;
	lblCommands = (SelectedFiles.size() == 0 ? CommandsTextNormal : CommandsTextSelected);
}

void ThemesPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage("主题页面容器", X, Y, DefaultWinFlags | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::PushFont(font25);

	if (DisplayEntries.size() == 0)
		ImGui::TextWrapped(NoThemesLbl.c_str());

	ImGui::SetCursorPosY(600);
	Utils::ImGuiRightString(lblPage);
	
	if (DisplayEntries.size() == 0)
		goto QUIT_RENDERING;

	ImGui::SetCursorPosY(570);
	ImGui::TextUnformatted(lblCommands.c_str());

	{
		Utils::ImGuiSetupPage(this, X, Y, DefaultWinFlags & ~ImGuiWindowFlags_NoScrollbar);
		int setNewMenuIndex = 0;
		if (ResetScroll || ImGui::GetCurrentWindow()->Appearing)
		{
			setNewMenuIndex = menuIndex;
			ImGui::NavMoveRequestCancel();
			ImGui::SetScrollY(0);
			FocusEvent.Set();
			ResetScroll = false;
		}
		ImGui::SetWindowSize(TabPageArea);
		{
			int count = 0;
			for (auto& e : DisplayEntries)
			{
				bool Selected = IsSelected(e->GetPath());
				if (Selected)
					ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x366e64ff);

				if (e->IsHighlighted())
					menuIndex = count;
				auto res = e->Render(Selected);

				if (Selected)
					ImGui::PopStyleColor();
				if (count == setNewMenuIndex && FocusEvent.Reset()) Utils::ImGuiSelectItem(true);
				Utils::ImGuiDragWithLastElement();

				if (res == ThemeEntry::UserAction::Preview)
					break;
				else if (res == ThemeEntry::UserAction::Enter)
					PushFunction([count, &e, this]()
						{
							if (e->IsFolder())
								SetDir(e->GetPath());
							else
							{
								if (SelectedFiles.size() == 0)
								{
										e->Install();
								}
								else
								{
									if (menuIndex != count)
										menuIndex = count;
									SelectCurrent();
								}
							}
						});

				count++;
			}
		}

		//Here scrolling is handled by the individual theme entries, Utils::ImGuiSetWindowScrollable is not needed
		Utils::ImGuiCloseWin();
	}
QUIT_RENDERING:
	ImGui::PopFont();
	Utils::ImGuiCloseWin();
}

int ThemesPage::PageItemsCount()
{
	int menuCount = CurrentFiles.size() - pageNum * LimitLoad;
	if (menuCount > LimitLoad)
		menuCount = LimitLoad;
	if (menuCount < 0) return 0;
	return menuCount;
}

inline bool ThemesPage::IsSelected(const std::string &fname)
{
	return (std::find(SelectedFiles.begin(), SelectedFiles.end(), fname) != SelectedFiles.end());
}

void ThemesPage::SelectCurrent()
{
	if (DisplayEntries[menuIndex]->IsFolder() || !DisplayEntries[menuIndex]->CanInstall()) return;
	auto fname = DisplayEntries[menuIndex]->GetPath();
	auto position = std::find(SelectedFiles.begin(), SelectedFiles.end(), fname);
	if (position != SelectedFiles.end())
	{
		SelectedFiles.erase(position);
	}
	else 
	{
		SelectedFiles.push_back(fname);
	}
	lblCommands = (SelectedFiles.size() == 0 ? CommandsTextNormal : CommandsTextSelected);
}

void ThemesPage::Update()
{
	int menuCount = PageItemsCount();	
	
	if (NAV_LEFT)
		Parent->PageLeaveFocus(this);
	if (KeyPressed(GLFW_GAMEPAD_BUTTON_B))
	{
		if (CurrentDir != SD_PREFIX "/themes/")
			SetDir(fs::GetParentDir(CurrentDir));
		else 
			Parent->PageLeaveFocus(this);
	}
	
	if (menuCount <= 0)
		return;

	if ((NAV_UP && menuIndex <= 0) || KeyPressed(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))
	{
		if (pageNum > 0)
		{
			SetPage(pageNum - 1);
			menuIndex = PageItemsCount() - 1;
			return;
		}
		else if (pageCount > 1)
		{
			SetPage(pageCount - 1);
			menuIndex = PageItemsCount() - 1;
			return;
		}
		else
		{
			menuIndex = PageItemsCount() - 1;
			ResetScroll = true;
		}
	}
	else if ((NAV_DOWN && menuIndex >= PageItemsCount() - 1) || KeyPressed(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER))
	{
		if (pageCount > pageNum + 1)
			SetPage(pageNum + 1);
		else if (pageNum != 0)
			SetPage(0);
		else
		{
			menuIndex = 0;
			ResetScroll = true;
		}
	}
	else if (KeyPressed(GLFW_GAMEPAD_BUTTON_Y))
	{
		if (SelectedFiles.size() == 0 && menuIndex >= 0)
			SelectCurrent();
		else {
			SelectedFiles.clear();
			lblCommands = CommandsTextNormal;
		}
	}
	else if (KeyPressed(GLFW_GAMEPAD_BUTTON_START) && SelectedFiles.size() != 0)
	{
		for (string file : SelectedFiles)
		{
			DisplayInstallDialog(file);
			if (!ThemeEntry::FromFile(file)->Install(false))
			{
				Dialog("安装主题失败，进程被取消");
				break;
			}
		}
		SelectedFiles.clear();
		lblCommands = CommandsTextNormal;
		SetPage(pageNum);		
	}
}

void ThemesPage::DisplayInstallDialog(const string& path)
{
	DisplayLoading({ "安装 " + path + " ...", "CFW 文件夹: " + fs::GetCfwFolder() });
}
