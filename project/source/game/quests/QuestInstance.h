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
	int refid, progress, start_time;
	Unit* start_unit;
	Location* start_location;
	asIScriptObject* script_object;

	QuestInstance(QuestScheme* scheme, int refid);
	int GetProgress() { return progress; }
	void SetProgress(int new_progress);
};
