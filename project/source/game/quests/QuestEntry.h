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

	string title;
	vector<string> msgs;
	State state;
	int refid, index;
};
