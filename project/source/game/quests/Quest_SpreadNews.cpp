#include "Pch.h"
#include "Base.h"
#include "Quest_SpreadNews.h"
#include "Dialog.h"
#include "Game.h"
#include "LocationHelper.h"
#include "QuestManager.h"

//-----------------------------------------------------------------------------
bool SortEntries(const Quest_SpreadNews::Entry& e1, const Quest_SpreadNews::Entry& e2)
{
	return e1.dist < e2.dist;
}

//=================================================================================================
void Quest_SpreadNews::Start()
{
	type = QuestType::Mayor;
	quest_id = Q_SPREAD_NEWS;
	start_loc = game->current_location;
	VEC2 pos = game->locations[start_loc]->pos;
	bool sorted = false;
	for(uint i=0, count = game->settlements; i<count; ++i)
	{
		if(i == start_loc)
			continue;
		Location& loc = *game->locations[i];
		if(loc.type != L_CITY)
			continue;
		float dist = distance(pos, loc.pos);
		bool ok = false;
		if(entries.size() < 5)
			ok = true;
		else
		{
			if(!sorted)
			{
				std::sort(entries.begin(), entries.end(), SortEntries);
				sorted = true;
			}
			if(entries.back().dist > dist)
			{
				ok = true;
				sorted = false;
				entries.pop_back();
			}
		}
		if(ok)
		{
			Entry& e = Add1(entries);
			e.location = i;
			e.given = false;
			e.dist = dist;
		}
	}
}

//=================================================================================================
cstring Quest_SpreadNews::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return "q_spread_news_start";
	case QUEST_DIALOG_FAIL:
		return "q_spread_news_timeout";
	case QUEST_DIALOG_NEXT:
		if(prog == Progress::Started)
			return "q_spread_news_tell";
		else
			return "q_spread_news_end";
	default:
		return nullptr;
	}
}

//=================================================================================================
void Quest_SpreadNews::SetProgress(int prog2)
{
	switch(prog2)
	{
	case Progress::Started:
		// told info to spread by player
		{
			prog = Progress::Started;

			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			quest_manager.quests_timeout2.push_back(this);

			Location& loc = *game->locations[start_loc];
			bool is_city = LocationHelper::IsCity(loc);

			StartQuest(game->txQuest[213]);
			AddEntry(game->txQuest[3], is_city ? game->txForMayor : game->txForSoltys, loc.name.c_str(), game->day+1, game->month+1, game->year);
			AddEntry(game->txQuest[17], Upper(is_city ? game->txForMayor : game->txForSoltys), loc.name.c_str(), FormatString("targets"));
		}
		break;
	case Progress::Deliver:
		// player told news to mayor
		{
			uint ile = 0;
			for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				if(game->current_location == it->location)
				{
					it->given = true;
					++ile;
				}
				else if(it->given)
					++ile;
			}

			Location& loc = *game->locations[game->current_location];
			AddEntry(game->txQuest[18], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());

			if(ile == entries.size())
			{
				prog = Progress::Deliver;
				AddEntry(game->txQuest[19], game->locations[start_loc]->name.c_str());
			}

			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);
		}
		break;
	case Progress::Timeout:
		// player failed to spread news in time
		{
			prog = Progress::Timeout;
			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::Failed;

			AddEntry(game->txQuest[20]);
			SetState(QuestEntry::FAILED);
		}
		break;
	case Progress::Finished:
		// player spread news to all mayors, end of quest
		{
			prog = Progress::Finished;
			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::None;
			game->AddReward(200);

			AddEntry(game->txQuest[21]);
			SetState(QuestEntry::FINISHED);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_SpreadNews::FormatString(const string& str)
{
	if(str == "targets")
	{
		static string s;
		s.clear();
		for(uint i=0, count = entries.size(); i<count; ++i)
		{
			s += game->locations[entries[i].location]->name;
			if(i == count-2)
				s += game->txQuest[264];
			else if(i != count-1)
				s += ", ";
		}
		return s.c_str();
	}
	else if(str == "start_loc")
		return game->locations[start_loc]->name.c_str();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_SpreadNews::IsTimedout() const
{
	return game->worldtime - start_time > 60 && prog < Progress::Deliver;
}

//=================================================================================================
bool Quest_SpreadNews::OnTimeout(TimeoutType ttype)
{
	AddEntry(game->txQuest[277]);

	return true;
}

//=================================================================================================
bool Quest_SpreadNews::IfNeedTalk(cstring topic) const
{
	if(strcmp(topic, "tell_news") == 0)
	{
		if(prog == Progress::Started && !timeout)
		{
			for(vector<Entry>::const_iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				if(it->location == game->current_location)
					return !it->given;
			}
		}
	}
	else if(strcmp(topic, "tell_news_end") == 0)
	{
		return prog == Progress::Deliver && game->current_location == start_loc;
	}
	return false;
}

//=================================================================================================
void Quest_SpreadNews::Save(HANDLE file)
{
	Quest::Save(file);

	if(prog != Finished && prog != Timeout)
	{
		uint count = entries.size();
		WriteFile(file, &count, sizeof(count), &tmp, nullptr);
		WriteFile(file, &entries[0], sizeof(Entry)*count, &tmp, nullptr);
	}
}

//=================================================================================================
void Quest_SpreadNews::Load(HANDLE file)
{
	Quest::Load(file);

	if(prog != Finished && prog != Timeout)
	{
		uint count;
		ReadFile(file, &count, sizeof(count), &tmp, nullptr);
		entries.resize(count);
		ReadFile(file, &entries[0], sizeof(Entry)*count, &tmp, nullptr);
	}
}
