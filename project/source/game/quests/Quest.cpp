#include "Pch.h"
#include "Base.h"
#include "Quest.h"
#include "Game.h"
#include "SaveState.h"
#include "Journal.h"
#include "QuestManager.h"
#include "GameGui.h"

Game* Quest::game;

//=================================================================================================
Quest::Quest() : quest_manager(QuestManager::Get()), prog(0), timeout(false), entry(nullptr)
{

}

//=================================================================================================
void Quest::Save(HANDLE file)
{
	WriteFile(file, &quest_id, sizeof(quest_id), &tmp, nullptr);
	WriteFile(file, &prog, sizeof(prog), &tmp, nullptr);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	WriteFile(file, &start_time, sizeof(start_time), &tmp, nullptr);
	WriteFile(file, &start_loc, sizeof(start_loc), &tmp, nullptr);
	WriteFile(file, &type, sizeof(type), &tmp, nullptr);
	WriteFile(file, &timeout, sizeof(timeout), &tmp, nullptr);
}

//=================================================================================================
bool Quest::Load(HANDLE file)
{
	// skip quest_id, it was read before calling this function
	//ReadFile(file, &quest_id, sizeof(quest_id), &tmp, nullptr);

	// read old quest journal title/state
	if(LOAD_VERSION <= V_0_10)
	{
		enum class OldQuestState
		{
			Hidden,
			Started,
			Completed,
			Failed
		};

		OldQuestState old_state;
		ReadFile(file, &old_state, sizeof(old_state), &tmp, nullptr);
		if(old_state >= OldQuestState::Hidden)
		{
			entry = new QuestEntry;
			ReadString1(file, entry->title);
			switch(old_state)
			{
			case OldQuestState::Started:
				entry->state = QuestEntry::IN_PROGRESS;
				break;
			case OldQuestState::Completed:
				entry->state = QuestEntry::FINISHED;
				break;
			case OldQuestState::Failed:
				entry->state = QuestEntry::FAILED;
				break;
			}
			quest_manager.AddEntry(entry);
		}
		else
			SkipString1(file);
	}
	
	// read quest data
	ReadFile(file, &prog, sizeof(prog), &tmp, nullptr);
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	ReadFile(file, &start_time, sizeof(start_time), &tmp, nullptr);
	ReadFile(file, &start_loc, sizeof(start_loc), &tmp, nullptr);
	ReadFile(file, &type, sizeof(type), &tmp, nullptr);

	// read old quest msgs
	if(LOAD_VERSION < V_0_10)
	{
		byte count;
		ReadFile(file, &count, sizeof(count), &tmp, nullptr);
		if(entry)
		{
			assert(count > 0);
			entry->refid = refid;
			entry->msgs.resize(count);
			for(byte i = 0; i < count; ++i)
				ReadString2(file, entry->msgs[i]);
		}
		else
		{
			assert(count == 0);
		}
	}

	// quest timeout info
	if(LOAD_VERSION >= V_0_4)
		ReadFile(file, &timeout, sizeof(timeout), &tmp, nullptr);
	else
		timeout = false;

	return true;
}

//=================================================================================================
Location& Quest::GetStartLocation()
{
	return *game->locations[start_loc];
}

//=================================================================================================
const Location& Quest::GetStartLocation() const
{
	return *game->locations[start_loc];
}

//=================================================================================================
cstring Quest::GetStartLocationName() const
{
	return GetStartLocation().name.c_str();
}

//=================================================================================================
GameDialog* Quest::GetGameDialog(int type2)
{
	auto dialog_id = GetDialog(type2);
	assert(dialog_id);
	auto dialog = GameDialogManager::Get().FindDialog(dialog_id);
	return dialog;
}

//=================================================================================================
void Quest_Dungeon::Save(HANDLE file)
{
	Quest::Save(file);

	WriteFile(file, &target_loc, sizeof(target_loc), &tmp, nullptr);
	WriteFile(file, &done, sizeof(done), &tmp, nullptr);
	WriteFile(file, &at_level, sizeof(at_level), &tmp, nullptr);
}

//=================================================================================================
bool Quest_Dungeon::Load(HANDLE file)
{
	Quest::Load(file);

	ReadFile(file, &target_loc, sizeof(target_loc), &tmp, nullptr);
	ReadFile(file, &done, sizeof(done), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_4 || !done)
		ReadFile(file, &at_level, sizeof(at_level), &tmp, nullptr);
	else
		at_level = -1;
	if(LOAD_VERSION < V_0_4 && target_loc != -1)
	{
		Location* loc = game->locations[target_loc];
		if(loc->outside)
			at_level = -1;
	}

	return true;
}

//=================================================================================================
void Quest::QuestHandler(delegate<void()> callback)
{
	int old_msg_count = (entry ? (int)entry->msgs.size() : -1);
	quest_changes = false;

	callback();

	if(quest_changes)
	{
		int msg_count = (entry ? (int)entry->msgs.size() : -1);
		int new_msgs = (msg_count - old_msg_count);
		assert(new_msgs > 0);
		if(old_msg_count != -1 && game->IsOnline())
			game->Net_UpdateQuest(refid, new_msgs);

		game->game_gui->journal->NeedUpdate(Journal::Quests, refid);
		game->AddGameMsg3(GMS_JOURNAL_UPDATED);
	}
}

//=================================================================================================
void Quest::ChangeProgress(int new_prog)
{
	QuestHandler([this, new_prog] { SetProgress(new_prog); });
}

//=================================================================================================
bool Quest::ChangeTimeout(TimeoutType ttype)
{
	bool result;
	QuestHandler([this, ttype, &result] {result = OnTimeout(ttype); });
	return result;
}

//=================================================================================================
void Quest::StartQuest(cstring title)
{
	assert(title);
	assert(!entry);

	entry = new QuestEntry;
	entry->state = QuestEntry::IN_PROGRESS;
	entry->title = title;
	entry->refid = refid;
	quest_manager.AddEntry(entry);

	start_time = game->worldtime;
	quest_changes = true;

	if(game->IsOnline())
		game->Net_AddQuest(refid);
}

//=================================================================================================
void Quest::AddEntry(cstring msg)
{
	assert(msg);
	assert(entry);

	entry->msgs.push_back(msg);
	quest_changes = true;
}

//=================================================================================================
void Quest::SetState(QuestEntry::State state)
{
	assert(entry);

	if(state != entry->state)
	{
		entry->state = state;
		quest_changes = true;
	}
}

//=================================================================================================
Location& Quest_Dungeon::GetTargetLocation()
{
	return *game->locations[target_loc];
}

//=================================================================================================
const Location& Quest_Dungeon::GetTargetLocation() const
{
	return *game->locations[target_loc];
}

//=================================================================================================
cstring Quest_Dungeon::GetTargetLocationName() const
{
	return GetTargetLocation().name.c_str();
}

//=================================================================================================
cstring Quest_Dungeon::GetTargetLocationDir() const
{
	return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
}

//=================================================================================================
Quest_Event* Quest_Dungeon::GetEvent(int current_loc)
{
	Quest_Event* event = this;

	while(event)
	{
		if(event->target_loc == current_loc || event->target_loc == -1)
			return event;
		event = event->next_event;
	}

	return nullptr;
}

//=================================================================================================
void Quest_Encounter::RemoveEncounter()
{
	if(enc == -1)
		return;
	game->RemoveEncounter(enc);
	enc = -1;
}

//=================================================================================================
void Quest_Encounter::Save(HANDLE file)
{
	Quest::Save(file);

	WriteFile(file, &enc, sizeof(enc), &tmp, nullptr);
}

//=================================================================================================
bool Quest_Encounter::Load(HANDLE file)
{
	Quest::Load(file);

	ReadFile(file, &enc, sizeof(enc), &tmp, nullptr);

	return true;
}
