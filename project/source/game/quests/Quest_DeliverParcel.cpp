#include "Pch.h"
#include "Base.h"
#include "Quest_DeliverParcel.h"
#include "Dialog.h"
#include "Game.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "Encounter.h"

//=================================================================================================
void Quest_DeliverParcel::Start()
{
	start_loc = game->current_location;
	end_loc = game->GetRandomSettlement(start_loc);
	quest_id = Q_DELIVER_PARCEL;
	type = QuestType::Mayor;
}

//=================================================================================================
cstring Quest_DeliverParcel::GetDialog(int dialog_type)
{
	switch(dialog_type)
	{
	case QUEST_DIALOG_START:
		return "q_deliver_parcel_start";
	case QUEST_DIALOG_FAIL:
		return "q_deliver_parcel_timeout";
	case QUEST_DIALOG_NEXT:
		return "q_deliver_parcel_give";
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_DeliverParcel::SetProgress(int prog2)
{
	bool apply = true;

	switch(prog2)
	{
	case Progress::Started:
		// give parcel to player
		{
			Location& loc = *game->locations[end_loc];
			const Item* base_item = content::GetItem("parcel");
			CreateItemCopy(parcel, base_item);
			parcel.id = "$parcel";
			parcel.name = Format(game->txQuest[8], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());
			parcel.refid = refid;
			game->current_dialog->pc->unit->AddItem(&parcel, 1, true);
			StartQuest(game->txQuest[9]);

			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			quest_manager.quests_timeout2.push_back(this);

			Location& loc2 = *game->locations[start_loc];
			AddEntry(game->txQuest[3], LocationHelper::IsCity(loc2) ? game->txForMayor : game->txForSoltys, loc2.name.c_str(),
				game->day+1, game->month+1, game->year);
			AddEntry(game->txQuest[10], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str(),
				GetLocationDirName(loc2.pos, loc.pos));

			if(rand2()%4 != 0)
			{
				Encounter* e = game->AddEncounter(enc);
				e->pos = (loc.pos+loc2.pos)/2;
				e->zasieg = 64;
				e->szansa = 45;
				e->dont_attack = true;
				e->dialog = GameDialogManager::Get().FindDialog("q_deliver_parcel_bandits");
				e->grupa = SG_BANDYCI;
				e->text = game->txQuest[11];
				e->quest = this;
				e->timed = true;
				e->location_event_handler = nullptr;
			}

			if(game->IsOnline())
			{
				game->Net_RegisterItem(&parcel, base_item);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, &parcel, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::DeliverAfterTime:
		// player failed to deliver parcel in time, but gain some gold anyway
		{
			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::Failed;

			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			game->AddReward(125);

			AddEntry(game->txQuest[12]);
			SetState(QuestEntry::FAILED);
			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);

			RemoveEncounter();

			if(game->IsOnline() && !game->current_dialog->is_local)
				game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
		}
		break;
	case Progress::Timeout:
		// player failed to deliver parcel in time
		{
			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::Failed;

			AddEntry(game->txQuest[13]);
			SetState(QuestEntry::FAILED);
			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);

			RemoveEncounter();
		}
		break;
	case Progress::Finished:
		// parcel delivered, end of quest
		{
			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::None;

			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			game->AddReward(250);

			RemoveEncounter();

			AddEntry(game->txQuest[14]);
			SetState(QuestEntry::FINISHED);
			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);

			if(game->IsOnline() && !game->current_dialog->is_local)
				game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
		}
		break;
	case Progress::AttackedBandits:
		// don't giver parcel to bandits, get attacked
		{
			RemoveEncounter();
			apply = false;

			AddEntry(game->txQuest[15]);
		}
		break;
	case Progress::ParcelGivenToBandits:
		// give parcel to bandits
		{
			RemoveEncounter();

			game->current_dialog->talker->AddItem(&parcel, 1, true);
			game->RemoveQuestItem(&parcel, refid);

			AddEntry(game->txQuest[16]);
		}
		break;
	case Progress::NoParcelAttackedBandits:
		// no parcel, attacked by bandits
		apply = false;
		RemoveEncounter();
		break;
	}

	if(apply)
		prog = prog2;
}

//=================================================================================================
cstring Quest_DeliverParcel::FormatString(const string& str)
{
	Location& loc = *game->locations[end_loc];
	if(str == "target_burmistrza")
		return (LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys);
	else if(str == "target_locname")
		return loc.name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, loc.pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_DeliverParcel::IsTimedout() const
{
	return game->worldtime - start_time > 15;
}

//=================================================================================================
bool Quest_DeliverParcel::OnTimeout(TimeoutType ttype)
{
	if(ttype == TIMEOUT2)
		AddEntry(game->txQuest[277]);

	return true;
}

//=================================================================================================
bool Quest_DeliverParcel::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_deliver_parcel_after") == 0)
		return game->worldtime - start_time < 30 && rand2()%2 == 0;
	else
	{
		assert(0);
		return false;
	}
}

//=================================================================================================
bool Quest_DeliverParcel::IfHaveQuestItem() const
{
	if(game->current_location == Game::Get().encounter_loc && prog == Progress::Started)
		return true;
	return game->current_location == end_loc && (prog == Progress::Started || prog == Progress::ParcelGivenToBandits);
}

//=================================================================================================
const Item* Quest_DeliverParcel::GetQuestItem()
{
	return &parcel;
}

//=================================================================================================
void Quest_DeliverParcel::Save(HANDLE file)
{
	Quest_Encounter::Save(file);

	if(prog != Progress::DeliverAfterTime && prog != Progress::Finished)
		WriteFile(file, &end_loc, sizeof(end_loc), &tmp, nullptr);
}

//=================================================================================================
bool Quest_DeliverParcel::Load(HANDLE file)
{
	Quest_Encounter::Load(file);

	if(prog != Progress::DeliverAfterTime && prog != Progress::Finished)
	{
		ReadFile(file, &end_loc, sizeof(end_loc), &tmp, nullptr);

		Location& loc = *game->locations[end_loc];
		const Item* base_item = content::GetItem("parcel");
		CreateItemCopy(parcel, base_item);
		parcel.id = "$parcel";
		parcel.name = Format(game->txQuest[8], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		parcel.refid = refid;
		if(game->mp_load)
			game->Net_RegisterItem(&parcel, base_item);
	}

	if(enc != -1)
	{
		Location& loc = *game->locations[end_loc];
		Location& loc2 = *game->locations[start_loc];
		Encounter* e = game->RecreateEncounter(enc);
		e->pos = (loc.pos+loc2.pos)/2;
		e->zasieg = 64;
		e->szansa = 45;
		e->dont_attack = true;
		e->dialog = GameDialogManager::Get().FindDialog("q_deliver_parcel_bandits");
		e->grupa = SG_BANDYCI;
		e->text = game->txQuest[11];
		e->quest = this;
		e->timed = true;
		e->location_event_handler = nullptr;
	}

	return true;
}
