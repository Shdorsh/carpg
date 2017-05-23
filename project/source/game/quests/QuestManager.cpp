#include "Pch.h"
#include "Base.h"
#include "BitStreamFunc.h"
#include "Game.h"
#include "GameGui.h"
#include "GameLoader.h"
#include "Journal.h"
#include "QuestManager.h"
#include "QuestEntry.h"
#include "QuestInstance.h"
#include "QuestScheme.h"
#include "SaveState.h"
#include "script/ScriptManager.h"

#include "Quest_Bandits.h"
#include "Quest_BanditsCollectToll.h"
#include "Quest_CampNearCity.h"
#include "Quest_Crazies.h"
#include "Quest_DeliverLetter.h"
#include "Quest_DeliverParcel.h"
#include "Quest_Evil.h"
#include "Quest_FindArtifact.h"
#include "Quest_Goblins.h"
#include "Quest_KillAnimals.h"
#include "Quest_LostArtifact.h"
#include "Quest_Mages.h"
#include "Quest_Main.h"
#include "Quest_Mine.h"
#include "Quest_Orcs.h"
#include "Quest_RescueCaptive.h"
#include "Quest_RetrievePackage.h"
#include "Quest_Sawmill.h"
#include "Quest_SpreadNews.h"
#include "Quest_StolenArtifact.h"
#include "Quest_Wanted.h"

QuestManager Singleton<QuestManager>::instance;

cstring quest_type_str[(int)QuestType::None] = {
	"mayor",
	"captain",
	"random",
	"unique"
};

//=================================================================================================
QuestManager::QuestManager() : forced_quest(nullptr)
{

}

//=================================================================================================
QuestManager::~QuestManager()
{

}

//=================================================================================================
void QuestManager::Init()
{
	AddQuestInfos();
}

//=================================================================================================
Quest* QuestManager::CreateQuest(QUEST quest_id)
{
	switch(quest_id)
	{
	case Q_DELIVER_LETTER:
		return new Quest_DeliverLetter;
	case Q_DELIVER_PARCEL:
		return new Quest_DeliverParcel;
	case Q_SPREAD_NEWS:
		return new Quest_SpreadNews;
	case Q_RETRIEVE_PACKAGE:
		return new Quest_RetrievePackage;
	case Q_RESCUE_CAPTIVE:
		return new Quest_RescueCaptive;
	case Q_BANDITS_COLLECT_TOLL:
		return new Quest_BanditsCollectToll;
	case Q_CAMP_NEAR_CITY:
		return new Quest_CampNearCity;
	case Q_KILL_ANIMALS:
		return new Quest_KillAnimals;
	case Q_FIND_ARTIFACT:
		return new Quest_FindArtifact;
	case Q_LOST_ARTIFACT:
		return new Quest_LostArtifact;
	case Q_STOLEN_ARTIFACT:
		return new Quest_StolenArtifact;
	case Q_SAWMILL:
		return new Quest_Sawmill;
	case Q_MINE:
		return new Quest_Mine;
	case Q_BANDITS:
		return new Quest_Bandits;
	case Q_MAGES:
		return new Quest_Mages;
	case Q_MAGES2:
		return new Quest_Mages2;
	case Q_ORCS:
		return new Quest_Orcs;
	case Q_ORCS2:
		return new Quest_Orcs2;
	case Q_GOBLINS:
		return new Quest_Goblins;
	case Q_EVIL:
		return new Quest_Evil;
	case Q_CRAZIES:
		return new Quest_Crazies;
	case Q_WANTED:
		return new Quest_Wanted;
	case Q_MAIN:
		return new Quest_Main;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
AnyQuestInfo QuestManager::GetRandomQuest(QuestType type)
{
	if(type == QuestType::Unique)
		return nullptr;

	if(forced_quest)
	{
		if(forced_quest->type == QuestType::None)
		{
			if(type == QuestType::Random)
				ERROR("Random quest don't support 'none' yet. Ignoring forced quest.");
			else
				return nullptr;
		}
		if(forced_quest->type == type)
			return forced_quest->quest;
		else
			WARN(Format("Can't force quest '%s' for %s. Types don't match.", forced_quest->id, quest_type_str[(int)type]));
	}

	auto& chance = quest_chance[(int)type];
	if(chance.IsEmpty())
		return nullptr;
	else
		return chance.Get();
}

//=================================================================================================
Quest* QuestManager::StartQuest(AnyQuestInfo quest_info)
{
	assert(quest_info);

	if(quest_info.is_new)
	{
		// TODO
		assert(0);
		return nullptr;
	}
	else
	{
		Quest* quest = CreateQuest(quest_info.quest_id);
		quest->refid = quest_counter++;
		quest->Start();
		unaccepted_quests.push_back(quest);
		return quest;
	}
}

//=================================================================================================
void QuestManager::AddQuestItemRequest(const Item** item, cstring name, int quest_refid, vector<ItemSlot>* items, Unit* unit)
{
	assert(item && name && quest_refid != -1);
	QuestItemRequest* q = new QuestItemRequest;
	q->item = item;
	q->name = name;
	q->quest_refid = quest_refid;
	q->items = items;
	q->unit = unit;
	quest_item_requests.push_back(q);
}

//=================================================================================================
void QuestManager::Reset()
{
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	DeleteElements(quest_item_requests);
	quests_timeout.clear();
	quests_timeout2.clear();
	quest_counter = 0;
	unique_quests_completed = 0;
	unique_completed_show = false;
	quest_rumor_counter = P_MAX;
	for(int i = 0; i < P_MAX; ++i)
		quest_rumor[i] = false;
}

//=================================================================================================
void QuestManager::Cleanup()
{
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	DeleteElements(quest_item_requests);
	DeleteElements(quest_schemes);
}

//=================================================================================================
void QuestManager::Write(BitStream& stream)
{
	stream.WriteCasted<word>(quest_entries.size());
	for(QuestEntry* entry : quest_entries)
	{
		stream.Write(entry->refid);
		stream.WriteCasted<byte>(entry->state);
		WriteString1(stream, entry->title);
		WriteStringArray<byte, word>(stream, entry->msgs);
	}
}

//=================================================================================================
bool QuestManager::Read(BitStream& stream)
{
	const int ENTRY_MIN_SIZE = sizeof(int) + sizeof(byte) * 3;
	word entries_count;
	if(!stream.Read(entries_count)
		|| !EnsureSize(stream, ENTRY_MIN_SIZE * entries_count))
	{
		ERROR("Read world: Broken packet for quest entries.");
		return false;
	}

	quest_entries.resize(entries_count);
	for(int i=0; i<entries_count; ++i)
	{
		QuestEntry* entry = new QuestEntry;
		if(!stream.Read(entry->refid) ||
			!stream.ReadCasted<byte>(entry->state) ||
			!ReadString1(stream, entry->title) ||
			!ReadStringArray<byte, word>(stream, entry->msgs))
		{
			ERROR(Format("Read world: Broken packet for quest entry %d.", i));
			delete entry;
			return false;
		}
		quest_entries[i] = entry;
	}

	return true;
}

//=================================================================================================
void QuestManager::Save(HANDLE file)
{
	FileWriter f(file);

	SaveQuests(f);
	SaveQuestEntries(f);
}

//=================================================================================================
void QuestManager::SaveQuests(FileWriter& f)
{
	f << quests.size();
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
		(*it)->Save(f.file);

	f << unaccepted_quests.size();
	for(vector<Quest*>::iterator it = unaccepted_quests.begin(), end = unaccepted_quests.end(); it != end; ++it)
		(*it)->Save(f.file);

	f << quests_timeout.size();
	for(Quest_Dungeon* q : quests_timeout)
		f << q->refid;

	f << quests_timeout2.size();
	for(Quest* q : quests_timeout2)
		f << q->refid;

	f << quest_counter;
	f << unique_quests_completed;
	f << unique_completed_show;
	f << quest_rumor_counter;
	f << quest_rumor;
}

//=================================================================================================
void QuestManager::SaveQuestEntries(FileWriter& f)
{
	f << quest_entries.size();
	for(QuestEntry* entry : quest_entries)
	{
		f << entry->refid;
		f << entry->state;
		f << entry->title;
		f.WriteStringArray<byte, word>(entry->msgs);
	}
}

//=================================================================================================
void QuestManager::Load(HANDLE file)
{
	FileReader f(file);

	LoadQuests(f);
	if(LOAD_VERSION >= V_0_10)
		LoadQuestEntries(f);
	ConfigureQuests();
	ProcessQuestItemRequests();
	if(LOAD_VERSION < V_0_4)
		LoadOldQuestsData(f);
}

//=================================================================================================
void QuestManager::LoadQuests(FileReader& f)
{
	LoadQuests(f.file, quests);
	LoadQuests(f.file, unaccepted_quests);

	quests_timeout.resize(f.Read<uint>());
	for(Quest_Dungeon*& q : quests_timeout)
		q = (Quest_Dungeon*)FindQuest(f.Read<uint>(), false);

	if(LOAD_VERSION >= V_0_4)
	{
		quests_timeout2.resize(f.Read<uint>());
		for(Quest*& q : quests_timeout2)
			q = FindQuest(f.Read<uint>(), false);
	}
	else
	{
		// old timed units (now removed)
		uint count;
		f >> count;
		f.Skip(sizeof(int) * 3 * count);
	}

	f >> quest_counter;
	f >> unique_quests_completed;
	f >> unique_completed_show;
	f >> quest_rumor_counter;
	f >> quest_rumor;
}

//=================================================================================================
void QuestManager::LoadQuestEntries(FileReader& f)
{
	uint count;
	f >> count;
	for(uint i = 0; i < count; ++i)
	{
		QuestEntry* entry = new QuestEntry;
		entry->index = i;
		f >> entry->refid;
		f >> entry->state;
		f >> entry->title;
		f.ReadStringArray<byte, word>(entry->msgs);
		AnyQuest quest = FindQuest2(entry->refid);
		if(quest)
		{
			if(quest.is_new)
				quest.instance->entry = entry;
			else
				quest.quest->entry = entry;
		}
	}
}

//=================================================================================================
void QuestManager::ConfigureQuests()
{
	Game& game = Game::Get();

	game.quest_sawmill = (Quest_Sawmill*)FindQuestById(Q_SAWMILL);
	game.quest_mine = (Quest_Mine*)FindQuestById(Q_MINE);
	game.quest_bandits = (Quest_Bandits*)FindQuestById(Q_BANDITS);
	game.quest_goblins = (Quest_Goblins*)FindQuestById(Q_GOBLINS);
	game.quest_mages = (Quest_Mages*)FindQuestById(Q_MAGES);
	game.quest_mages2 = (Quest_Mages2*)FindQuestById(Q_MAGES2);
	game.quest_orcs = (Quest_Orcs*)FindQuestById(Q_ORCS);
	game.quest_orcs2 = (Quest_Orcs2*)FindQuestById(Q_ORCS2);
	game.quest_evil = (Quest_Evil*)FindQuestById(Q_EVIL);
	game.quest_crazies = (Quest_Crazies*)FindQuestById(Q_CRAZIES);

	if(!game.quest_mages2)
	{
		game.quest_mages2 = new Quest_Mages2;
		game.quest_mages2->refid = quest_counter++;
		game.quest_mages2->Start();
		unaccepted_quests.push_back(game.quest_mages2);
	}
}

//=================================================================================================
void QuestManager::ProcessQuestItemRequests()
{
	for(vector<QuestItemRequest*>::iterator it = quest_item_requests.begin(), end = quest_item_requests.end(); it != end; ++it)
	{
		QuestItemRequest* qir = *it;
		*qir->item = FindQuestItem(qir->name.c_str(), qir->quest_refid);
		if(qir->items)
		{
			bool ok = true;
			for(vector<ItemSlot>::iterator it2 = qir->items->begin(), end2 = qir->items->end(); it2 != end2; ++it2)
			{
				if(it2->item == QUEST_ITEM_PLACEHOLDER)
				{
					ok = false;
					break;
				}
			}
			if(ok)
			{
				if(LOAD_VERSION < V_0_2_10)
					RemoveNullItems(*qir->items);
				SortItems(*qir->items);
				if(qir->unit && LOAD_VERSION < V_0_2_10)
					qir->unit->RecalculateWeight();
			}
		}
		delete *it;
	}
	quest_item_requests.clear();
}

//=================================================================================================
void QuestManager::LoadOldQuestsData(FileReader& f)
{
	Game& game = Game::Get();

	// load quests old data (now are stored inside quest)
	game.quest_sawmill->LoadOld(f.file);
	game.quest_mine->LoadOld(f.file);
	game.quest_bandits->LoadOld(f.file);
	game.quest_mages2->LoadOld(f.file);
	game.quest_orcs2->LoadOld(f.file);
	game.quest_goblins->LoadOld(f.file);
	game.quest_evil->LoadOld(f.file);
	game.quest_crazies->LoadOld(f.file);
}

//=================================================================================================
void QuestManager::LoadQuests(HANDLE file, vector<Quest*>& quests)
{
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	quests.resize(count);
	for(uint i = 0; i<count; ++i)
	{
		QUEST quest_type;
		ReadFile(file, &quest_type, sizeof(quest_type), &tmp, nullptr);

		Quest* quest = CreateQuest(quest_type);
		quest->quest_id = quest_type;
		if(quest->Load(file))
			quests[i] = quest;
		else
			delete quest;
	}
}

//=================================================================================================
Quest* QuestManager::FindQuest(int refid, bool active)
{
	for(Quest* quest : quests)
	{
		if((!active || quest->IsActive()) && quest->refid == refid)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindQuest(int loc, QuestType type)
{
	for(Quest* quest : quests)
	{
		if(quest->start_loc == loc && quest->type == type)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindQuestById(QUEST quest_id)
{
	for(Quest* quest : quests)
	{
		if(quest->quest_id == quest_id)
			return quest;
	}

	for(Quest* quest : unaccepted_quests)
	{
		if(quest->quest_id == quest_id)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindUnacceptedQuest(int loc, QuestType type)
{
	for(Quest* quest : unaccepted_quests)
	{
		if(quest->start_loc == loc && quest->type == type)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
Quest* QuestManager::FindUnacceptedQuest(int refid)
{
	for(Quest* quest : unaccepted_quests)
	{
		if(quest->refid == refid)
			return quest;
	}

	return nullptr;
}

//=================================================================================================
const Item* QuestManager::FindQuestItem(cstring name, int refid)
{
	for(Quest* quest : quests)
	{
		if(refid == quest->refid)
			return quest->GetQuestItem();
	}

	assert(0);
	return nullptr;
}

//=================================================================================================
void QuestManager::EndUniqueQuest()
{
	++unique_quests_completed;
}

//=================================================================================================
bool QuestManager::RemoveQuestRumor(PLOTKA_QUESTOWA rumor_id)
{
	if(!quest_rumor[rumor_id])
	{
		quest_rumor[rumor_id] = true;
		--quest_rumor_counter;
		return true;
	}
	else
		return false;
}

enum QUEST_EVENT
{
	E_TIMEOUT,
	E_CLEARED_LOCATION
};

struct QuestEvent
{
	QUEST_EVENT type;
};

void SLocation_AddEvent(Location* location, QuestInstance* quest_instance, QUEST_EVENT event_type)
{
	// TODO
	assert(0);
}

void SLocation_RemoveEvent(Location* location, QuestInstance* quest_instance)
{
	// TODO
	assert(0);
}

const string& S_Str(const string& s)
{
	// TODO
	assert(0);
	static string ss;
	return ss;
}

void QuestManager::InitializeScript()
{
	auto& manager = ScriptManager::Get();
	auto engine = manager.GetEngine();

	manager.AddEnum("EVENT", {
		{"E_TIMEOUT", E_TIMEOUT},
		{"E_CLEARED_LOCATION", E_CLEARED_LOCATION}
	});

	manager.AddStruct<QuestEvent>("Event")
		.Member("EVENT type", asOFFSET(QuestEvent, type));

	manager.AddType("QuestInstance")
		.Method("int get_progress()", asMETHOD(QuestInstance, GetProgress))
		.Method("void set_progress(int)", asMETHOD(QuestInstance, SetProgress))
		.Member("Unit@ start_unit", asOFFSET(QuestInstance, start_unit))
		.Member("Location@ start_location", asOFFSET(QuestInstance, start_location));

	manager.ForType("Location")
		.Method("void AddEvent(QuestInstance@, EVENT)", asFUNCTION(SLocation_AddEvent))
		.Method("void RemoveEvent(QuestInstance@)", asFUNCTION(SLocation_RemoveEvent));

	manager.AddFunction("const string& Str(const string& in)", asFUNCTION(S_Str));

	script_code = R"code(
class Quest
{
	QuestInstance@ instance;
	Location@ start_location { get { return instance.start_location; } }
	Unit@ start_unit { get { return instance.start_unit; } }

	void OnStart() {}
	void OnProgress() {}
	void OnEvent(Event e) {}
}

)code";
	
#define REGISTER_API(decl, func) CHECKED(engine->RegisterGlobalFunction(decl, asMETHOD(QuestManager, func), asCALL_THISCALL_ASGLOBAL, this))

	REGISTER_API("void StartQuest(const string& in)", StartQuestEntry);
	REGISTER_API("void AddQuestEntry(const string& in)", AddQuestEntry);
	REGISTER_API("void FinishQuest()", FinishQuest);
	REGISTER_API("void FailQuest()", FailQuest);
	REGISTER_API("void AddQuestReward(uint)", AddQuestReward);
	REGISTER_API("void AddQuestTimeout(uint)", AddQuestTimeout);

#undef REGISTER_API
}

void QuestManager::StartQuestEntry(const string& title)
{
	QuestInstance* quest = GetCurrentQuest();
	if(quest->entry)
		throw ScriptException("QuestEntry already started.");
	quest->entry = new QuestEntry;
	quest->entry->title = title;
	quest->entry->state = QuestEntry::IN_PROGRESS;
	journal_changes = true;
}

void QuestManager::AddQuestEntry(const string& text)
{
	QuestInstance* quest = GetCurrentQuest();
	if(!quest->entry)
		throw ScriptException("QuestEntry missing.");
	quest->entry->msgs.push_back(text);
	journal_changes = true;
}

void QuestManager::FinishQuest()
{
	QuestInstance* quest = GetCurrentQuest();
	if(!quest->entry)
		throw ScriptException("QuestEntry missing.");
	if(quest->entry->state != QuestEntry::FINISHED)
	{
		quest->entry->state = QuestEntry::FINISHED;
		journal_changes = true;
	}
}

void QuestManager::FailQuest()
{
	QuestInstance* quest = GetCurrentQuest();
	if(!quest->entry)
		throw ScriptException("QuestEntry missing.");
	if(quest->entry->state != QuestEntry::FAILED)
	{
		quest->entry->state = QuestEntry::FAILED;
		journal_changes = true;
	}
}

QuestInstance* QuestManager::GetCurrentQuest()
{
	if(current_quest)
		throw ScriptException("Called outside Quest.");
	return current_quest;
}

int QuestManager::AddDialogScript(Tokenizer& t)
{
	char s = t.MustGetSymbol("({");
	const string& code = t.GetBlock(s);
	string* script;
	int* index;
	if(parsed_quest)
	{
		script = &parsed_quest->code;
		index = &quest_script_index;
	}
	else
	{
		script = &script_code;
		index = &script_index;
	}
	*script += Format("void _script_func%d(){%s}\n\n", *index, code.c_str());
	t.Next();
	return (*index)++;
}

int QuestManager::AddDialogIfScript(Tokenizer& t)
{
	char s = t.MustGetSymbol("({");
	const string& code = t.GetBlock(s);
	string* script;
	int* index;
	if(parsed_quest)
	{
		script = &parsed_quest->code;
		index = &quest_if_script_index;
	}
	else
	{
		script = &script_code;
		index = &if_script_index;
	}
	*script += Format("bool _if_script_func%d(){return (%s);}\n\n", *index, code.c_str());
	t.Next();
	return (*index)++;
}

int QuestManager::FindQuestProgress(Tokenizer& t)
{
	if(!parsed_quest)
		t.Throw("Dialog type 'set_progress' can only be used inside quest dialog.");
	auto& progress = parsed_quest->progress;
	if(t.IsInt())
	{
		int val = t.GetInt();
		if(val < 0 || val >= (int)progress.size())
			t.Throw("Invalid progress value '%d' for quest '%s'.", val, parsed_quest->id.c_str());
		return val;
	}
	else if(t.IsItem())
	{
		const string& str = t.GetItem();
		int index = 0;
		for(auto& progress_val : progress)
		{
			if(progress_val == str)
				return index;
			++index;
		}
		t.Throw("Invalid progress value '%s' for quest '%s'.", str.c_str(), parsed_quest->id.c_str());
	}
	else
		t.Expected("quest progress value");
}

void QuestManager::SetParsedQuest(QuestScheme* quest_scheme)
{
	parsed_quest = quest_scheme;
	quest_if_script_index = 0;
	quest_script_index = 0;
}

void QuestManager::BuildQuestScheme()
{
	QuestScheme& quest_scheme = *parsed_quest;
	// namespace
	script_code += Format("namespace _quest_%s {\n", quest_scheme.id.c_str());
	// progress enum
	if(!quest_scheme.progress.empty())
	{
		script_code += "enum Progress {\n";
		bool first = true;
		for(string& s : quest_scheme.progress)
		{
			if(first)
				first = false;
			else
				script_code += ",\n";
			script_code += s;
		}
		script_code += "\n}\n";
	}
	// quest class
	script_code += Format("class %s : Quest{ \n", quest_scheme.id.c_str());
	if(quest_scheme.progress.empty())
		script_code += "int progress { get { return instance.progress; } set { instance.progress = value; } }\n";
	else
		script_code += "Progress progress { get { return Progress(instance.progress); } set { instance.progress = int(value); } }\n";
	script_code += quest_scheme.code;
	script_code += "\n}//quest\n}//namespace\n";
	parsed_quest = nullptr;
}

void QuestManager::BuildScripts()
{
	INFO("Building scripts...");

#ifdef _DEBUG
	CreateDirectory("debug", nullptr);
	core::io::WriteStringToFile("debug/scripts.txt", script_code);
#endif

	// add quest infos
	for(auto scheme : quest_schemes)
		quest_infos.push_back({ scheme->id.c_str(), scheme->type, scheme });

	// build scripts
	auto module = ScriptManager::Get().GetModule();

	module->AddScriptSection("quest script", script_code.c_str());
	int r = module->Build();
	if(r < 0)
		throw Format("Failed to build quest scripts, check log for details (%d).", r);

	INFO("Finished building scripts.");
}

void QuestManager::AddQuestReward(uint gold)
{
	// TODO
	assert(0);
}

void QuestManager::AddQuestTimeout(uint days)
{
	// TODO
	assert(0);
}

QuestEntry* QuestManager::GetQuestEntry(int quest_index)
{
	if(quest_index < 0 || quest_index >= (int)quest_entries.size())
		return nullptr;
	return quest_entries[quest_index];
}

void QuestManager::AddEntry(QuestEntry* entry)
{
	assert(entry);
	entry->index = (int)quest_entries.size();
	quest_entries.push_back(entry);
}

void QuestManager::QuestCallback(QuestInstance* quest, delegate<void()> clbk)
{
	assert(quest);
	current_quest = quest;
	journal_changes = true;
	int old_msg_count = (quest->entry ? (int)quest->entry->msgs.size() : -1);

	clbk();

	if(journal_changes)
	{
		Game& game = Game::Get();
		int msg_count = (quest->entry ? (int)quest->entry->msgs.size() : -1);
		int new_msgs = (msg_count - old_msg_count);
		assert(new_msgs > 0);
		if(old_msg_count != -1 && game.IsOnline())
			game.Net_UpdateQuest(quest->refid, new_msgs);

		game.game_gui->journal->NeedUpdate(Journal::Quests, quest->entry->index);
		game.AddGameMsg3(GMS_JOURNAL_UPDATED);
	}
	current_quest = nullptr;
}

AnyQuest QuestManager::FindQuest2(int refid)
{
	for(auto quest : quests)
	{
		if(quest->refid == refid)
			return quest;
	}

	for(auto quest : unaccepted_quests)
	{
		if(quest->refid == refid)
			return quest;
	}

	for(auto instance : quest_instances)
	{
		if(instance->refid == refid)
			return instance;
	}

	return nullptr;
}

void QuestManager::AddQuestInfos()
{
	quest_infos.push_back({ "bandits", QuestType::Unique, Q_BANDITS });
	quest_infos.push_back({ "crazies", QuestType::Unique, Q_CRAZIES });
	quest_infos.push_back({ "evil", QuestType::Unique, Q_EVIL });
	quest_infos.push_back({ "goblins", QuestType::Unique, Q_GOBLINS });
	quest_infos.push_back({ "mages", QuestType::Unique, Q_MAGES });
	quest_infos.push_back({ "mages2", QuestType::Unique, Q_MAGES2 });
	quest_infos.push_back({ "mine", QuestType::Unique, Q_MINE });
	quest_infos.push_back({ "orcs", QuestType::Unique, Q_ORCS });
	quest_infos.push_back({ "orcs2", QuestType::Unique, Q_ORCS2 });
	quest_infos.push_back({ "sawmill", QuestType::Unique, Q_SAWMILL });

	quest_infos.push_back({ "bandits_collect_toll", QuestType::Captain, Q_BANDITS_COLLECT_TOLL });
	quest_infos.push_back({ "camp_near_city", QuestType::Captain, Q_CAMP_NEAR_CITY });
	quest_infos.push_back({ "rescue_captive", QuestType::Captain, Q_RESCUE_CAPTIVE });
	quest_infos.push_back({ "kill_animals", QuestType::Captain, Q_KILL_ANIMALS });
	quest_infos.push_back({ "wanted", QuestType::Captain, Q_WANTED });

	quest_infos.push_back({ "deliver_letter", QuestType::Mayor, Q_DELIVER_LETTER });
	quest_infos.push_back({ "deliver_parcel", QuestType::Mayor, Q_DELIVER_PARCEL });
	quest_infos.push_back({ "spread_news", QuestType::Mayor, Q_SPREAD_NEWS });
	quest_infos.push_back({ "retrive_package", QuestType::Mayor, Q_RETRIEVE_PACKAGE });

	quest_infos.push_back({ "find_artifact", QuestType::Captain, Q_FIND_ARTIFACT });
	quest_infos.push_back({ "lost_artifact", QuestType::Captain, Q_LOST_ARTIFACT });
	quest_infos.push_back({ "stolen_artifact", QuestType::Captain, Q_STOLEN_ARTIFACT });
}

QuestInfo* QuestManager::FindQuestInfo(const string& id)
{
	for(auto& info : quest_infos)
	{
		if(id == info.id)
			return &info;
	}

	return nullptr;
}

void QuestManager::SetForcedQuest(QuestInfo* quest_info)
{
	forced_quest = quest_info;
}

void QuestManager::LoadRandomQuestInfo()
{
	string path = Format("%s/quests.txt", game_loader.system_dir.c_str());
	Tokenizer t;
	if(!t.FromFile(path.c_str()))
	{
		Error("Failed to load quests from '%s'.", path.c_str());
		game_loader.errors++;
		return;
	}

	t.AddEnums<QuestType>(0, {
		{"mayor", QuestType::Mayor},
		{"captain", QuestType::Captain},
		{"random", QuestType::Random},
		{"unique", QuestType::Unique}
	});
	t.AddKeyword("null", 0, 1);

	try
	{
		while(t.Next())
		{
			QuestType type = (QuestType)t.MustGetKeywordId(0);
			if(type == QuestType::Unique)
				t.Throw("Unique quests can't be used on random quest list.");
			auto& chances = quest_chance[(int)type];
			t.Next();
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				int chance = t.MustGetInt();
				if(chance <= 0)
					t.Throw("Chance must be larger then 0.");
				t.Next();
				if(t.IsKeyword(0, 1)) // null
				{
					if(type == QuestType::Random)
						t.Throw("Null quest for random type is not implemented yet.");
					chances.Add(AnyQuestInfo(nullptr), chance);
				}
				else
				{
					auto& id = t.MustGetString();
					auto quest_info = FindQuestInfo(id);
					if(!quest_info)
						t.Throw("Missing quest '%s'.", id.c_str());
					chances.Add(quest_info->quest, chance);
				}
				t.Next();
			}
		}
	}
	catch(Tokenizer::Exception& e)
	{
		Error("Failed to load quests from '%s': %s", path.c_str(), e.ToString());
		game_loader.errors++;
	}
}

void QuestManager::RemoveQuestEntry(QuestEntry* entry)
{
	assert(entry);
	DeleteElement(quest_entries, entry);
}
