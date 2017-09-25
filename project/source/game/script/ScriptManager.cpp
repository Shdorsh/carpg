#include "Pch.h"
#include "Core.h"
#include "ScriptManager.h"
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>
#include "PlayerController.h"
#include "Unit.h"
#include "Location.h"
#include "Team.h"

ScriptManager Singleton<ScriptManager>::instance;
void RegisterScriptHelpers();

void MessageCallback(const asSMessageInfo* msg, void* param)
{
	Logger::Level level;
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
	Logger::global->Log(level, Format("(%d:%d) %s", msg->row, msg->col, msg->message));
}

ScriptException::ScriptException(cstring msg)
{
	ScriptManager::Get().last_exception = msg;
}

ScriptManager::ScriptManager() : engine(nullptr), module(nullptr)
{
}

void f_info(const string& s)
{
	Info(s.c_str());
}

void ScriptManager::Init()
{
	Info("Initializing ScriptManager...");

	engine = asCreateScriptEngine();
	module = engine->GetModule("Core", asGM_CREATE_IF_NOT_EXISTS);

	CHECKED(engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL));

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	RegisterStdStringUtils(engine);
	RegisterScriptHelpers();
	RegisterAllTypes();
}

void ScriptManager::Release()
{
	if(engine)
		engine->ShutDownAndRelease();
}

bool ScriptManager::RunScript(cstring code)
{
	assert(code);

	// compile
	auto tmp_module = engine->GetModule("RunScriptModule", asGM_ALWAYS_CREATE);
	cstring packed_code = Format("void f() { %s; }", code);
	asIScriptFunction* func;
	int r = tmp_module->CompileFunction("RunScript", packed_code, -1, 0, &func);
	if(r < 0)
	{
		Error("ScriptManager: Failed to parse script (%d): %.100s", r, code);
		return false;
	}

	// run
	auto tmp_context = engine->RequestContext();
	r = tmp_context->Prepare(func);
	if(r >= 0)
	{
		last_exception = nullptr;
		r = tmp_context->Execute();
	}

	bool finished = (r == asEXECUTION_FINISHED);
	if(!finished)
	{
		if(r == asEXECUTION_EXCEPTION)
		{
			if(!last_exception)
				Error("ScriptManager: Failed to run script, exception thrown \"%s\" at %s(%d): %.100s", tmp_context->GetExceptionString(),
					tmp_context->GetExceptionFunction()->GetName(), tmp_context->GetExceptionLineNumber(), code);
			else
				Error("ScriptManager: Failed to run script, script exception thrown \"%s\" at %s(%d): %.100s", last_exception,
					tmp_context->GetExceptionFunction()->GetName(), tmp_context->GetExceptionLineNumber(), code);
		}
		else
			Error("ScriptManager: Failed to run script (%d): %.100s", r, code);
	}

	func->Release();
	engine->ReturnContext(tmp_context);

	return finished;
}

bool ScriptManager::RunIfScript(cstring code)
{
	assert(code);

	// compile
	auto tmp_module = engine->GetModule("RunScriptModule", asGM_ALWAYS_CREATE);
	cstring packed_code = Format("bool f() { return (%s); }", code);
	asIScriptFunction* func;
	int r = tmp_module->CompileFunction("RunScript", packed_code, -1, 0, &func);
	if(r < 0)
	{
		Error("ScriptManager: Failed to parse if script (%d): %.100s", r, code);
		return false;
	}

	// run
	auto tmp_context = engine->RequestContext();
	r = tmp_context->Prepare(func);
	if(r >= 0)
	{
		last_exception = nullptr;
		r = tmp_context->Execute();
	}

	bool ok;
	if(r != asEXECUTION_FINISHED)
	{
		ok = false;
		if(r == asEXECUTION_EXCEPTION)
		{
			if(!last_exception)
				Error("ScriptManager: Failed to run if script, exception thrown \"%s\" at %s(%d): %.100s", tmp_context->GetExceptionString(),
					tmp_context->GetExceptionFunction()->GetName(), tmp_context->GetExceptionLineNumber(), code);
			else
				Error("ScriptManager: Failed to run if script, script exception thrown \"%s\" at %s(%d): %.100s", last_exception,
					tmp_context->GetExceptionFunction()->GetName(), tmp_context->GetExceptionLineNumber(), code);
		}
		else
			Error("ScriptManager: Failed to run if script (%d): %.100s", r, code);
	}
	else
		ok = (tmp_context->GetReturnByte() != 0);

	func->Release();
	engine->ReturnContext(tmp_context);

	return ok;
}

void ScriptManager::RegisterAllTypes()
{
	// declare types
	AddType("Unit");
	AddType("Player");
	AddType("HeroData");
	AddType("HumanData");
	AddType("Item");

	AddEnum<Class>("CLASS", {
		{ Class::BARBARIAN, "BARBARIAN" },
		{ Class::BARD, "BARD" },
		{ Class::CLERIC, "CLERIC" },
		{ Class::DRUID, "DRUID" },
		{ Class::HUNTER, "HUNTER" },
		{ Class::MAGE, "MAGE" },
		{ Class::MONK, "MONK" },
		{ Class::PALADIN, "PALADIN" },
		{ Class::ROGUE, "ROGUE" },
		{ Class::WARRIOR, "WARRIOR" }
	});

	ForType("Unit")
		.Member("const int gold", offsetof(Unit, gold))
		.Member("const HeroData@ hero", offsetof(Unit, hero))
		.Member("const HumanData@ human", offsetof(Unit, human_data))
		.Member("const int level", offsetof(Unit, level))
		.Member("const Player@ player", offsetof(Unit, player))
		.Member("const Vec3 pos", offsetof(Unit, pos))
		.Method("float GetHpp()", asMETHOD(Unit, GetHpp))
		.Method("uint GiveGold(Unit@, uint)", asMETHOD(Unit, GiveGold));

	ForType("HumanData")
		.Member("const int beard", offsetof(HumanData, beard))
		.Member("const float height", offsetof(HumanData, height))
		.Member("const int hair", offsetof(HumanData, hair))
		.Member("const Vec4 hair_color", offsetof(HumanData, hair_color))
		.Member("const int mustache", offsetof(HumanData, mustache));

	ForType("Player")
		.Member("const CLASS clas", offsetof(PlayerController, clas))
		.Member("const int credit", offsetof(PlayerController, credit))
		.Member("const string name", offsetof(PlayerController, name))
		.Member("const Unit@ unit", offsetof(PlayerController, unit));

	AddEnum("LOCATION_TYPE", {
		{ L_CITY, "L_CITY" },
		{ L_CAVE, "L_CAVE" },
		{ L_CAMP, "L_CAMP" },
		{ L_DUNGEON, "L_DUNGEON" },
		{ L_CRYPT, "L_CRYPT" },
		{ L_FOREST, "L_FOREST" },
		{ L_MOONWELL, "L_MOONWELL" },
		{ L_ENCOUNTER, "L_ENCOUNTER" },
		{ L_ACADEMY, "L_ACADEMY" }
	});

	AddType("Location")
		.Member("const string name", offsetof(Location, name))
		.Member("const LOCATION_TYPE type", offsetof(Location, type));

	AddType("ScriptContext")
		.Member("const Unit@ talker", offsetof(ScriptContext, talker))
		.Member("const Player@ player", offsetof(ScriptContext, player))
		.Member("const Player@ local_player", offsetof(ScriptContext, local_player))
		.Member("const Location@ location", offsetof(ScriptContext, location))
		.WithInstance("ScriptContext C", &Ctx);

	HeroData::Register();
	Team.Register();
}

void ScriptManager::AddEnum(cstring name, std::initializer_list<Enum> const& values)
{
	assert(name && values.size() > 0u);
	CHECKED(engine->RegisterEnum(name));
	for(auto& e : values)
		CHECKED(engine->RegisterEnumValue(name, e.name, e.value));
}
