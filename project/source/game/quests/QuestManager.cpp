#include "Pch.h"
#include "Base.h"
#include "BitStreamFunc.h"
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
Quest* QuestManager::GetMayorQuest(int force)
{
	if(force == -1)
	{
		switch(rand2() % 12)
		{
		case 0:
		case 1:
		case 2:
			return new Quest_DeliverLetter;
		case 3:
		case 4:
		case 5:
			return new Quest_DeliverParcel;
		case 6:
		case 7:
			return new Quest_SpreadNews;
			break;
		case 8:
		case 9:
			return new Quest_RetrievePackage;
		case 10:
		case 11:
		default:
			return nullptr;
		}
	}
	else
	{
		switch(force)
		{
		case 0:
		default:
			return nullptr;
		case 1:
			return new Quest_DeliverLetter;
		case 2:
			return new Quest_DeliverParcel;
		case 3:
			return new Quest_SpreadNews;
		case 4:
			return new Quest_RetrievePackage;
		}
	}
}

//=================================================================================================
Quest* QuestManager::GetCaptainQuest(int force)
{
	if(force == -1)
	{
		switch(rand2() % 11)
		{
		case 0:
		case 1:
			return new Quest_RescueCaptive;
		case 2:
		case 3:
			return new Quest_BanditsCollectToll;
		case 4:
		case 5:
			return new Quest_CampNearCity;
		case 6:
		case 7:
			return new Quest_KillAnimals;
		case 8:
		case 9:
			return new Quest_Wanted;
		case 10:
		default:
			return nullptr;
		}
	}
	else
	{
		switch(force)
		{
		case 0:
		default:
			return nullptr;
		case 1:
			return new Quest_RescueCaptive;
		case 2:
			return new Quest_BanditsCollectToll;
		case 3:
			return new Quest_CampNearCity;
		case 4:
			return new Quest_KillAnimals;
		case 5:
			return new Quest_Wanted;
		}
	}
}

//=================================================================================================
Quest* QuestManager::GetAdventurerQuest(int force)
{
	if(force == -1)
	{
		switch(rand2() % 3)
		{
		case 0:
		default:
			return new Quest_FindArtifact;
		case 1:
			return new Quest_LostArtifact;
		case 2:
			return new Quest_StolenArtifact;
		}
	}
	else
	{
		switch(force)
		{
		case 1:
		default:
			return new Quest_FindArtifact;
		case 2:
			return new Quest_LostArtifact;
		case 3:
			return new Quest_StolenArtifact;
		}
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
}

//=================================================================================================
void QuestManager::Write(BitStream& stream)
{
	stream.WriteCasted<word>(quests.size());
	for(Quest* quest : quests)
	{
		stream.Write(quest->refid);
		stream.WriteCasted<byte>(quest->state);
		WriteString1(stream, quest->name);
		WriteStringArray<byte, word>(stream, quest->msgs);
	}
}

//=================================================================================================
bool QuestManager::Read(BitStream& stream)
{
	const int QUEST_MIN_SIZE = sizeof(int) + sizeof(byte) * 3;
	word quest_count;
	if(!stream.Read(quest_count)
		|| !EnsureSize(stream, QUEST_MIN_SIZE * quest_count))
	{
		ERROR("Read world: Broken packet for quests.");
		return false;
	}
	quests.resize(quest_count);

	int index = 0;
	for(Quest*& quest : quests)
	{
		quest = new PlaceholderQuest;
		quest->quest_index = index;
		if(!stream.Read(quest->refid) ||
			!stream.ReadCasted<byte>(quest->state) ||
			!ReadString1(stream, quest->name) ||
			!ReadStringArray<byte, word>(stream, quest->msgs))
		{
			ERROR(Format("Read world: Broken packet for quest %d.", index));
			return false;
		}
		++index;
	}

	return true;
}

//=================================================================================================
void QuestManager::Save(HANDLE file)
{
	FileWriter f(file);

	f << quests.size();
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
		(*it)->Save(file);

	f << unaccepted_quests.size();
	for(vector<Quest*>::iterator it = unaccepted_quests.begin(), end = unaccepted_quests.end(); it != end; ++it)
		(*it)->Save(file);

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
void QuestManager::Load(HANDLE file)
{
	FileReader f(file);

	LoadQuests(file, quests);
	LoadQuests(file, unaccepted_quests);

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
void QuestManager::LoadQuests(HANDLE file, vector<Quest*>& quests)
{
	DWORD tmp;

	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	quests.resize(count);
	for(uint i = 0; i<count; ++i)
	{
		QUEST quest_type;
		ReadFile(file, &quest_type, sizeof(quest_type), &tmp, nullptr);

		Quest* quest = CreateQuest(quest_type);
		quest->quest_id = quest_type;
		quest->quest_index = i;
		quest->Load(file);
		quests[i] = quest;
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

cstring QuestClass = R"code(
	class _Quest
	{
		protected _QuestInstance@ _instance;

		virtual void OnProgress() {}
	}
)code";

void QuestManager::InitializeScript()
{
	auto engine = ScriptManager::Get().GetEngine();
	
	CHECKED(engine->RegisterObjectType("_QuestInstance", 0, asOBJ_REF | asOBJ_NOCOUNT));
	CHECKED(engine->RegisterObjectMethod("_QuestInstance", "int get_progress()", asMETHOD(QuestInstance, GetProgress), asCALL_THISCALL));
	CHECKED(engine->RegisterObjectMethod("_QuestInstance", "void set_progress(int)", asMETHOD(QuestInstance, SetProgress), asCALL_THISCALL));

	script_code = R"code(
	class _Quest
	{
		protected _QuestInstance@ _instance;

		virtual void OnProgress() {}
	}

)code";

#define REGISTER_API(decl, func) CHECKED(engine->RegisterGlobalFunction(decl, asMETHOD(QuestManager, func), asCALL_THISCALL_ASGLOBAL, this))

	REGISTER_API("void StartQuest(const string& in)", StartQuest);
	REGISTER_API("void AddQuestEntry(const string& in)", AddQuestEntry);
	REGISTER_API("void FinishQuest()", FinishQuest);
	REGISTER_API("void FailQuest()", FailQuest);

#undef REGISTER_API
}

void QuestManager::StartQuest(const string& title)
{
	QuestInstance* quest = GetCurrentQuest();
	if(quest->quest_entry)
		throw ScriptException("QuestEntry already started.");
	quest->quest_entry = new QuestEntry;
	quest->quest_entry->title = title;
	quest->quest_entry->state = QuestEntry::IN_PROGRESS;
	//journal_changes = true;
}

void QuestManager::AddQuestEntry(const string& text)
{
	QuestInstance* quest = GetCurrentQuest();
	if(!quest->quest_entry)
		throw ScriptException("QuestEntry missing.");
	quest->quest_entry->msgs.push_back(text);
	//journal_changes = true;
}

void QuestManager::FinishQuest()
{
	QuestInstance* quest = GetCurrentQuest();
	if(!quest->quest_entry)
		throw ScriptException("QuestEntry missing.");
	if(quest->quest_entry->state != QuestEntry::FINISHED)
	{
		quest->quest_entry->state = QuestEntry::FINISHED;
		//journal_changes = true;
	}
}

void QuestManager::FailQuest()
{
	QuestInstance* quest = GetCurrentQuest();
	if(!quest->quest_entry)
		throw ScriptException("QuestEntry missing.");
	if(quest->quest_entry->state != QuestEntry::FAILED)
	{
		quest->quest_entry->state = QuestEntry::FAILED;
		//journal_changes = true;
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
	script_code += Format("class %s : _Quest {\n", quest_scheme.id.c_str());
	if(quest_scheme.progress.empty())
		script_code += "int progress { get { return _instance.progress; } set { _instance.progress = value; } }\n";
	else
		script_code += "Progress progress { get { return (Progress)_instance.progress; } set { _instance.progress = (int)value; } }\n";
	script_code += quest_scheme.code;
	script_code += "\n}\n";
	parsed_quest = nullptr;
}

void QuestManager::BuildScripts()
{
#ifdef _DEBUG
	CreateDirectory("debug", nullptr);
	core::io::WriteStringToFile("debug/scripts.txt", script_code);
#endif
}
