#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "MyTypes.h"
#include <unordered_map>
#include "SarcLib/Sarc.hpp"

struct ThemeFileManifest
{
	int Version;
	std::string Author;
	std::string ThemeName;
	std::string LayoutInfo;
	std::string Target;
};

struct SystemVersion { 
	u32 major, minor, micro;
	bool IsGreater(const SystemVersion& other) const 
	{
		if (major > other.major)
			return true;
		else if (major < other.major)
			return false;
		else
		{
			if (minor > other.minor)
				return true;
			else if (minor < other.minor)
				return false;
			else return micro > other.micro;
		}
	}
	bool IsEqual(const SystemVersion& other) const
	{
		return other.major == major && other.minor == minor && other.micro == micro;
	}
};

extern SystemVersion HOSVer;

extern std::unordered_map<std::string,std::string> ThemeTargetToName;
extern std::unordered_map<std::string,std::string> ThemeTargetToFileName;

const std::unordered_map<std::string,std::string> ThemeTargetToName6X
{
	{"home","主菜单"},
	{"lock","锁定显示器"},
	{"user","用户界面"},
	{"apps","所有软件界面"},
	{"set","设置界面"},
	{"news","新闻界面" },
	//{"opt","Options menu" },
	{"psl","玩家选择界面" },
};

const std::unordered_map<std::string,std::string> ThemeTargetToFileName6X
{
	{"home","ResidentMenu.szs"},
	{"lock","Entrance.szs"},
	{"user","MyPage.szs"},
	{"apps","Flaunch.szs"},
	{"set","Set.szs"},
	{"news","Notification.szs"},
	//{"opt","Option.szs" },
	{"psl","Psl.szs" },
};

const std::unordered_map<std::string,std::string> ThemeTargetToName5X
{
	{"home","主菜单"},
	{"lock","锁定显示器"},
	{"user","用户界面"},
	{"apps","所有软件界面"},
	{"set","设置界面"},
	{"news","新闻界面"},
	//{"opt","Options menu" },
	{"psl","玩家选择界面" },
};

ThemeFileManifest ParseNXThemeFile(SARC::SarcData &SData);