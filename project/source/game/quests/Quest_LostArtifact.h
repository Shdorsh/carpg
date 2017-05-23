#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_LostArtifact : public Quest_Dungeon
{
public:
	enum Progress
	{
		None,
		Started,
		Finished,
		Timeout
	};

	void Start();
	cstring GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout() const;
	bool OnTimeout(TimeoutType ttype);
	bool IfHaveQuestItem2(cstring id) const;
	const Item* GetQuestItem();
	void Save(HANDLE file);
	bool Load(HANDLE file);

private:
	const Item* item;
	OtherItem quest_item;
};
