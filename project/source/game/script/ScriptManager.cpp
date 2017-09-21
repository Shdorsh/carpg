#include "Pch.h"
#include "Core.h"
#include "ScriptManager.h"
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>
#include "PlayerController.h"
#include "Unit.h"

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
		Error("ScriptManager: Failed to parse script (%d): %s", r, code);
		return false;
	}

	// run
	auto tmp_context = engine->RequestContext();
	r = tmp_context->Prepare(func);
	if(r >= 0)
		r = tmp_context->Execute();
	func->Release();
	engine->ReturnContext(tmp_context);

	if(r >= 0)
		return true;
	else
	{
		Error("ScriptManager: Failed to run script (%d): %s", r, code);
		return false;
	}
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
		Error("ScriptManager: Failed to parse script (%d): %s", r, code);
		return false;
	}

	// run
	bool ok = false;
	auto tmp_context = engine->RequestContext();
	r = tmp_context->Prepare(func);
	if(r >= 0)
	{
		r = tmp_context->Execute();
		ok = (tmp_context->GetReturnByte() != 0);
	}
	func->Release();
	engine->ReturnContext(tmp_context);

	if(r >= 0)
		return ok;
	else
	{
		Error("ScriptManager: Failed to run script (%d): %s", r, code);
		return false;
	}
}

void ScriptManager::RegisterAllTypes()
{
	AddType("Unit");
	AddType("Player");

	ForType("Unit")
		.Member("Player@ player", offsetof(Unit, player))
		.Member("int gold", offsetof(Unit, gold))
		.Member("Vec3 pos", offsetof(Unit, pos));

	ForType("Player")
		.Member("Unit@ unit", offsetof(PlayerController, unit));

	AddType("ScriptContext")
		.Member("Player@ player", offsetof(ScriptContext, player));

	CHECKED(engine->RegisterGlobalProperty("ScriptContext C", &Ctx));
}
