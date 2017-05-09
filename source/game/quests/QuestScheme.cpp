#include "Pch.h"
#include "Base.h"
#include "Dialog.h"
#include "QuestScheme.h"
#include "TypeVectorContainer.h"

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
		auto& id = t.MustGetText();
		auto existing_dialog = quest.FindDialog(id);
		if(existing_dialog)
			t.Throw("Dialog '%s' already exists.", id.c_str());
		Ptr<GameDialog> dialog;
		dialog->id = id;
		t.Next();

		t.AssertSymbol('{');
		t.Next();
	}

	bool Compare(TypeItem* item1, TypeItem* item2, uint offset)
	{

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

	}

	bool Compare(TypeItem* item1, TypeItem* item2, uint offset)
	{

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
			{"Mayor", QuestType::Mayor},
			{"Captain", QuestType::Captain},
			{"Random", QuestType::Random},
			{"Unique", QuestType::Unique}
		}).NotRequired();
		AddItemList("progress", offsetof(QuestScheme, progress))
			.NotRequired();
		AddCustomField("dialog", new QuestSchemeDialogHandler)
			.NotRequired()
			.AllowMultiple();
		AddCustomField("quest", new QuestSchemeCodeHandler)
			.NotRequired()
			.AllowMultiple();
	}
};

Type* CreateQuestSchemeHandler()
{
	return new QuestSchemeHandler;
}
