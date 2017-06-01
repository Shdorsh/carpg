#include "Pch.h"
#include "Base.h"
#include "QuestManager.h"
#include "script/AnyData.h"
#include "script/ScriptManager.h"
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>
#include "Game.h"

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

std::map<int, asIScriptFunction*> tostring_map;

asIScriptFunction* FindToString(asIScriptEngine* engine, int type_id)
{
	// find mapped type
	auto it = tostring_map.find(type_id);
	if(it != tostring_map.end())
		return it->second;

	// get type
	auto type = engine->GetTypeInfoById(type_id);
	assert(type);

	// special case - cast to string
	auto name = type->GetName();
	if(strcmp(name, "string") == 0)
	{
		tostring_map[type_id] = nullptr;
		return nullptr;
	}

	// find function
	auto func = type->GetMethodByDecl("string ToString() const");
	if(!func)
		func = type->GetMethodByDecl("string ToString()");
	if(!func)
		throw ScriptException("Missing ToString method for object '%s'.", name);

	// add mapping
	tostring_map[type_id] = func;
	return func;
}

string ToString(asIScriptGeneric* gen, void* adr, int type_id)
{
	auto engine = gen->GetEngine();
	auto func = FindToString(engine, type_id);

	if(!func)
	{
		// cast to string
		string& value = **(string**)adr;
		return value;
	}

	// call function
	auto obj = *(asIScriptObject**)adr;
	auto ctx = engine->RequestContext();
	int r = ctx->Prepare(func);
	if(r >= 0)
	{
		r = ctx->SetObject(obj);
		if(r >= 0)
			r = ctx->Execute();
	}
	if(r < 0)
	{
		auto type = engine->GetTypeInfoById(type_id);
		auto name = type->GetName();
		throw ScriptException("Failed to call ToString on object '%s' (%d).", name, r);
	}
	void* ret_adr = ctx->GetReturnAddress();
	string& ret_str = **(string**)ret_adr;
	return ret_str;
}

void FormatStr(asIScriptGeneric* gen)
{
	int count = gen->GetArgCount();

	string result, part;
	string& fmt = **(string**)gen->GetAddressOfArg(0);

	for(uint i = 0, len = fmt.length(); i < len; ++i)
	{
		char c = fmt[i];
		if(c != '{')
		{
			result += c;
			continue;
		}
		if(++i == len)
			throw ScriptException("Broken format string, { at end of string.");
		c = fmt[i];
		if(c == '{')
		{
			result += 'c';
			continue;
		}
		uint pos = fmt.find_first_of('}', i);
		if(pos == string::npos)
			throw ScriptException("Broken format string, missing closing }.");
		part = fmt.substr(i, pos - i);
		int index;
		if(!TextHelper::ToInt(part.c_str(), index))
			throw ScriptException("Broken format string, invalid index '%s'.", part.c_str());
		++index;
		if(index < 1 || index >= count)
			throw ScriptException("Broken format string, invalid index %d.", index - 1);
		auto type_id = gen->GetArgTypeId(index);
		void* adr = gen->GetAddressOfArg(index);
		cstring s = nullptr;
		switch(type_id)
		{
		case asTYPEID_BOOL:
			{
				bool value = **(bool**)adr;
				s = (value ? "true" : "false");
			}
			break;
		case asTYPEID_INT8:
			{
				char value = **(char**)adr;
				s = Format("%d", (int)value);
			}
			break;
		case asTYPEID_INT16:
			{
				short value = **(short**)adr;
				s = Format("%d", (int)value);
			}
			break;
		case asTYPEID_INT32:
			{
				int value = **(int**)adr;
				s = Format("%d", value);
			}
			break;
		case asTYPEID_INT64:
			{
				int64 value = **(int64**)adr;
				s = Format("%I64d", value);
			}
			break;
		case asTYPEID_UINT8:
			{
				byte value = **(byte**)adr;
				s = Format("%u", (uint)value);
			}
			break;
		case asTYPEID_UINT16:
			{
				word value = **(word**)adr;
				s = Format("%u", (uint)value);
			}
			break;
		case asTYPEID_UINT32:
			{
				uint value = **(uint**)adr;
				s = Format("%u", value);
			}
			break;
		case asTYPEID_UINT64:
			{
				uint64 value = **(uint64**)adr;
				s = Format("%I64u", value);
			}
			break;
		case asTYPEID_FLOAT:
			{
				float value = **(float**)adr;
				s = Format("%g", value);
			}
			break;
		case asTYPEID_DOUBLE:
			{
				double value = **(double**)adr;
				s = Format("%g", value);
			}
			break;
		default:
			{
				part = ToString(gen, adr, type_id);
				s = part.c_str();
			}
			break;
		}

		result += s;
		i = pos;
	}

	new(gen->GetAddressOfReturnLocation()) string(result);
}

void ScriptManager::Init()
{
	engine = asCreateScriptEngine();
	module = engine->GetModule("Core", asGM_CREATE_IF_NOT_EXISTS);

	CHECKED(engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL));	
	
	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	RegisterStdStringUtils(engine);
	AnyData::Register(engine);
	
	string func_sign = "string Format(const string& in)";
	for(int i = 1; i <= 9; ++i)
	{
		func_sign.pop_back();
		func_sign += ", ?& in)";
		CHECKED(engine->RegisterGlobalFunction(func_sign.c_str(), asFUNCTION(FormatStr), asCALL_GENERIC));
	}

	AddFunction("int rand()", asFUNCTIONPR(rand2, (), int));
	
	Game::Get().InitializeScript();
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

asIScriptObject* ScriptManager::CreateInstance(asIScriptFunction* factory)
{
	assert(factory);

	asIScriptObject* obj = nullptr;

	auto tmp_context = engine->RequestContext();
	int r = tmp_context->Prepare(factory);
	if(r >= 0)
		r = tmp_context->Execute();
	if(r >= 0)
	{
		obj = *(asIScriptObject**)tmp_context->GetAddressOfReturnValue();
		obj->AddRef();
	}
	engine->ReturnContext(tmp_context);

	if(r < 0)
		Error("ScriptManager: Failed to create instance '%s' (%d)", factory->GetDeclaration(), r);
	
	return obj;
}
