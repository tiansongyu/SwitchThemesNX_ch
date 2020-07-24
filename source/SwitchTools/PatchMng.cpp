#include "PatchMng.hpp"
#include <string>
#include "../fs.hpp"
#include "../SwitchThemesCommon/NXTheme.hpp"
#include <filesystem>
#include "../Platform/Platform.hpp"
#include <unordered_map>
#include "../UI/DialogPages.hpp"

using namespace std;

static const u32 PatchSetVer = 4;
#define LastSupportedVerSTR "10.1.0"
static const SystemVersion LastSupportedVer = { 10,1,0 };

#define ThemePatchesDir "NxThemesInstaller/"

#define WarnIntro " \n 9.0版本之后,主菜单的某些部分需要自定义代码补丁(exefs补丁)才能正常运行.\n"
#define WarnOutro "\n\n如果没有正确的补丁，一些主题可能会出错，当你安装一个已知会引起问题的主题时，你会收到警告"

//Is there even another CFW ?
const char* WarningCFW = WarnIntro "不幸的是，你的CFW似乎不支持ip补丁的标题." WarnOutro;

static const char* WarningSX = 
	WarnIntro "\n你使用的是SX OS，对这些补丁的支持只在2.9.4 beta版本中添加."
			  "这意味着如果您运行的是旧版本并且您的CFW不兼容，您将看到此警告，因为此应用程序无法检测哪个是您的当前版本.\n如果确定是最新版本，不需理会上述警告\n\n"
			  "当安装锁定显示器主题时，你会被警告，如果你确定你的版本支持它，你可以安装这个主题.\n\n"
			  "如果你没有正确的版本的主题，你的主机将在启动时出错，安装前的警告也显示了怎么解决这个问题";

static const char* WarningFWVer =
	WarnIntro "您正在运行的固件版本可能不受此安装程序支持. 这个安装程序支持 " LastSupportedVerSTR ".\n"
			  "如果主题已经更新了补丁但还是无法正常运行，你应该检查主题安装程序的更新。\n\n"
			  "如果程序存在'小'更新 比如 9.0.0 更新到 9.0.1  这样的更新无所谓，即使不更新可能也没有关系，你可以忽略这个警告。" WarnOutro;

static const char* WarningSDFail = WarnIntro "在访问你的sd卡上的补丁目录时出错，你的sd卡可能损坏" WarnOutro;

const char* PatchMng::InstallWarnStr = 
	"你想要安装的主题在没有主菜单补丁的情况下会出错，因为你没安装需要的兼容的主菜单补丁,"
	"它可能工作，但也有可能在启动时出错。你想继续吗?\n\n"
	"如果在启动时出错，你可以通过手动删除sd卡上/atmosphere/contents中的0100000000001000文件夹来删除主题, (在TX中是 /<your cfw>/titles 这个文件夹 )";

static const unordered_map<string, SystemVersion> PartsRequiringPatch = 
{
	{"Entrance.szs", {9,0,0} }
};

static bool HasLatestPatches = true;

static string GetExefsPatchesPath()
{
	if (fs::GetCfwFolder() == SD_PREFIX ATMOS_DIR)
		return SD_PREFIX ATMOS_DIR "exefs_patches/";
	else if (fs::GetCfwFolder() == SD_PREFIX REINX_DIR)
		return SD_PREFIX REINX_DIR "patches/";
	else if (fs::GetCfwFolder() == SD_PREFIX SX_DIR)
		return SD_PREFIX SX_DIR "exefs_patches/";
	else return "";
}

bool PatchMng::CanInstallTheme(const string& FileName)
{
	if (HOSVer.major < 9) return true;
	if (!PartsRequiringPatch.count(FileName)) return true;
	
	const auto& ver = PartsRequiringPatch.at(FileName);

	if (HOSVer.IsGreater(ver) || HOSVer.IsEqual(ver))
		return HasLatestPatches;
	else return true;

}

bool PatchMng::ExefsCompatAsk(const std::string& SzsName)
{
	if (!PatchMng::CanInstallTheme(SzsName))
		return YesNoPage::Ask(PatchMng::InstallWarnStr);
	return true;
}

void PatchMng::RemoveAll()
{
	fs::RecursiveDeleteFolder(GetExefsPatchesPath() + ThemePatchesDir);
	HasLatestPatches = false;
}

static bool ExtractPatches() 
{
	auto&& p = GetExefsPatchesPath();

	mkdir(p.c_str(), ACCESSPERMS);
	p += ThemePatchesDir;
	mkdir(p.c_str(), ACCESSPERMS);

	try {
		for (const auto& v : filesystem::directory_iterator(ASSET("patches")))
			fs::WriteFile(p + v.path().filename().string(), fs::OpenFile(v.path().string()));
	}
	catch (...)
	{
		return false;
	}

	FILE* f = fopen((p + "ver.txt").c_str(), "w");
	if (!f)
		return false;
	fprintf(f, "%u", PatchSetVer);
	fclose(f);

	return true;
}

const char* PatchMng::EnsureInstalled()
{
	if (HOSVer.major < 9) return nullptr;
	auto&& outDir = GetExefsPatchesPath();
	if (outDir == "")
	{
		HasLatestPatches = false;
		return WarningCFW;
	}

	FILE* f = fopen((outDir + ThemePatchesDir "ver.txt").c_str(), "r");
	if (!f)
		HasLatestPatches = ExtractPatches();
	else {
		u32 CurVer = 0;
		fscanf(f, "%u", &CurVer);
		fclose(f);

		if (CurVer < PatchSetVer)
			HasLatestPatches = ExtractPatches();
	}
	if (!HasLatestPatches)
		return WarningSDFail;

	if (HOSVer.IsGreater(LastSupportedVer)) {
		HasLatestPatches = false;
		return WarningFWVer;
	}

	if (fs::GetCfwFolder() == SD_PREFIX SX_DIR)
	{
		HasLatestPatches = false;
		return WarningSX;
	}

	return nullptr;
}

