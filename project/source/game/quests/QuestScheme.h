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
	vector<GameDialog*> dialogs;
	LanguageMap strs;

	~QuestScheme();

	GameDialog* FindDialog(const string& id);
};
