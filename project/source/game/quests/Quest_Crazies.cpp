#include "Pch.h"
#include "Base.h"
#include "Quest_Crazies.h"
#include "Dialog.h"
#include "Game.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"

//=================================================================================================
void Quest_Crazies::Start()
{
	type = QuestType::Unique;
	quest_id = Q_CRAZIES;
	target_loc = -1;
	crazies_state = State::None;
	days = 0;
	check_stone = false;
}

//=================================================================================================
cstring Quest_Crazies::GetDialog(int type2)
{
	return "q_crazies_trainer";
}

//=================================================================================================
void Quest_Crazies::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started: // zaatakowano przez unk
		{
			StartQuest(game->txQuest[253]);

			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			AddEntry(game->txQuest[170], game->day+1, game->month+1, game->year);
			AddEntry(game->txQuest[254]);
		}
		break;
	case Progress::KnowLocation: // trener powiedzia³ o labiryncie
		{
			target_loc = game->CreateLocation(L_DUNGEON, VEC2(0,0), -128.f, LABIRYNTH, SG_UNK, false);
			start_loc = game->current_location;
			Location& loc = GetTargetLocation();
			loc.active_quest = this;
			loc.state = LS_KNOWN;
			loc.st = 13;

			crazies_state = State::TalkedTrainer;

			AddEntry(game->txQuest[255], game->location->name.c_str(), loc.name.c_str(), GetTargetLocationDir());
			
			if(game->IsOnline())
				game->Net_ChangeLocationState(target_loc, false);
		}
		break;
	case Progress::Finished: // schowano kamieñ do skrzyni
		{
			GetTargetLocation().active_quest = nullptr;

			crazies_state = State::End;

			AddEntry(game->txQuest[256]);
			SetState(QuestEntry::FINISHED);
			quest_manager.EndUniqueQuest();
		}
	}
}

//=================================================================================================
cstring Quest_Crazies::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Crazies::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "szaleni") == 0;
}

//=================================================================================================
void Quest_Crazies::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

	f << crazies_state;
	f << days;
	f << check_stone;
}

//=================================================================================================
void Quest_Crazies::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	if(LOAD_VERSION >= V_0_4)
	{
		GameReader f(file);

		f >> crazies_state;
		f >> days;
		f >> check_stone;
	}
}

//=================================================================================================
void Quest_Crazies::LoadOld(HANDLE file)
{
	int old_refid;
	GameReader f(file);

	f >> crazies_state;
	f >> old_refid;
	f >> check_stone;

	// days was missing in save!
	days = 13;
}
