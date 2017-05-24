#include "Pch.h"
#include "Base.h"
#include "Quest_Goblins.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "InsideLocation.h"
#include "GameGui.h"
#include "Team.h"

//=================================================================================================
void Quest_Goblins::Start()
{
	type = QuestType::Unique;
	quest_id = Q_GOBLINS;
	enc = -1;
	goblins_state = State::None;
	nobleman = nullptr;
	messenger = nullptr;
}

//=================================================================================================
cstring Quest_Goblins::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = game->current_dialog->talker->data->id;

	if(id == "q_gobliny_szlachcic")
		return "q_goblins_nobleman";
	else if(id == "q_gobliny_mag")
		return "q_goblins_mage";
	else if(id == "innkeeper")
		return "q_goblins_innkeeper";
	else if(id == "q_gobliny_szlachcic2")
		return "q_goblins_boss";
	else
	{
		assert(id == "q_gobliny_poslaniec");
		return "q_goblins_messenger";
	}
}

//=================================================================================================
bool CzyMajaStaryLuk()
{
	return Team.HaveQuestItem(content::GetItem("q_gobliny_luk"));
}

//=================================================================================================
void DodajStraznikow()
{
	Game& game = Game::Get();
	Unit* u = nullptr;
	UnitData* ud = FindUnitData("q_gobliny_szlachcic2");

	// szukaj szlachcica
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// szukaj tronu
	Useable* use = nullptr;

	for(vector<Useable*>::iterator it = game.local_ctx.useables->begin(), end = game.local_ctx.useables->end(); it != end; ++it)
	{
		if((*it)->type == U_THRONE)
		{
			use = *it;
			break;
		}
	}
	assert(use);

	// przesuñ szlachcica w poblirze tronu
	game.WarpUnit(*u, use->pos);

	// usuñ pozosta³e osoby z pomieszczenia
	InsideLocation* inside = (InsideLocation*)game.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Room* room = lvl.GetNearestRoom(u->pos);
	assert(room);
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data != ud && room->IsInside((*it)->pos))
		{
			(*it)->to_remove = true;
			game.to_remove.push_back(*it);
		}
	}

	// dodaj ochronê
	UnitData* ud2 = FindUnitData("q_gobliny_ochroniarz");
	for(int i=0; i<3; ++i)
	{
		Unit* u2 = game.SpawnUnitInsideRoom(*room, *ud2, 10);
		if(u2)
		{
			u2->dont_attack = true;
			u2->guard_target = u;
		}
	}
	
	// ustaw szlachcica
	u->hero->name = game.txQuest[215];
	u->hero->know_name = true;
	u->ApplyHumanData(game.quest_goblins->hd_nobleman);
}

//=================================================================================================
void Quest_Goblins::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::NotAccepted:
		// nie zaakceptowano
		{
			// dodaj plotkê
			if(quest_manager.RemoveQuestRumor(P_GOBLINY))
			{
				cstring text = Format(game->txQuest[211], game->locations[start_loc]->name.c_str());
				game->rumors.push_back(Format(game->game_gui->journal->txAddNote, game->day+1, game->month+1, game->year, text));
				game->game_gui->journal->NeedUpdate(Journal::Rumors);
				game->AddGameMsg3(GMS_ADDED_RUMOR);
				if(game->IsOnline())
				{
					NetChange& c = Add1(game->net_changes);
					c.type = NetChange::ADD_RUMOR;
					c.id = int(game->rumors.size())-1;
				}
			}
		}
		break;
	case Progress::Started:
		// zaakceptowano
		{
			StartQuest(game->txQuest[212]);
			// usuñ plotkê
			quest_manager.RemoveQuestRumor(P_GOBLINY);
			// dodaj lokalizacje
			target_loc = game->GetNearestLocation2(GetStartLocation().pos, 1<<L_FOREST, true);
			Location& target = GetTargetLocation();
			bool not_known = false;
			if(target.state == LS_UNKNOWN)
			{
				target.state = LS_KNOWN;
				not_known = true;
			}
			target.reset = true;
			target.active_quest = this;
			target.st = 7;
			spawn_item = Quest_Event::Item_OnGround;
			item_to_give[0] = content::GetItem("q_gobliny_luk");
			// questowe rzeczy
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			AddEntry(game->txQuest[217], GetStartLocationName(), game->day+1, game->month+1, game->year);
			AddEntry(game->txQuest[218], GetTargetLocationName(), GetTargetLocationDir());
			// encounter
			Encounter* e = game->AddEncounter(enc);
			e->check_func = CzyMajaStaryLuk;
			e->dialog = GameDialogManager::Get().FindDialog("q_goblins_encounter");
			e->dont_attack = true;
			e->grupa = SG_GOBLINY;
			e->location_event_handler = nullptr;
			e->pos = GetStartLocation().pos;
			e->quest = (Quest_Encounter*)this;
			e->szansa = 10000;
			e->zasieg = 32.f;
			e->text = game->txQuest[219];
			e->timed = false;

			if(game->IsOnline() && not_known)
				game->Net_ChangeLocationState(target_loc, false);
		}
		break;
	case Progress::BowStolen:
		// gobliny ukrad³y ³uk
		{
			game->RemoveQuestItem(content::GetItem("q_gobliny_luk"));
			AddEntry(game->txQuest[220]);
			game->RemoveEncounter(enc);
			enc = -1;
			GetTargetLocation().active_quest = nullptr;
			game->AddNews(game->txQuest[221]);
		}
		break;
	case Progress::TalkedAboutStolenBow:
		// poinformowano o kradzie¿y
		{
			AddEntry(game->txQuest[222]);
			SetState(QuestEntry::FAILED);
			goblins_state = State::Counting;
			days = random(15, 30);
		}
		break;
	case Progress::InfoAboutGoblinBase:
		// pos³aniec dostarczy³ info o bazie goblinów
		{
			target_loc = game->GetRandomSpawnLocation(GetStartLocation().pos, SG_GOBLINY);
			Location& target = GetTargetLocation();
			bool now_known = false;
			if(target.state == LS_UNKNOWN)
			{
				target.state = LS_KNOWN;
				now_known = true;
			}
			target.st = 11;
			target.reset = true;
			target.active_quest = this;
			done = false;
			spawn_item = Quest_Event::Item_GiveSpawned;
			unit_to_spawn = g_spawn_groups[SG_GOBLINY].GetSpawnLeader();
			unit_spawn_level = -3;
			item_to_give[0] = content::GetItem("q_gobliny_luk");
			at_level = target.GetLastLevel();
			AddEntry(game->txQuest[223], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName());
			SetState(QuestEntry::IN_PROGRESS);
			goblins_state = State::MessengerTalked;

			if(game->IsOnline() && now_known)
				game->Net_ChangeLocationState(target_loc, false);
		}
		break;
	case Progress::GivenBow:
		// oddano ³uk
		{
			const Item* item = content::GetItem("q_gobliny_luk");
			game->RemoveItem(*game->current_dialog->pc->unit, item, 1);
			game->current_dialog->talker->AddItem(item, 1, true);
			game->AddReward(500);
			AddEntry(game->txQuest[224]);
			SetState(QuestEntry::FINISHED);
			goblins_state = State::GivenBow;
			GetTargetLocation().active_quest = nullptr;
			target_loc = -1;
			game->AddNews(game->txQuest[225]);
		}
		break;
	case Progress::DidntTalkedAboutBow:
		// nie chcia³eœ powiedzieæ o ³uku
		{
			AddEntry(game->txQuest[226]);
			goblins_state = State::MageTalkedStart;
		}
		break;
	case Progress::TalkedAboutBow:
		// powiedzia³eœ o ³uku
		{
			AddEntry(game->txQuest[227]);
			SetState(QuestEntry::IN_PROGRESS);
			goblins_state = State::MageTalked;
		}
		break;
	case Progress::PayedAndTalkedAboutBow:
		// zap³aci³eœ i powiedzia³eœ o ³uku
		{
			game->current_dialog->pc->unit->gold -= 100;

			AddEntry(game->txQuest[228]);
			SetState(QuestEntry::IN_PROGRESS);
			goblins_state = State::MageTalked;

			if(game->IsOnline() && !game->current_dialog->is_local)
				game->GetPlayerInfo(game->current_dialog->pc).UpdateGold();
		}
		break;
	case Progress::TalkedWithInnkeeper:
		// pogadano z karczmarzem
		{
			goblins_state = State::KnownLocation;
			target_loc = game->CreateLocation(L_DUNGEON, game->world_pos, 128.f, THRONE_FORT, SG_GOBLINY, false);
			Location& target = GetTargetLocation();
			target.st = 13;
			target.state = LS_KNOWN;
			target.active_quest = this;
			done = false;
			unit_to_spawn = FindUnitData("q_gobliny_szlachcic2");
			spawn_unit_room = RoomTarget::Throne;
			callback = DodajStraznikow;
			unit_dont_attack = true;
			unit_auto_talk = true;
			unit_event_handler = this;
			item_to_give[0] = nullptr;
			spawn_item = Quest_Event::Item_DontSpawn;
			at_level = target.GetLastLevel();
			AddEntry(game->txQuest[229], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName());

			if(game->IsOnline())
				game->Net_ChangeLocationState(target_loc, false);
		}
		break;
	case Progress::KilledBoss:
		// zabito szlachcica
		{
			AddEntry(game->txQuest[230]);
			SetState(QuestEntry::FINISHED);
			GetTargetLocation().active_quest = nullptr;
			quest_manager.EndUniqueQuest();
			game->AddNews(game->txQuest[231]);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Goblins::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "start_loc")
		return GetStartLocationName();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Goblins::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "gobliny") == 0;
}

//=================================================================================================
void Quest_Goblins::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::DIE && prog == Progress::TalkedWithInnkeeper)
	{
		SetProgress(Progress::KilledBoss);
		unit->event_handler = nullptr;
	}
}

//=================================================================================================
void Quest_Goblins::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

	f << enc;
	f << goblins_state;
	f << days;
	f << nobleman;
	f << messenger;
	f << hd_nobleman;
}

//=================================================================================================
bool Quest_Goblins::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);

	f >> enc;

	if(LOAD_VERSION >= V_0_4)
	{
		f >> goblins_state;
		f >> days;
		f >> nobleman;
		f >> messenger;
		f >> hd_nobleman;
	}

	if(!done)
	{
		if(prog == Progress::Started)
		{
			spawn_item = Quest_Event::Item_OnGround;
			item_to_give[0] = content::GetItem("q_gobliny_luk");
		}
		else if(prog == Progress::InfoAboutGoblinBase)
		{
			spawn_item = Quest_Event::Item_GiveSpawned;
			unit_to_spawn = g_spawn_groups[SG_GOBLINY].GetSpawnLeader();
			unit_spawn_level = -3;
			item_to_give[0] = content::GetItem("q_gobliny_luk");
		}
		else if(prog == Progress::TalkedWithInnkeeper)
		{
			unit_to_spawn = FindUnitData("q_gobliny_szlachcic2");
			spawn_unit_room = RoomTarget::Throne;
			callback = DodajStraznikow;
			unit_dont_attack = true;
			unit_auto_talk = true;
		}
	}

	unit_event_handler = this;

	if(enc != -1)
	{
		Encounter* e = game->RecreateEncounter(enc);
		e->check_func = CzyMajaStaryLuk;
		e->dialog = GameDialogManager::Get().FindDialog("q_goblins_encounter");
		e->dont_attack = true;
		e->grupa = SG_GOBLINY;
		e->location_event_handler = nullptr;
		e->pos = GetStartLocation().pos;
		e->quest = (Quest_Encounter*)this;
		e->szansa = 10000;
		e->zasieg = 32.f;
		e->text = game->txQuest[219];
		e->timed = false;
	}

	return true;
}

//=================================================================================================
void Quest_Goblins::LoadOld(HANDLE file)
{
	GameReader f(file);
	int old_refid, city;

	f >> goblins_state;
	f >> old_refid;
	f >> city;
	f >> days;
	f >> nobleman;
	f >> messenger;
	f >> hd_nobleman;
}
