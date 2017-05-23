#pragma once

#include "Chest.h"
#include "Location.h"
#include "Item.h"
#include "Unit.h"
#include "Mapa2.h"
#include "QuestConsts.h"
#include "QuestEntry.h"

//-----------------------------------------------------------------------------
#define QUEST_DIALOG_START 0
#define QUEST_DIALOG_FAIL 1
#define QUEST_DIALOG_NEXT 2

//-----------------------------------------------------------------------------
struct DialogEntry;
struct Game;
struct GameDialog;
struct Item;
class QuestManager;

//-----------------------------------------------------------------------------
enum TimeoutType
{
	TIMEOUT_NORMAL,
	TIMEOUT_CAMP,
	TIMEOUT2,
	TIMEOUT_ENCOUNTER
};

//-----------------------------------------------------------------------------
struct Quest
{
	QuestManager& quest_manager;
	QUEST quest_id;
	QuestEntry* entry;
	int prog, refid, start_time, start_loc;
	QuestType type;
	bool timeout, quest_changes;
	static Game* game;

	Quest();
	virtual ~Quest() {}

private:
	// call ChangeProgress to change quest progress
	virtual void SetProgress(int prog2) = 0;
	// call ChangeTimeout
	// called on quest timeout, return true if timeout handled (if false it will be called on next time update)
	virtual bool OnTimeout(TimeoutType ttype) { return true; }
	void QuestHandler(delegate<void()> callback);

public:
	virtual void Start() = 0;
	virtual cstring GetDialog(int type2) = 0;
	virtual cstring FormatString(const string& str) = 0;
	virtual bool IsTimedout() const { return false; }	
	virtual bool IfHaveQuestItem() const { return false; }
	virtual bool IfHaveQuestItem2(cstring id) const { return false; }
	virtual bool IfNeedTalk(cstring topic) const { return false; }
	virtual bool IfQuestEvent() const { return false; }
	virtual void Special(DialogContext& ctx, cstring msg) {}
	virtual bool IfSpecial(DialogContext& ctx, cstring msg) { return false; }
	virtual const Item* GetQuestItem() { return nullptr; }

	virtual void Save(HANDLE file);
	virtual bool Load(HANDLE file);

	// to powinno by� inline ale nie wysz�o :/
	Location& GetStartLocation();
	const Location& GetStartLocation() const;
	cstring GetStartLocationName() const;
	bool IsActive() const { return !entry || entry->state == QuestEntry::IN_PROGRESS; };
	GameDialog* GetGameDialog(int type2);

	void ChangeProgress(int new_prog);
	bool ChangeTimeout(TimeoutType ttype);
	void StartQuest(cstring title);
	void AddEntry(cstring msg);
	void SetState(QuestEntry::State state);

	template<typename... Args>
	void AddEntry(cstring msg, const Args&... args)
	{
		AddEntry(Format(msg, args...));
	}
};

//-----------------------------------------------------------------------------
struct Quest_Encounter : public Quest
{
	int enc;

	Quest_Encounter() : enc(-1)
	{

	}

	void Save(HANDLE file) override;
	bool Load(HANDLE file) override;
	void RemoveEncounter();
};

typedef void(*VoidFunc)();

//-----------------------------------------------------------------------------
// aktualnie next_event i item_to_give2 dzia�a tylko w podziemiach
// w mie�cie dzia�a tylko unit_to_spawn, unit_dont_attack, unit_auto_talk, unit_spawn_level, unit_event_handler i send_spawn_event (spawnuje w karczmie)
struct Quest_Event
{
	enum SpawnItem
	{
		Item_DontSpawn,
		Item_GiveStrongest,
		Item_GiveSpawned,
		Item_InTreasure, // tylko labirynt i krypta!
		Item_OnGround,
		Item_InChest, // tylko podziemia
		Item_GiveSpawned2 // tylko podziemia
	};

	static const int MAX_ITEMS = 4;

	SpawnItem spawn_item;
	const Item* item_to_give[MAX_ITEMS]; // wiele przedmiot�w obs�ugiwane tylko w Item_InChest
	int target_loc, at_level;
	bool done;
	LocationEventHandler* location_event_handler;
	Quest_Event* next_event;
	ChestEventHandler* chest_event_handler;
	bool whole_location_event_handler; // czy location_event_handler jest dla wszystkich poziom�w czy tylko okre�lonego
	VoidDelegate callback;

	// unit
	UnitData* unit_to_spawn, *unit_to_spawn2; // second unit only in inside location
	bool unit_dont_attack, unit_auto_talk, send_spawn_event;
	bool spawn_2_guard_1; // only works in inside location
	UnitEventHandler* unit_event_handler;
	int unit_spawn_level, unit_spawn_level2; // >=0-vlue, -1-minimum, -2-random(min,max), -3-random(dungeon units level)
	RoomTarget spawn_unit_room; // room to spawn unit, only works in inside location
	RoomTarget spawn_unit_room2; // room to spawn second unit, only works in inside location

	Quest_Event() : done(false), item_to_give(), at_level(-1), spawn_item(Item_DontSpawn), unit_to_spawn(nullptr), unit_dont_attack(false),
		location_event_handler(nullptr), target_loc(-1), next_event(nullptr), chest_event_handler(nullptr), unit_event_handler(nullptr), unit_auto_talk(false),
		whole_location_event_handler(false), spawn_unit_room(RoomTarget::None), callback(nullptr), unit_to_spawn2(nullptr), send_spawn_event(false),
		unit_spawn_level(-2), unit_spawn_level2(-2), spawn_2_guard_1(false), spawn_unit_room2(RoomTarget::None)
	{

	}
};

//-----------------------------------------------------------------------------
struct Quest_Dungeon : public Quest, public Quest_Event
{
	void Save(HANDLE file) override;
	bool Load(HANDLE file) override;

	// to powinno by� inline ale nie wysz�o :/
	Location& GetTargetLocation();
	const Location& GetTargetLocation() const;
	cstring GetTargetLocationName() const;
	cstring GetTargetLocationDir() const;

	Quest_Event* GetEvent(int current_loc);
};
