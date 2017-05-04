#pragma once

struct Quest;
class QuestInstance;

class QuestEntry
{
public:
	enum State
	{
		IN_PROGRESS,
		FINISHED,
		FAILED
	};

	QuestInstance* instance;
	string title;
	vector<string> msgs;
	State state;
};
