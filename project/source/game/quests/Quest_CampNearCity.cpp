#include "Pch.h"
#include "Base.h"
#include "Quest_CampNearCity.h"
#include "Dialog.h"
#include "Game.h"
#include "LocationHelper.h"
#include "QuestManager.h"

//=================================================================================================
void Quest_CampNearCity::Start()
{
	quest_id = Q_CAMP_NEAR_CITY;
	type = QuestType::Captain;
	start_loc = game->current_location;
	switch(rand2()%3)
	{
	case 0:
		group = SG_BANDYCI;
		break;
	case 1:
		group = SG_GOBLINY;
		break;
	case 2:
		group = SG_ORKOWIE;
		break;
	}
}

//=================================================================================================
cstring Quest_CampNearCity::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return "q_camp_near_city_start";
	case QUEST_DIALOG_FAIL:
		return "q_camp_near_city_timeout";
	case QUEST_DIALOG_NEXT:
		return "q_camp_near_city_end";
	default:
		return nullptr;
	}
}

//=================================================================================================
void Quest_CampNearCity::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		// player accepted quest
		{
			Location& sl = *game->locations[start_loc];
			bool is_city = LocationHelper::IsCity(sl);

			StartQuest(is_city ? game->txQuest[57] : game->txQuest[58]);

			// event
			target_loc = game->CreateCamp(sl.pos, group);
			location_event_handler = this;

			Location& tl = *game->locations[target_loc];
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			cstring gn;
			switch(group)
			{
			case SG_BANDYCI:
			default:
				gn = game->txQuest[59];
				break;
			case SG_ORKOWIE:
				gn = game->txQuest[60];
				break;
			case SG_GOBLINY:
				gn = game->txQuest[61];
				break;
			}

			quest_manager.quests.push_back(this);
			quest_manager.quests_timeout.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);

			AddEntry(game->txQuest[29], sl.name.c_str(), game->day+1, game->month+1, game->year);
			AddEntry(game->txQuest[62], gn, GetLocationDirName(sl.pos, tl.pos), sl.name.c_str(), is_city ? game->txQuest[63] : game->txQuest[64]);

			if(game->IsOnline() && now_known)
				game->Net_ChangeLocationState(target_loc, false);
		}
		break;
	case Progress::ClearedLocation:
		// player cleared location
		{
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
			AddEntry(game->txQuest[65]);
		}
		break;
	case Progress::Finished:
		// player talked with captain, end of quest
		{
			((City*)game->locations[start_loc])->quest_captain = CityQuestState::None;
			game->AddReward(2500);
			AddEntry(game->txQuest[66]);
			SetState(QuestEntry::FINISHED);
		}
		break;
	case Progress::Timeout:
		// player failed to clear camp on time
		{
			City* city = (City*)game->locations[start_loc];
			city->quest_captain = CityQuestState::Failed;
			AddEntry(game->txQuest[67], LocationHelper::IsCity(city) ? game->txQuest[63] : game->txQuest[64]);
			SetState(QuestEntry::FINISHED);
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
cstring Quest_CampNearCity::FormatString(const string& str)
{
	if(str == "Bandyci_zalozyli")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[68];
		case SG_ORKOWIE:
			return game->txQuest[69];
		case SG_GOBLINY:
			return game->txQuest[70];
		default:
			assert(0);
			return game->txQuest[71];
		}
	}
	else if(str == "naszego_miasta")
	{
		if(LocationHelper::IsCity(game->locations[start_loc]))
			return game->txQuest[72];
		else
			return game->txQuest[73];
	}
	else if(str == "miasto")
	{
		if(LocationHelper::IsCity(game->locations[start_loc]))
			return game->txQuest[63];
		else
			return game->txQuest[64];
	}
	else if(str == "dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "bandyci_zaatakowali")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[74];
		case SG_ORKOWIE:
			return game->txQuest[75];
		case SG_GOBLINY:
			return game->txQuest[266];
		default:
			assert(0);
			return nullptr;
		}
	}
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_CampNearCity::IsTimedout() const
{
	return game->worldtime - start_time > 30;
}

//=================================================================================================
bool Quest_CampNearCity::OnTimeout(TimeoutType ttype)
{
	if(prog == Progress::Started)
	{
		AddEntry(game->txQuest[277]);

		if(ttype == TIMEOUT_CAMP)
			game->AbadonLocation(game->locations[target_loc]);
	}

	return true;
}

//=================================================================================================
void Quest_CampNearCity::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started && !timeout)
		SetProgress(Progress::ClearedLocation);
}

//=================================================================================================
bool Quest_CampNearCity::IfNeedTalk(cstring topic) const
{
	return (strcmp(topic, "camp") == 0 && prog == Progress::ClearedLocation);
}

//=================================================================================================
void Quest_CampNearCity::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &group, sizeof(group), &tmp, nullptr);
}

//=================================================================================================
void Quest_CampNearCity::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &group, sizeof(group), &tmp, nullptr);

	location_event_handler = this;
}
