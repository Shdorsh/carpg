// attributes & skill profiles
#include "Pch.h"
#include "GameCore.h"
#include "StatProfile.h"

//-----------------------------------------------------------------------------
vector<StatProfile*> StatProfile::profiles;

//=================================================================================================
bool StatProfile::operator != (const StatProfile& p) const
{
	bool result = false;
	if(fixed != p.fixed)
		result = true;
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		if(attrib[i] != p.attrib[i])
		{
			Info("AttributeId %s: %d and %d.", Attribute::attributes[i].id, attrib[i], p.attrib[i]);
			result = true;
		}
	}
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		if(skill[i] != p.skill[i])
		{
			Info("SkillId %s: %d and %d.", Skill::skills[i].id, skill[i], p.skill[i]);
			result = true;
		}
	}
	return result;
}

//=================================================================================================
void StatProfile::Set(int level, int* attribs, int* skills) const
{
	assert(skills && attribs);

	if(level == 0 || fixed)
	{
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
			attribs[i] = attrib[i];
		for(int i = 0; i < (int)SkillId::MAX; ++i)
			skills[i] = max(0, skill[i]);
	}
	else
	{
		int unused;
		float lvl = float(level) / 5;
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
			attribs[i] = attrib[i] + int(Attribute::GetModifier(attrib[i], unused) * lvl);
		for(int i = 0; i < (int)SkillId::MAX; ++i)
		{
			int val = max(0, skill[i]);
			skills[i] = val + int(Skill::GetModifier(val, unused) * lvl);
		}
	}
}

//=================================================================================================
void StatProfile::SetForNew(int level, int* attribs, int* skills) const
{
	assert(skills && attribs);

	if(level == 0 || fixed)
	{
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
		{
			if(attribs[i] == -1)
				attribs[i] = attrib[i];
		}
		for(int i = 0; i < (int)SkillId::MAX; ++i)
		{
			if(skills[i] == -1)
				skills[i] = max(0, skill[i]);
		}
	}
	else
	{
		int unused;
		float lvl = float(level) / 5;
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
		{
			if(attribs[i] == -1)
				attribs[i] = attrib[i] + int(Attribute::GetModifier(attrib[i], unused) * lvl);
		}
		for(int i = 0; i < (int)SkillId::MAX; ++i)
		{
			if(skills[i] == -1)
			{
				int val = max(0, skill[i]);
				skills[i] = val + int(Skill::GetModifier(val, unused) * lvl);
			}
		}
	}
}

//=================================================================================================
StatProfile* StatProfile::TryGet(Cstring id)
{
	for(auto profile : profiles)
	{
		if(profile->id == id)
			return profile;
	}

	return nullptr;
}
