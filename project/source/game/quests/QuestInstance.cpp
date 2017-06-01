#include "Pch.h"
#include "Base.h"
#include "QuestInstance.h"

QuestInstance::QuestInstance(QuestScheme* scheme, int refid) : scheme(scheme), refid(refid), progress(0), entry(nullptr), start_unit(nullptr),
start_location(nullptr)
{
	assert(scheme && refid != -1);
}

void QuestInstance::SetProgress(int new_progress)
{
	progress = new_progress;
}
