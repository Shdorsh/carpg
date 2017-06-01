#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_KillAnimals : public Quest_Dungeon
{
public:
	enum Progress
	{
		None,
		Started,
		ClearedLocation,
		Finished,
		Timeout
	};

	void Start();
	cstring GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool Load(HANDLE file);
};
