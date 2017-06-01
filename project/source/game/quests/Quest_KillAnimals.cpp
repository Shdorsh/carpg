#include "Pch.h"
#include "Base.h"
#include "Quest_KillAnimals.h"
#include "Dialog.h"
#include "Game.h"
#include "QuestManager.h"
#include "City.h"
#include "QuestInstance.h"
#include "QuestScheme.h"
#include "script/ScriptManager.h"

//=================================================================================================
void Quest_KillAnimals::Start()
{
	assert(0);
}

//=================================================================================================
cstring Quest_KillAnimals::GetDialog(int type2)
{
	assert(0);
	return nullptr;
}

//=================================================================================================
void Quest_KillAnimals::SetProgress(int prog2)
{
	assert(0);
}

//=================================================================================================
cstring Quest_KillAnimals::FormatString(const string& str)
{
	assert(0);
	return nullptr;
}

//=================================================================================================
bool Quest_KillAnimals::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	// quest finished, no need to upgrade
	if(prog == Finished || prog == Timeout)
		return false;

	// remove old active quest
	if(target_loc != -1)
	{
		Location& loc = *game->locations[target_loc];
		if(loc.active_quest == (Quest_Dungeon*)refid)
		{
			loc.active_quest = nullptr;
			RemoveElement(game->load_location_quest, &loc);
		}
	}

	// can't upgrade this quest, script mapping is missing
	QuestManager& quest_manager = QuestManager::Get();
	auto quest_scheme = quest_manager.FindQuestScheme("kill_animals");
	if(!quest_scheme || !quest_scheme->script_upgrade_func)
		return false;

	// upgrade
	auto loc = game->locations[start_loc];
	AnyData* data = new AnyData(ScriptManager::Get().GetEngine());
	if(target_loc != -1)
		data->Store("target_loc", game->locations[target_loc], "Location@");
	quest_manager.UpgradeQuest(this, quest_scheme, SU_MAYOR, data);
	
	return false;
}
