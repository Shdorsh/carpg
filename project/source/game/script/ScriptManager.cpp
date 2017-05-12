#include "Pch.h"
#include "Base.h"
#include "QuestManager.h"
#include "script/ScriptManager.h"
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>

void MessageCallback(const asSMessageInfo* msg, void* param)
{
	Logger::LOG_LEVEL level;
	switch(msg->type)
	{
	case asMSGTYPE_ERROR:
		level = Logger::L_ERROR;
		break;
	case asMSGTYPE_WARNING:
		level = Logger::L_WARN;
		break;
	case asMSGTYPE_INFORMATION:
	default:
		level = Logger::L_INFO;
		break;
	}
	logger->Log(Format("(%d:%d) %s", msg->row, msg->col, msg->message), level);
}

ScriptManager Singleton<ScriptManager>::instance;

ScriptManager::ScriptManager() : engine(nullptr)
{

}

void ScriptManager::Init()
{
	engine = asCreateScriptEngine();
	module = engine->GetModule("Core", asGM_CREATE_IF_NOT_EXISTS);

	CHECKED(engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL));	

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	RegisterStdStringUtils(engine);

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
	CHECKED(engine->RegisterEnum(name));
	for(const EnumToRegister& item : items)
		CHECKED(engine->RegisterEnumValue(name, item.name, item.id));
}
