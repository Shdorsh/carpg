#pragma once

class QuestEntry;
class QuestScheme;
struct Location;
struct Unit;

class QuestInstance
{
public:
	QuestScheme* scheme;
	QuestEntry* entry;
	int refid, progress;
	Unit* start_unit;
	Location* start_location;

	int GetProgress() { return progress; }
	void SetProgress(int new_progress);
};
