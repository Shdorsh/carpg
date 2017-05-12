#include "Pch.h"
#include "Base.h"
#include "Dialog.h"
#include "QuestManager.h"
#include "QuestScheme.h"
#include "TypeVectorContainer.h"

vector<QuestScheme*> quest_schemes;

GameDialog* QuestScheme::FindDialog(const string& id)
{
	return container::Find(dialogs, [id](GameDialog* dialog) {return dialog->id == id; });
}

class QuestSchemeDialogHandler : public Type::CustomFieldHandler
{
public:
	void SaveText(TextWriter& t, TypeItem* item, uint offset)
	{
		/*auto& quest = *(QuestScheme*)item;
		if(quest.dialogs.empty())
			return;

		for()*/
	}

	void LoadText(Tokenizer& t, TypeItem* item, uint offset)
	{
		auto& quest = item->To<QuestScheme>();
		auto dialog = GameDialogManager::Get().LoadDialog(t);
		auto existing_dialog = quest.FindDialog(dialog->id);
		if(existing_dialog)
		{
			delete dialog;
			t.Throw("Dialog '%s' already exists.", existing_dialog->id.c_str());
		}
		quest.dialogs.push_back(dialog);
	}

	bool Compare(TypeItem* item1, TypeItem* item2, uint offset)
	{
		return true;
	}

	void Copy(TypeItem* from, TypeItem* to, uint offset)
	{

	}
};

class QuestSchemeCodeHandler : public Type::CustomFieldHandler
{
public:
	void SaveText(TextWriter& t, TypeItem* item, uint offset)
	{

	}

	void LoadText(Tokenizer& t, TypeItem* item, uint offset)
	{
		auto& quest = item->To<QuestScheme>();

		const string& code = t.GetBlock('{');
		quest.code += code;
		quest.code += "\n\n";
	}

	bool Compare(TypeItem* item1, TypeItem* item2, uint offset)
	{
		return true;
	}

	void Copy(TypeItem* from, TypeItem* to, uint offset)
	{

	}
};

class QuestSchemeHandler : public TypeImpl<QuestScheme>
{
public:
	QuestSchemeHandler() : TypeImpl(TypeId::QuestScheme, "quest", "Quest", "quests")
	{
		AddId(offsetof(QuestScheme, id));
		AddEnum<QuestType>("type", offsetof(QuestScheme, type), {
			{"mayor", QuestType::Mayor},
			{"captain", QuestType::Captain},
			{"random", QuestType::Random},
			{"unique", QuestType::Unique}
		}).NotRequired();
		AddItemList("progress", offsetof(QuestScheme, progress))
			.NotRequired();
		AddCustomField("dialog", new QuestSchemeDialogHandler)
			.NotRequired()
			.AllowMultiple();
		AddCustomField("code", new QuestSchemeCodeHandler)
			.NotRequired()
			.AllowMultiple();

		container = new TypeVectorContainer(this, QuestManager::Get().quest_schemes);
	}

	void BeforeLoad(TypeItem* item) override
	{
		QuestManager::Get().parsed_quest = (QuestScheme*)item;
	}

	cstring Prepare(TypeItem* item) override
	{
		//auto& quest = item->To<QuestScheme>();
		QuestManager::Get().BuildQuestScheme();
		return nullptr;
	}
};

Type* CreateQuestSchemeHandler()
{
	return new QuestSchemeHandler;
}
