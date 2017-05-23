#pragma once

//-----------------------------------------------------------------------------
#include "QuestConsts.h"

//-----------------------------------------------------------------------------
struct Item;
struct ItemSlot;
struct Quest;
struct Quest_Dungeon;
struct Unit;
class QuestEntry;
class QuestInstance;
class QuestScheme;

//-----------------------------------------------------------------------------
struct QuestItemRequest
{
	const Item** item;
	string name;
	int quest_refid;
	vector<ItemSlot>* items;
	Unit* unit;
};

//-----------------------------------------------------------------------------
struct AnyQuest
{
	union
	{
		Quest* quest;
		QuestInstance* instance;
	};
	bool is_new;

	AnyQuest(nullptr_t) : quest(nullptr), is_new(false) {}
	AnyQuest(Quest* quest) : quest(quest), is_new(false) {}
	AnyQuest(QuestInstance* instance) : instance(instance), is_new(true) {}

	operator bool() const
	{
		return quest != nullptr;
	}
};

//-----------------------------------------------------------------------------
struct AnyQuestInfo
{
	union
	{
		QUEST quest_id;
		QuestScheme* scheme;
	};
	bool is_new;

	AnyQuestInfo(nullptr_t) : quest_id(Q_NONE), is_new(false) {}
	AnyQuestInfo(QUEST quest_id) : quest_id(quest_id), is_new(false) {}
	AnyQuestInfo(QuestScheme* scheme) : scheme(scheme), is_new(true) {}

	operator bool() const
	{
		return is_new || quest_id != Q_NONE;
	}
};

//-----------------------------------------------------------------------------
struct QuestInfo
{
	cstring id;
	QuestType type;
	AnyQuestInfo quest;

	QuestInfo(cstring id, QuestType type, QUEST quest_id) : id(id), type(type), quest(quest_id) {}
	QuestInfo(cstring id, QuestType type, QuestScheme* scheme) : id(id), type(type), quest(scheme) {}
};

//-----------------------------------------------------------------------------
class QuestManager : public Singleton<QuestManager>
{
	friend class QuestSchemeHandler;
public:
	QuestManager();
	~QuestManager();
	void Init();

	Quest* CreateQuest(QUEST quest_id);
	AnyQuestInfo GetRandomQuest(QuestType type);
	Quest* StartQuest(AnyQuestInfo quest_info);
	void AddQuestItemRequest(const Item** item, cstring name, int quest_refid, vector<ItemSlot>* items, Unit* unit = nullptr);
	void Reset();
	void Cleanup();
	void Write(BitStream& stream);
	bool Read(BitStream& stream);
	void Save(HANDLE file);
	void Load(HANDLE file);
	Quest* FindQuest(int location, QuestType type);
	Quest* FindQuest(int refid, bool active = true);
	Quest* FindQuestById(QUEST quest_id);
	Quest* FindUnacceptedQuest(int location, QuestType type);
	Quest* FindUnacceptedQuest(int refid);
	const Item* FindQuestItem(cstring name, int refid);
	void EndUniqueQuest();
	bool RemoveQuestRumor(PLOTKA_QUESTOWA rumor_id);
	void InitializeScript();
	int AddDialogScript(Tokenizer& t);
	int AddDialogIfScript(Tokenizer& t);
	int FindQuestProgress(Tokenizer& t);
	void BuildScripts();
	vector<QuestInfo>& GetQuestInfos() { return quest_infos; }
	QuestInfo* FindQuestInfo(const string& id);
	void SetForcedQuest(QuestInfo* quest_info);
	void LoadRandomQuestInfo();
	void RemoveQuestEntry(QuestEntry* entry);

	vector<Quest*> unaccepted_quests;
	vector<Quest*> quests;
	vector<Quest_Dungeon*> quests_timeout;
	vector<Quest*> quests_timeout2;
	vector<QuestScheme*> quest_schemes;
	int quest_counter;
	int unique_quests_completed;
	bool unique_completed_show;
	int quest_rumor_counter;
	bool quest_rumor[P_MAX];

	void AddEntry(QuestEntry* entry);
	vector<QuestEntry*>& GetQuestEntries() { return quest_entries; }
	QuestEntry* GetQuestEntry(int quest_index);
	AnyQuest FindQuest2(int refid);

private:
	void AddQuestInfos();
	void SaveQuests(FileWriter& f);
	void SaveQuestEntries(FileWriter& f);
	void LoadQuests(FileReader& f);
	void LoadQuests(HANDLE file, vector<Quest*>& quests);
	void LoadQuestEntries(FileReader& f);
	void ConfigureQuests();
	void ProcessQuestItemRequests();
	void LoadOldQuestsData(FileReader& f);

	void StartQuestEntry(const string& title);
	void AddQuestEntry(const string& text);
	void FinishQuest();
	void FailQuest();
	QuestInstance* GetCurrentQuest();
	void SetParsedQuest(QuestScheme* quest_scheme);
	void BuildQuestScheme();
	void AddQuestReward(uint gold);
	void AddQuestTimeout(uint days);
	void QuestCallback(QuestInstance* quest, delegate<void()> clbk);

	WeightData<AnyQuestInfo> quest_chance[3];
	vector<QuestItemRequest*> quest_item_requests;
	vector<QuestEntry*> quest_entries;
	vector<QuestInstance*> quest_instances;
	vector<QuestInfo> quest_infos;
	QuestScheme* parsed_quest;
	QuestInstance* current_quest;
	QuestInfo* forced_quest;
	string script_code;
	int script_index, if_script_index, quest_script_index, quest_if_script_index;
	bool journal_changes;
};
