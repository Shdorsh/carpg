#include "Pch.h"
#include "Base.h"
#include "QuestManager.h"
#include "script/ScriptManager.h"

#ifdef _DEBUG
#define CHECK(x) {int _result = (x); assert(_result >= 0);}
#else
#define CHECK(x) x
#endif

ScriptManagerInstance ScriptManager;

ScriptManager::ScriptManager() : engine(nullptr)
{

}

void ScriptManager::Init()
{
	engine = asCreateScriptEngine();
	module = engine->GetModule("Core", asGM_CREATE_IF_NOT_EXISTS);

	QuestManager::Get().InitializeScript();
}

void ScriptManager::Release()
{
	if(engine)
		engine->ShutDownAndRelease();
}

void ScriptManager::AddEnum(cstring name, std::initializer_list<EnumToRegister> const& items)
{
	assert(name);
	CHECK(engine->RegisterEnum(name));
	for(const EnumToRegister& item : items)
		CHECK(engine->RegisterEnumValue(name, item.name, item.id));

	module->AddScriptSection("Quest", "class Quest{}");
	module->Build();
	auto questType = module->GetTypeInfoByName("Quest");

	uint typeCount = module->GetObjectTypeCount();
	for(uint i = 0; i < typeCount; ++i)
	{
		auto type = module->GetObjectTypeByIndex(i);
		auto baseType = type->GetBaseType();
		if(baseType == questType)
		{
			// is quest
			//type->
		}
	}
}
