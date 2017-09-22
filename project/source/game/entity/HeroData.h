// dane bohatera - jednostki któr¹ mo¿na zrekrutowaæ
#pragma once

//-----------------------------------------------------------------------------
#include "HeroPlayerCommon.h"

//-----------------------------------------------------------------------------
struct HeroData : public HeroPlayerCommon
{
	enum Mode
	{
		Wander,
		Wait,
		Follow,
		Leave
	};

	Unit* following; // unit to fallow when warping
	int expe; // experience gain
	float phase_timer;
	bool team_member, // is team member
		lost_pvp, // lost pvp recently
		melee, // prefer melee combat
		phase, // trying to get unstuck
		free, // don't get shares
		gained_gold; // temporary, used in spliting shares

	bool GetKnowName() const { return know_name; }
	Mode GetMode() const { return mode; }
	void Init(Unit& unit);
	int JoinCost() const;
	void LevelUp();
	void Load(HANDLE file);
	void PassTime(int days = 1, bool travel = false);
	void Save(HANDLE file);
	void SetKnowName(bool know_name);
	void SetKnowNameDirect(bool new_know_name) { know_name = know_name; }
	void SetMode(Mode mode);

	static void Register();

private:
	Mode mode;
	bool know_name; // player know hero name
};
