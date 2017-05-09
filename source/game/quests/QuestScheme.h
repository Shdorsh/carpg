#pragma once

#include "QuestConsts.h"
#include "TypeItem.h"

struct GameDialog;

class QuestScheme : public TypeItem
{
public:
	string id;
	vector<string> progress;
	QuestType type;
	vector<GameDialog*> dialogs;

	GameDialog* FindDialog(const string& id);
};
