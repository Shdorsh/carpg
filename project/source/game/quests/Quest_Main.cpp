#include "Pch.h"
#include "Base.h"
#include "Quest_Main.h"
#include "QuestManager.h"

// removed in 0.10

//=================================================================================================
void Quest_Main::Start()
{
	assert(0);
}

//=================================================================================================
cstring Quest_Main::GetDialog(int type2)
{
	assert(0);
	return nullptr;
}

//=================================================================================================
void Quest_Main::SetProgress(int prog2)
{
	assert(0);
}

//=================================================================================================
cstring Quest_Main::FormatString(const string& str)
{
	assert(0);
	return nullptr;
}

//=================================================================================================
bool Quest_Main::Load(HANDLE file)
{
	Quest::Load(file);

	FileReader f(file);
	int target_loc, close_loc;
	float timer;

	if(prog == Progress::TalkedWithMayor)
	{
		f >> close_loc;
		f >> target_loc;
	}
	else
		f >> timer;

	if(entry)
		QuestManager::Get().RemoveQuestEntry(entry);

	return false;
}
