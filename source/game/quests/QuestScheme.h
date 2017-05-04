#pragma once

#include "QuestConsts.h"
#include "TypeItem.h"

class QuestScheme : public TypeItem
{
public:
	string id;
	vector<string> progress;
	QuestType type;
};
