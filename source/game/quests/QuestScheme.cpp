#include "Pch.h"
#include "Base.h"
#include "QuestScheme.h"
#include "TypeVectorContainer.h"

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
	}
};

/*enum class QuestType
{
	Mayor,
	Captain,
	Random,
	Unique
};*/

Type* CreateQuestSchemeHandler()
{
	return new QuestSchemeHandler;
}
