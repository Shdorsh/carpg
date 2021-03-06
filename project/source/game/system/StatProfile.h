// attributes & skill profiles
#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"

//-----------------------------------------------------------------------------
struct StatProfile
{
	string id;
	bool fixed;
	int attrib[(int)AttributeId::MAX];
	int skill[(int)SkillId::MAX];

	bool operator != (const StatProfile& p) const;

	void Set(int level, int* attribs, int* skills) const;
	void Set(int level, UnitStats& stats) const { Set(level, stats.attrib, stats.skill); }
	void SetForNew(int level, int* attribs, int* skills) const;
	void SetForNew(int level, UnitStats& stats) const { Set(level, stats.attrib, stats.skill); }

	static vector<StatProfile*> profiles;
	static StatProfile* TryGet(Cstring id);
};
