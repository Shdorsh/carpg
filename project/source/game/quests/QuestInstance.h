#pragma once

class QuestEntry;
class QuestScheme;

class QuestInstance
{
public:
	QuestScheme* quest;
	QuestEntry* quest_entry;
	int progress;

	int GetProgress() { return progress; }
	void SetProgress(int new_progress);
};
