#pragma once

struct Item;
struct Unit;

struct TeamInfo
{
	int players;
	int npcs;
	int heroes;
	int sane_heroes;
	int insane_heroes;
	int free_members;
	int summons;
};

class TeamSingleton
{
public:
	Unit* FindActiveTeamMember(int netid);
	bool FindItemInTeam(const Item* item, int refid, Unit** unit_result, int* i_index, bool check_npc = true);
	Unit* FindTeamMember(cstring id);
	uint GetActiveNpcCount();
	uint GetActiveTeamSize() { return active_members.size(); }
	Unit* GetLeader() { return leader; }
	uint GetMaxSize() { return 8u; }
	uint GetNpcCount();
	int GetPCShare();
	int GetPCShare(int pc, int npc);
	Unit* GetRandomSaneHero();
	void GetTeamInfo(TeamInfo& info);
	uint GetPlayersCount();
	uint GetTeamSize() { return members.size(); }
	bool HaveActiveNpc();
	bool HaveQuestItem(const Item* item, int refid = -1) { return FindItemInTeam(item, refid, nullptr, nullptr, true); }
	bool HaveNpc();
	bool HaveOtherActiveTeamMember() { return GetActiveTeamSize() > 1u; }
	bool HaveOtherPlayer();
	bool IsAnyoneAlive();
	bool IsLeader(const Unit& unit) { return &unit == GetLeader(); }
	bool IsLeader(const Unit* unit) { assert(unit); return unit == GetLeader(); }
	bool IsTeamMember(Unit& unit);
	bool IsTeamNotBusy();
	void Load(HANDLE file);
	void Register();
	void Reset();
	void Save(HANDLE file);
	void SaveOnWorldmap(HANDLE file);

	vector<Unit*> members; // all team members
	vector<Unit*> active_members; // team members that get gold (without quest units)
	Unit* leader;
	int team_gold; // not divided team gold
	bool crazies_attack, // team attacked by crazies on current level
		free_recruit, // first hero joins for free if playing alone
		is_bandit; // attacked npc, now npc's are aggresive

private:
	Unit* S_GetActiveMember(uint index);
	Unit* S_GetMember(uint index);
};

extern TeamSingleton Team;
