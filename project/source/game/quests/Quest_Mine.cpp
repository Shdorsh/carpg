#include "Pch.h"
#include "Base.h"
#include "Quest_Mine.h"
#include "Dialog.h"
#include "Game.h"
#include "GameFile.h"
#include "SaveState.h"
#include "LocationHelper.h"
#include "QuestManager.h"

//=================================================================================================
void Quest_Mine::Start()
{
	quest_id = Q_MINE;
	type = QuestType::Unique;
	dungeon_loc = -2;
	mine_state = State::None;
	mine_state2 = State2::None;
	mine_state3 = State3::None;
	messenger = nullptr;
	days = 0;
	days_required = 0;
	days_gold = 0;
}

//=================================================================================================
cstring Quest_Mine::GetDialog(int type2)
{
	if(type2 != QUEST_DIALOG_NEXT)
		return nullptr;

	if(game->current_dialog->talker->data->id == "inwestor")
		return "q_mine_investor";
	else if(game->current_dialog->talker->data->id == "poslaniec_kopalnia")
	{
		if(prog == Quest_Mine::Progress::SelectedShares)
			return "q_mine_messenger";
		else if(prog == Quest_Mine::Progress::GotFirstGold || prog == Quest_Mine::Progress::SelectedGold)
		{
			if(days >= days_required)
				return "q_mine_messenger2";
			else
				return "messenger_talked";
		}
		else if(prog == Quest_Mine::Progress::Invested)
		{
			if(days >= days_required)
				return "q_mine_messenger3";
			else
				return "messenger_talked";
		}
		else if(prog == Quest_Mine::Progress::UpgradedMine)
		{
			if(days >= days_required)
				return "q_mine_messenger4";
			else
				return "messenger_talked";
		}
		else
			return "messenger_talked";
	}
	else
		return "q_mine_boss";
}

//=================================================================================================
void Quest_Mine::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			StartQuest(game->txQuest[131]);

			location_event_handler = this;

			Location& sl = GetStartLocation();
			Location& tl = GetTargetLocation();
			at_level = 0;
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}
			else if(tl.state >= LS_ENTERED)
				tl.reset = true;
			tl.st = 10;

			InitSub();

			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);

			AddEntry(game->txQuest[132], sl.name.c_str(), game->day+1, game->month+1, game->year);
			AddEntry(game->txQuest[133], tl.name.c_str(), GetTargetLocationDir());

			if(game->IsOnline() && now_known)
				game->Net_ChangeLocationState(target_loc, false);
		}
		break;
	case Progress::ClearedLocation:
		{
			AddEntry(game->txQuest[134]);
		}
		break;
	case Progress::SelectedShares:
		{
			AddEntry(game->txQuest[135]);
			mine_state = State::Shares;
			mine_state2 = State2::InBuild;
			days = 0;
			days_required = random(30, 45);
			quest_manager.RemoveQuestRumor(P_KOPALNIA);
		}
		break;
	case Progress::GotFirstGold:
		{
			AddEntry(game->txQuest[136]);
			SetState(QuestEntry::FINISHED);
			game->AddReward(500);
			mine_state2 = State2::Built;
			days -= days_required;
			days_required = random(60, 90);
			if(days >= days_required)
				days = days_required - 1;
			days_gold = 0;
		}
		break;
	case Progress::SelectedGold:
		{
			AddEntry(game->txQuest[137]);
			SetState(QuestEntry::FINISHED);
			game->AddReward(3000);
			mine_state2 = State2::InBuild;
			days = 0;
			days_required = random(30, 45);
			quest_manager.RemoveQuestRumor(P_KOPALNIA);
		}
		break;
	case Progress::NeedTalk:
		{
			AddEntry(game->txQuest[138]);
			SetState(QuestEntry::IN_PROGRESS);
			mine_state2 = State2::CanExpand;
			game->AddNews(Format(game->txQuest[139], GetTargetLocationName()));
		}
		break;
	case Progress::Talked:
		{
			AddEntry(game->txQuest[140], mine_state == State::Shares ? 10000 : 12000);
		}
		break;
	case Progress::NotInvested:
		{
			AddEntry(game->txQuest[141]);
			SetState(QuestEntry::FINISHED);
			quest_manager.EndUniqueQuest();
		}
		break;
	case Progress::Invested:
		{
			if(mine_state == State::Shares)
				game->current_dialog->pc->unit->gold -= 10000;
			else
				game->current_dialog->pc->unit->gold -= 12000;
			AddEntry(game->txQuest[142]);
			mine_state2 = State2::InExpand;
			days = 0;
			days_required = random(30, 45);

			if(game->IsOnline() && !game->current_dialog->is_local)
				game->GetPlayerInfo(game->current_dialog->pc).update_flags |= PlayerInfo::UF_GOLD;
		}
		break;
	case Progress::UpgradedMine:
		{
			AddEntry(game->txQuest[143]);
			SetState(QuestEntry::FINISHED);
			game->AddReward(1000);
			mine_state = State::BigShares;
			mine_state2 = State2::Expanded;
			days -= days_required;
			days_required = random(60, 90);
			if(days >= days_required)
				days = days_required - 1;
			days_gold = 0;
			game->AddNews(Format(game->txQuest[144], GetTargetLocationName()));
		}
		break;
	case Progress::InfoAboutPortal:
		{
			AddEntry(game->txQuest[145]);
			SetState(QuestEntry::IN_PROGRESS);
			mine_state2 = State2::FoundPortal;
			game->AddNews(Format(game->txQuest[146], GetTargetLocationName()));
		}
		break;
	case Progress::TalkedWithMiner:
		{
			AddEntry(game->txQuest[147]);
			const Item* item = FindItem("key_kopalnia");
			game->current_dialog->pc->unit->AddItem(item, 1, true);

			if(game->IsOnline())
			{
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, item, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::Finished:
		{
			AddEntry(game->txQuest[148]);
			SetState(QuestEntry::FINISHED);
			quest_manager.EndUniqueQuest();
			game->AddNews(game->txQuest[149]);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Mine::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "burmistrzem")
		return LocationHelper::IsCity(GetStartLocation()) ? game->txQuest[150] : game->txQuest[151];
	else if(str == "zloto")
		return Format("%d", mine_state == State::Shares ? 10000 : 12000);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Mine::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "kopalnia") == 0;
}

//=================================================================================================
bool Quest_Mine::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "udzialy_w_kopalni") == 0)
		return mine_state == State::Shares;
	else if(strcmp(msg, "have_10000") == 0)
		return ctx.pc->unit->gold >= 10000;
	else if(strcmp(msg, "have_12000") == 0)
		return ctx.pc->unit->gold >= 12000;
	else
	{
		assert(0);
		return false;
	}
}

//=================================================================================================
void Quest_Mine::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == Progress::Started && event == LocationEventHandler::CLEARED)
		SetProgress(Progress::ClearedLocation);
}

//=================================================================================================
void Quest_Mine::HandleChestEvent(ChestEventHandler::Event event)
{
	if(prog == Progress::TalkedWithMiner && event == ChestEventHandler::Opened)
		SetProgress(Progress::Finished);
}

//=================================================================================================
void Quest_Mine::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

	f << sub.done;
	f << dungeon_loc;
	f << mine_state;
	f << mine_state2;
	f << mine_state3;
	f << days;
	f << days_required;
	f << days_gold;
	f << messenger;
}

//=================================================================================================
void Quest_Mine::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);

	f >> sub.done;
	f >> dungeon_loc;

	if(LOAD_VERSION >= V_0_4)
	{
		f >> mine_state;
		f >> mine_state2;
		f >> mine_state3;
		f >> days;
		f >> days_required;
		f >> days_gold;
		f >> messenger;
	}

	location_event_handler = this;
	InitSub();
}

//=================================================================================================
void Quest_Mine::LoadOld(HANDLE file)
{
	GameReader f(file);
	int city, cave;

	f >> mine_state;
	f >> mine_state2;
	f >> mine_state3;
	f >> city;
	f >> cave;
	f >> refid;
	f >> days;
	f >> days_required;
	f >> days_gold;
	f >> messenger;
}

//=================================================================================================
void Quest_Mine::InitSub()
{
	if(sub.done)
		return;

	ItemListResult result = FindItemList("ancient_armory_armors");
	result.lis->Get(3, sub.item_to_give);
	sub.item_to_give[3] = FindItem("al_angelskin");
	sub.spawn_item = Quest_Event::Item_InChest;
	sub.target_loc = dungeon_loc;
	sub.at_level = 0;
	sub.chest_event_handler = this;
	next_event = &sub;
}

//=================================================================================================
int Quest_Mine::GetIncome(int days_passed)
{
	if(mine_state == State::Shares && mine_state2 >= State2::Built)
	{
		days_gold += days_passed;
		int count = days_gold / 30;
		if(count)
		{
			days_gold -= count * 30;
			return count * 500;
		}
	}
	else if(mine_state == State::BigShares && mine_state2 >= State2::Expanded)
	{
		days_gold += days_passed;
		int count = days_gold / 30;
		if(count)
		{
			days_gold -= count * 30;
			return count * 1000;
		}
	}
	return 0;
}
