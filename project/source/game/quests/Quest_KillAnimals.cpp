#include "Pch.h"
#include "Base.h"
#include "Quest_KillAnimals.h"
#include "Dialog.h"
#include "Game.h"
#include "QuestManager.h"
#include "City.h"

//=================================================================================================
void Quest_KillAnimals::Start()
{
	quest_id = Q_KILL_ANIMALS;
	type = QuestType::Captain;
	start_loc = game->current_location;
}

//=================================================================================================
cstring Quest_KillAnimals::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return "q_kill_animals_start";
	case QUEST_DIALOG_FAIL:
		return "q_kill_animals_timeout";
	case QUEST_DIALOG_NEXT:
		return "q_kill_animals_end";
	default:
		return nullptr;
	}
}

//=================================================================================================
void Quest_KillAnimals::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		// player accepted quest
		{
			StartQuest(game->txQuest[76]);

			Location& sl = *game->locations[start_loc];

			// event
			target_loc = game->GetClosestLocation(rand2()%2 == 0 ? L_FOREST : L_CAVE, sl.pos);
			location_event_handler = this;
			
			Location& tl = *game->locations[target_loc];
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			quest_manager.quests.push_back(this);
			quest_manager.quests_timeout.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);

			AddEntry(game->txQuest[29], sl.name.c_str(), game->day+1, game->month+1, game->year);
			AddEntry(game->txQuest[77], sl.name.c_str(), tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos));

			if(game->IsOnline() && now_known)
				game->Net_ChangeLocationState(target_loc, false);
		}
		break;
	case Progress::ClearedLocation:
		// player cleared location from animals
		{
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
			AddEntry(game->txQuest[78], game->locations[target_loc]->name.c_str());
		}
		break;
	case Progress::Finished:
		// player talked with captain, end of quest
		{
			((City*)game->locations[start_loc])->quest_captain = CityQuestState::None;
			game->AddReward(1200);
			AddEntry(game->txQuest[79]);
			SetState(QuestEntry::FINISHED);
		}
		break;
	case Progress::Timeout:
		// player failed to clear location in time
		{
			((City*)game->locations[start_loc])->quest_captain = CityQuestState::Failed;
			AddEntry(game->txQuest[80]);
			SetState(QuestEntry::FAILED);
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_KillAnimals::FormatString(const string& str)
{
	if(str == "target_loc")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_KillAnimals::IsTimedout() const
{
	return game->worldtime - start_time > 30;
}

//=================================================================================================
bool Quest_KillAnimals::OnTimeout(TimeoutType ttype)
{
	if(prog == Progress::Started)
	{
		AddEntry(game->txQuest[277]);
		game->AbadonLocation(game->locations[target_loc]);
	}

	return true;
}

//=================================================================================================
void Quest_KillAnimals::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started && !timeout)
		SetProgress(Progress::ClearedLocation);
}

//=================================================================================================
bool Quest_KillAnimals::IfNeedTalk(cstring topic) const
{
	return (strcmp(topic, "animals") == 0 && prog == Progress::ClearedLocation);
}

//=================================================================================================
bool Quest_KillAnimals::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	location_event_handler = this;

	return true;
}
