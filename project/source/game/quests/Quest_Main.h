#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// removed in 0.10
class Quest_Main : public Quest
{
public:
	enum Progress
	{
		Started,
		TalkedWithMayor
	};

	void Start() override;
	cstring GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool Load(HANDLE file) override;
};
