#include "fs.hpp"
#include "Platform/Platform.hpp"

#include <filesystem>
#include "ViewFunctions.hpp"
#include <sys/stat.h>

using namespace std;
using namespace fs;

static string CfwFolder = "";
static string TitlesFolder = "";

string fs::GetCfwFolder() { return CfwFolder; }

string fs::GetFsMitmFolder() { return CfwFolder + TitlesFolder; }

void fs::SetCfwFolder(const string& s)
{
	CfwFolder = s;
	if ((CfwFolder == SD_PREFIX ATMOS_DIR || CfwFolder == SD_PREFIX REINX_DIR) && filesystem::exists(CfwFolder + "contents/"))
		TitlesFolder = "contents/";
	else
		TitlesFolder = "titles/";
}

vector<string> fs::SearchCfwFolders()
{
	vector<string> res;
	DIR * dir = nullptr;
	#define CHECKFOLDER(f) dir = opendir(f); \
	if (dir) { res.push_back(f); closedir(dir); dir = nullptr;}
	CHECKFOLDER(SD_PREFIX ATMOS_DIR)
	CHECKFOLDER(SD_PREFIX REINX_DIR)
	CHECKFOLDER(SD_PREFIX SX_DIR)
	#undef CHECKFOLDER
	if (res.size() == 1)
		SetCfwFolder(res[0]);
	return res;
}

bool StrEndsWith(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool StrStartsWith(const std::string& str, const std::string& prefix)
{
	return str.size() >= prefix.size() &&
		str.compare(0, prefix.size(), prefix) == 0;
}

static string &replaceWindowsPathChar(string& str)
{
	char* c = str.data();
	while (*c)
	{
		if (*c == '\\')
			*c = '/';
		c++;
	}

	return str;
}

static vector<string> GetThemeFilesInDirRecursive(const string &path, int level)
{
	vector<string> res;
	if (level > 7) return res;
	for (auto p : filesystem::directory_iterator(path))
	{
		if (p.is_directory() && p.path().filename() != "systemData" && p.path().filename() != "shuffle")
		{
			auto path = p.path().string();
			res.push_back(replaceWindowsPathChar(path));
			auto v = GetThemeFilesInDirRecursive(p.path().string(), level + 1);
			res.insert(res.end(), v.begin(), v.end());
		}
		else if (p.is_regular_file())
		{
			if (StrEndsWith(p.path().string(), ".szs") || StrEndsWith(p.path().string(), ".nxtheme") || StrEndsWith(p.path().string(), ".ttf")) {
				auto str = p.path().string();
				res.push_back(replaceWindowsPathChar(str));
			}
		}
	}
	return res;
}

vector<string> fs::GetThemeFiles()
{
	vector<string> res;
	
	{	
		DIR *dir = opendir(SD_PREFIX "/themes");
		if (dir)
			closedir(dir);
		else
			return res;
	}
	
	res = GetThemeFilesInDirRecursive(SD_PREFIX "/themes", 0);
	
	return res;	
}

vector<u8> fs::OpenFile(const string &name)
{
	FILE* f = fopen(name.c_str(),"rb");
	if (!f){
		DialogBlocking(
			"读取文件 " + name + "失败 !\n" 
			"这可能是因为你的SD卡是exfat格式的, 很可能是因为你用的MAC电脑格式化的这张SD卡.\n"
			"尝试用Windows系统格式化一遍 或者删除主题文件夹并通过FTP复制文件");
		return {};
	}
	fseek(f,0,SEEK_END);
	auto len = ftell(f);
	rewind(f);

	vector<u8> coll(len);
	fread(coll.data(), 1, len, f);
	fclose(f);
	return coll;
}

void fs::WriteFile(const string &name,const vector<u8> &data)
{
	if (filesystem::exists(name))
		remove(name.c_str());
	
	FILE* f = fopen(name.c_str(),"wb");
	if (!f)
	{
		DialogBlocking("存储文件 " + name + "失败 !");
		return;
	}
	fwrite(data.data(),1,data.size(),f);
	fflush(f);
	fclose(f);
}

void fs::RecursiveDeleteFolder(const string &path)
{
	if (!filesystem::exists(path)) return;
	vector<string> toDelete;
	for (auto p : filesystem::directory_iterator(path))
	{
		if (p.is_directory())
		{
			toDelete.push_back(p.path().string());
		}
		else if (p.is_regular_file())
		{
			remove(p.path().string().c_str());
		}
	}
	for (auto p : toDelete)
	{
		RecursiveDeleteFolder(p);
		rmdir(p.c_str());
	}
	rmdir(path.c_str());
}

void fs::UninstallTheme(bool full)
{
	#define DelDirFromCfw(x) if (filesystem::exists(fs::GetFsMitmFolder() + x)) \
		RecursiveDeleteFolder(fs::GetFsMitmFolder() + x);
	
	if (full)
	{
		DelDirFromCfw("0100000000001000")
		DelDirFromCfw("0100000000001013")
	}
	else 
	{
		DelDirFromCfw("0100000000001000/romfs/lyt")
		DelDirFromCfw("0100000000001013/romfs/lyt")
	}
	DelDirFromCfw("0100000000001007") //Player select
	DelDirFromCfw("0100000000000811") //Custom font
	DelDirFromCfw("0100000000000039") //needed to enable custom font
	
	#undef DelDirFromCfw
}

void fs::CreateFsMitmStructure(const string &tid)
{
	string path = GetFsMitmFolder();
	mkdir(path.c_str(), ACCESSPERMS);
	path += tid + "/";
	mkdir(path.c_str(), ACCESSPERMS);
	if (!filesystem::exists(path + "fsmitm.flag"))
	{
		vector<u8> t; 
		WriteFile(path + "fsmitm.flag", t);
	}		
}

void fs::CreateRomfsDir(const std::string &tid)
{
	string path = GetFsMitmFolder() + tid + "/romfs";
	mkdir(path.c_str(), ACCESSPERMS);
}

void fs::CreateThemeStructure(const string &tid)
{	
	CreateFsMitmStructure(tid);
	CreateRomfsDir(tid);
	mkdir((GetFsMitmFolder() + tid + "/romfs/lyt").c_str(), ACCESSPERMS);
}

bool fs::CheckThemesFolder()
{
	if (!filesystem::exists(SD_PREFIX "/themes"))
		mkdir(SD_PREFIX "/themes", ACCESSPERMS);
	bool Result = filesystem::exists(SD_PREFIX "/themes/systemData");
	if (!Result)
		mkdir(SD_PREFIX "/themes/systemData", ACCESSPERMS);
	return Result;
}

string fs::GetFileName(const string &path)
{
	return path.substr(path.find_last_of("/\\") + 1);
}

string fs::GetPath(const string &path)
{
	return path.substr(0, path.find_last_of("/\\") + 1);
}

string fs::GetParentDir(const string &path)
{
	string _path = path;
	if (StrEndsWith(_path,"/"))
		_path = _path.substr(0,_path.length() - 1);
	
	return _path.substr(0, _path.find_last_of("/\\") + 1);
}

#ifdef __SWITCH__
string fs::GetNcaPath(u64 tid)
{
	char path[FS_MAX_PATH] = {0};
	auto rc = lrInitialize();		
	if (R_FAILED(rc))
		DialogBlocking((string)"lrInitialize : " + to_string(rc));
	
	LrLocationResolver res;
	rc = lrOpenLocationResolver(NcmStorageId_BuiltInSystem,&res);
	if (R_FAILED(rc))
		DialogBlocking((string)"lrOpenLocationResolver :" + to_string(rc));
	
	rc = lrLrResolveProgramPath(&res, tid, path);
	if (R_FAILED(rc))
		DialogBlocking((string)"lrLrResolveDataPath : "+ to_string(rc));
	
	std::string result(path);
	result.erase(0, ((std::string)"@SystemContent://").length());
	return (std::string)"System:/Contents/" + result;
}
#endif

bool fs::DumpHomeMenuNca()
{	
#ifdef __SWITCH__
	FsFileSystem sys;
    fsOpenBisFileSystem(&sys, FsBisPartitionId_System, "");
	fsdevMountDevice("System", sys);
	try {		
		auto targetNca = GetNcaPath(0x0100000000001000);
		WriteFile(SD_PREFIX "/themes/systemData/home.nca",OpenFile(targetNca));
		targetNca = GetNcaPath(0x0100000000001013);
		WriteFile(SD_PREFIX "/themes/systemData/user.nca",OpenFile(targetNca));
	}
	catch (...)
	{
		fsdevUnmountDevice("System");
		fsFsClose(&sys);
		return false;
	}
	fsdevUnmountDevice("System");
	fsFsClose(&sys);
	return true;
#else
	return false;
#endif
}

void fs::RemoveSystemDataDir()
{
	RecursiveDeleteFolder(SD_PREFIX "/themes/systemData/");
	mkdir(SD_PREFIX "/themes/systemData/",ACCESSPERMS);
}

bool fs::WriteHomeDumpVer()
{
	FILE* ver = fopen(SD_PREFIX "/themes/systemData/ver.cfg", "w");
	if (!ver)
		return false;
	fprintf(ver, "%s", SystemVer.c_str());
	fclose(ver);
	return true;
}