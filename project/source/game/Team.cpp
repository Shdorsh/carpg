#include "Pch.h"
#include "Core.h"
#include "Team.h"
#include "Unit.h"
#include "ScriptManager.h"

TeamSingleton Team;

Unit* TeamSingleton::FindActiveTeamMember(int netid)
{
	for(Unit* unit : active_members)
	{
		if(unit->netid == netid)
			return unit;
	}

	return nullptr;
}

bool TeamSingleton::FindItemInTeam(const Item* item, int refid, Unit** unit_result, int* i_index, bool check_npc)
{
	assert(item);

	for(Unit* unit : members)
	{
		if(unit->IsPlayer() || check_npc)
		{
			int index = unit->FindItem(item, refid);
			if(index != INVALID_IINDEX)
			{
				if(unit_result)
					*unit_result = unit;
				if(i_index)
					*i_index = index;
				return true;
			}
		}
	}

	return false;
}

Unit* TeamSingleton::FindTeamMember(cstring id)
{
	UnitData* unit_data = FindUnitData(id);
	assert(unit_data);

	for(Unit* unit : members)
	{
		if(unit->data == unit_data)
			return unit;
	}

	assert(0);
	return nullptr;
}

uint TeamSingleton::GetActiveNpcCount()
{
	uint count = 0;
	for(Unit* unit : active_members)
	{
		if(!unit->player)
			++count;
	}
	return count;
}

uint TeamSingleton::GetNpcCount()
{
	uint count = 0;
	for(Unit* unit : members)
	{
		if(!unit->player)
			++count;
	}
	return count;
}

int TeamSingleton::GetPCShare()
{
	uint pc = 0, npc = 0;
	for(Unit* unit : active_members)
	{
		if(unit->IsPlayer())
			++pc;
		else if(!unit->hero->free)
			++npc;
	}
	return GetPCShare(pc, npc);
}

int TeamSingleton::GetPCShare(int pc, int npc)
{
	int r = 100 - npc * 10;
	return r / pc;
}

Unit* TeamSingleton::GetRandomSaneHero()
{
	LocalVector<Unit*> v;

	for(Unit* unit : active_members)
	{
		if(unit->IsHero() && !IS_SET(unit->data->flags, F_CRAZY))
			v->push_back(unit);
	}

	return v->at(Rand() % v->size());
}

void TeamSingleton::GetTeamInfo(TeamInfo& info)
{
	info.players = 0;
	info.npcs = 0;
	info.heroes = 0;
	info.sane_heroes = 0;
	info.insane_heroes = 0;
	info.free_members = 0;

	for(Unit* unit : members)
	{
		if(unit->IsPlayer())
			++info.players;
		else
		{
			++info.npcs;
			if(unit->IsHero())
			{
				if(unit->summoner != nullptr)
				{
					++info.free_members;
					++info.summons;
				}
				else
				{
					if(unit->hero->free)
						++info.free_members;
					else
					{
						++info.heroes;
						if(IS_SET(unit->data->flags, F_CRAZY))
							++info.insane_heroes;
						else
							++info.sane_heroes;
					}
				}
			}
			else
				++info.free_members;
		}
	}
}

uint TeamSingleton::GetPlayersCount()
{
	uint count = 0;
	for(Unit* unit : active_members)
	{
		if(unit->player)
			++count;
	}
	return count;
}

bool TeamSingleton::HaveActiveNpc()
{
	for(Unit* unit : active_members)
	{
		if(!unit->player)
			return true;
	}
	return false;
}

bool TeamSingleton::HaveNpc()
{
	for(Unit* unit : members)
	{
		if(!unit->player)
			return true;
	}
	return false;
}

bool TeamSingleton::HaveOtherPlayer()
{
	bool first = true;
	for(Unit* unit : active_members)
	{
		if(unit->player)
		{
			if(!first)
				return true;
			first = false;
		}
	}
	return false;
}

bool TeamSingleton::IsAnyoneAlive()
{
	for(Unit* unit : members)
	{
		if(unit->IsAlive() || unit->in_arena != -1)
			return true;
	}

	return false;
}

bool TeamSingleton::IsTeamMember(Unit& unit)
{
	if(unit.IsPlayer())
		return true;
	else if(unit.IsHero())
		return unit.hero->team_member;
	else
		return false;
}

bool TeamSingleton::IsTeamNotBusy()
{
	for(Unit* unit : members)
	{
		if(unit->busy)
			return false;
	}

	return true;
}

void TeamSingleton::Load(HANDLE file)
{
	int refid;
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	members.resize(count);
	for(Unit*& unit : members)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
		unit = Unit::GetByRefid(refid);
	}

	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	active_members.resize(count);
	for(Unit*& unit : active_members)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
		unit = Unit::GetByRefid(refid);
	}

	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	leader = Unit::GetByRefid(refid);
	ReadFile(file, &team_gold, sizeof(team_gold), &tmp, nullptr);
	ReadFile(file, &crazies_attack, sizeof(crazies_attack), &tmp, nullptr);
	ReadFile(file, &is_bandit, sizeof(is_bandit), &tmp, nullptr);
	ReadFile(file, &free_recruit, sizeof(free_recruit), &tmp, nullptr);
}

void TeamSingleton::Register()
{
	auto& sm = ScriptManager::Get();

	sm.AddType("TeamSingleton")
		.Member("bool allow_free_recruit", offsetof(TeamSingleton, free_recruit))
		.Member("const bool is_bandit", offsetof(TeamSingleton, is_bandit))
		.Method("uint GetActiveNpcCount()", asMETHOD(TeamSingleton, GetActiveNpcCount))
		.Method("uint GetActiveTeamSize()", asMETHOD(TeamSingleton, GetActiveTeamSize))
		.Method("Unit@ GetLeader()", asMETHOD(TeamSingleton, GetLeader))
		.Method("uint GetMaxSize()", asMETHOD(TeamSingleton, GetMaxSize))
		.Method("uint GetNpcCount()", asMETHOD(TeamSingleton, GetNpcCount))
		.Method("uint GetTeamSize()", asMETHOD(TeamSingleton, GetTeamSize))
		.Method("bool HaveActiveNpc()", asMETHOD(TeamSingleton, HaveActiveNpc))
		.Method("bool HaveNpc()", asMETHOD(TeamSingleton, HaveNpc))
		.Method("bool HaveOtherActiveTeamMember()", asMETHOD(TeamSingleton, HaveOtherActiveTeamMember))
		.Method("bool HaveOtherPlayer()", asMETHOD(TeamSingleton, HaveOtherPlayer))
		.Method("bool IsAnyoneAlive()", asMETHOD(TeamSingleton, IsAnyoneAlive))
		.Method("bool IsLeader(Unit@ unit)", asMETHODPR(TeamSingleton, IsLeader, (const Unit&), bool))
		.Method("bool IsTeamMember(Unit@ unit)", asMETHOD(TeamSingleton, IsTeamMember))
		.WithInstance("TeamSingleton Team", &Team);
}

void TeamSingleton::Reset()
{
	team_gold = 0;
	crazies_attack = false;
	is_bandit = false;
	free_recruit = true;
}

void TeamSingleton::Save(HANDLE file)
{
	DWORD tmp;
	uint count = GetTeamSize();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(Unit* unit : members)
		WriteFile(file, &unit->refid, sizeof(unit->refid), &tmp, nullptr);

	count = GetActiveTeamSize();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(Unit* unit : active_members)
		WriteFile(file, &unit->refid, sizeof(unit->refid), &tmp, nullptr);

	WriteFile(file, &leader->refid, sizeof(leader->refid), &tmp, nullptr);
	WriteFile(file, &team_gold, sizeof(team_gold), &tmp, nullptr);
	WriteFile(file, &crazies_attack, sizeof(crazies_attack), &tmp, nullptr);
	WriteFile(file, &is_bandit, sizeof(is_bandit), &tmp, nullptr);
	WriteFile(file, &free_recruit, sizeof(free_recruit), &tmp, nullptr);
}

void TeamSingleton::SaveOnWorldmap(HANDLE file)
{
	DWORD tmp;
	uint count = GetTeamSize();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(Unit* unit : members)
	{
		unit->Save(file, false);
		unit->refid = (int)Unit::refid_table.size();
		Unit::refid_table.push_back(unit);
	}
}
