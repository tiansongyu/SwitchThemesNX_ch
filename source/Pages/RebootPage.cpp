#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "../SwitchThemesCommon/SwitchThemesCommon.hpp"
#include "../UI/UI.hpp"
#include "../fs.hpp"
#include "../SwitchTools/PayloadReboot.hpp"

#include "../Platform/Platform.hpp"
#include "../ViewFunctions.hpp"

const u32 YELLOW_WARN = 0xffce0aff;
const u32 RED_ERROR = 0xff3419ff;

class RebootPage : public IPage
{
	public:
		RebootPage() :
		DescriptionLbl("重新启动到注入程序允许您重新启动主机，而无需再次注入注入程序。目前它只在大气中支持."),
		ErrorLbl("这个功能在您当前的设置中不可用，您需要使用Atmosphere >= 0.8.3，并在sd卡/ Atmosphere /reboot_payload.bin处重新启动注入程序" ),
		WarningLbl("正确地设置了重新启动到注入程序，但在sd卡上检测到多个CFWs，如果没有运行atmosphere，这可能无法工作 "),
		RebootBtn("重启")
		{
			Name = "重启进入注入程序";
			
			auto v = fs::SearchCfwFolders();
			bool hasAtmos = false;
			if (std::find(v.begin(), v.end(), SD_PREFIX ATMOS_DIR ) != v.end())
			{
				ShowError = false;
				hasAtmos = true;
			}
						
			if (!hasAtmos) return;			
			
			if (!PayloadReboot::Init())
			{				
				ShowError = true;
				return;
			}
			else CanReboot = true;
			
			if (hasAtmos && v.size() != 1)
				ShowWarning = true;
		}
		
		void Render(int X, int Y) 
		{
			Utils::ImGuiSetupPage(this, X, Y);
			ImGui::PushFont(font30);
			ImGui::SetCursorPos({ 5, 10 });

			ImGui::TextWrapped(DescriptionLbl.c_str());
			if (ShowError)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, RED_ERROR);
				ImGui::TextWrapped(ErrorLbl.c_str());
				ImGui::PopStyleColor();
			}
			if (ShowWarning)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, YELLOW_WARN);
				ImGui::TextWrapped(WarningLbl.c_str());
				ImGui::PopStyleColor();
			}
			if (CanReboot)
			{
				if (ImGui::Button(RebootBtn.c_str()))
				{
					PayloadReboot::Reboot();
				}
				PAGE_RESET_FOCUS
			}
			ImGui::PopFont();
			Utils::ImGuiCloseWin();
		}
		
		void Update() override
		{
			if (!CanReboot)
				Parent->PageLeaveFocus(this);			
			
			else if (Utils::PageLeaveFocusInput()){
				Parent->PageLeaveFocus(this);
			}
		}
	private:
		bool CanReboot = false;
		bool ShowError = true;
		bool ShowWarning = false;
	
		std::string DescriptionLbl;
		std::string ErrorLbl;
		std::string WarningLbl;
		std::string RebootBtn;
};