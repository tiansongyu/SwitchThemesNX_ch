#include "ThemeEntry.hpp"
#include "../../fs.hpp"
#include "../../SwitchTools/PatchMng.hpp"
#include "../../SwitchThemesCommon/SarcLib/Yaz0.hpp"
#include "../../ViewFunctions.hpp"
#include "../../SwitchThemesCommon/SwitchThemesCommon.hpp"
#include "../ThemePage.hpp"

using namespace std;

class LegacyEntry : public ThemeEntry
{
public:
	LegacyEntry(const string& fileName, vector<u8>&& RawData)
	{
		FileName = fileName;
		file = move(RawData);
		auto DecompressedFile = Yaz0::Decompress(file);
		ParseLegacyTheme(SARC::Unpack(DecompressedFile));
	}

	LegacyEntry(const string& fileName, SARC::SarcData&& _SData)
	{
		FileName = fileName;

		auto packed = SARC::Pack(_SData);
		file = move(Yaz0::Compress(packed.data));

		ParseLegacyTheme(move(_SData));
	}

	bool IsFolder() override { return false; }
	bool CanInstall() override { return _CanInstall; }
	bool HasPreview() override { return false; }
protected:
	bool DoInstall(bool ShowDialogs = true) override
	{
		if (ShowDialogs)
			ThemesPage::DisplayInstallDialog(FileName);
		PatchTemplate patch = SwitchThemesCommon::SzsPatcher::DetectSarc(SData);

		if (!PatchMng::ExefsCompatAsk(patch.szsName))
			return false;

		fs::CreateThemeStructure(patch.TitleId);;
		fs::WriteFile(fs::GetFsMitmFolder() + patch.TitleId + "/romfs/lyt/" + patch.szsName, file);

		return true;
	}

private:
	bool _CanInstall = true;
	SARC::SarcData SData;

	LoadedImage GetPreview() override
	{
		throw runtime_error("没有为szs主题实现预览");
	}

	void ParseLegacyTheme(SARC::SarcData&& _Sdata)
	{	
		SData = move(_Sdata);
		if (FileName == "")
		{
			lblFname = ("Unknown.szs");
			lblLine1 = ("远程安装");
		}
		else
		{
			lblFname = (fs::GetFileName(FileName));
			lblLine1 = (FileName);
		}
		auto patch = SwitchThemesCommon::SzsPatcher::DetectSarc(SData);
		if (patch.FirmName == "")
		{
			lblLine2 = ("无效的主题");
			_CanInstall = false;
		}
		else lblLine2 = (patch.TemplateName + " for " + patch.FirmName);
	}
};