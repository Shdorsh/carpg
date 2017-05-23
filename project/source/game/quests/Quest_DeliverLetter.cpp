#include "Pch.h"
#include "Base.h"
#include "Quest_DeliverLetter.h"
#include "Dialog.h"
#include "Game.h"
#include "LocationHelper.h"
#include "QuestManager.h"

//=================================================================================================
void Quest_DeliverLetter::Start()
{
	start_loc = game->current_location;
	end_loc = game->GetRandomSettlement(start_loc);
	quest_id = Q_DELIVER_LETTER;
	type = QuestType::Mayor;
}

//=================================================================================================
cstring Quest_DeliverLetter::GetDialog(int dialog_type)
{
	switch(dialog_type)
	{
	case QUEST_DIALOG_START:
		return "q_deliver_letter_start";
	case QUEST_DIALOG_FAIL:
		return "q_deliver_letter_timeout";
	case QUEST_DIALOG_NEXT:
		if(prog == Progress::Started)
			return "q_deliver_letter_give";
		else
			return "q_deliver_letter_end";
	default:
		return nullptr;
	}
}

//=================================================================================================
void Quest_DeliverLetter::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog)
	{
	case Progress::Started:
		// give letter to player
		{
			Location& loc = *game->locations[end_loc];
			const Item* base_item = FindItem("letter");
			CreateItemCopy(letter, base_item);
			letter.id = "$letter";
			letter.name = Format(game->txQuest[0], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());
			letter.refid = refid;
			game->current_dialog->pc->unit->AddItem(&letter, 1, true);
			StartQuest(game->txQuest[2]);

			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			quest_manager.quests_timeout2.push_back(this);

			Location& loc2 = *game->locations[start_loc];
			AddEntry(game->txQuest[3], LocationHelper::IsCity(loc2) ? game->txForMayor : game->txForSoltys, loc2.name.c_str(),
				game->day+1, game->month+1, game->year);
			AddEntry(game->txQuest[4], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str(),
				kierunek_nazwa[GetLocationDir(loc2.pos, loc.pos)]);

			if(game->IsOnline())
			{
				game->Net_RegisterItem(&letter, base_item);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, &letter, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::Timeout:
		// player failed to deliver letter in time
		{
			bool removed_item = false;

			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::Failed;
			if(game->current_location == end_loc)
			{
				game->current_dialog->pc->unit->RemoveQuestItem(refid);
				removed_item = true;
			}

			AddEntry(game->txQuest[5]);
			SetState(QuestEntry::FAILED);

			if(game->IsOnline() && removed_item && !game->current_dialog->is_local)
				game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
		}
		break;
	case Progress::GotResponse:
		// given letter, got response
		{
			Location& loc = *game->locations[end_loc];
			letter.name = Format(game->txQuest[1], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());

			AddEntry(game->txQuest[6]);

			if(game->IsOnline())
				game->Net_RenameItem(&letter);
		}
		break;
	case Progress::Finished:
		// given response, end of quest
		{
			game->AddReward(100);

			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::None;
			game->current_dialog->pc->unit->RemoveQuestItem(refid);

			AddEntry(game->txQuest[7]);
			SetState(QuestEntry::FINISHED);
			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);

			if(game->IsOnline() && !game->current_dialog->is_local)
				game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_DeliverLetter::FormatString(const string& str)
{
	Location& loc = *game->locations[end_loc];
	if(str == "target_burmistrza")
		return (LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys);
	else if(str == "target_locname")
		return loc.name.c_str();
	else if(str == "target_dir")
		return kierunek_nazwa[GetLocationDir(game->locations[start_loc]->pos, loc.pos)];
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_DeliverLetter::IsTimedout() const
{
	return game->worldtime - start_time > 30;
}

//=================================================================================================
bool Quest_DeliverLetter::OnTimeout(TimeoutType ttype)
{
	AddEntry(game->txQuest[277]);

	return true;
}

//=================================================================================================
bool Quest_DeliverLetter::IfHaveQuestItem() const
{
	if(prog == Progress::Started)
		return game->current_location == end_loc;
	else
		return game->current_location == start_loc && prog == Progress::GotResponse;
}

//=================================================================================================
const Item* Quest_DeliverLetter::GetQuestItem()
{
	return &letter;
}

//=================================================================================================
void Quest_DeliverLetter::Save(HANDLE file)
{
	Quest::Save(file);

	if(prog < Progress::Finished)
		WriteFile(file, &end_loc, sizeof(end_loc), &tmp, nullptr);
}

//=================================================================================================
bool Quest_DeliverLetter::Load(HANDLE file)
{
	Quest::Load(file);

	if(prog < Progress::Finished)
	{
		ReadFile(file, &end_loc, sizeof(end_loc), &tmp, nullptr);

		Location& loc = *game->locations[end_loc];

		const Item* base_item = FindItem("letter");
		CreateItemCopy(letter, base_item);
		letter.id = "$letter";
		letter.name = Format(game->txQuest[prog == Progress::GotResponse ? 1 : 0], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys,
			loc.name.c_str());
		letter.refid = refid;
		if(game->mp_load)
			game->Net_RegisterItem(&letter, base_item);
	}

	return true;
}
