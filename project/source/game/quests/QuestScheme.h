#pragma once

#include "Language.h"
#include "QuestConsts.h"
#include "TypeItem.h"

struct GameDialog;

class QuestScheme : public TypeItem
{
public:
	string id, code;
	vector<string> progress;
	QuestType type;
	asITypeInfo* script_type;
	asIScriptFunction* script_factory;
	asIScriptFunction* script_upgrade_func;
	vector<GameDialog*> dialogs;
	LanguageMap strs;

	~QuestScheme();

	GameDialog* FindDialog(const string& id);
};
