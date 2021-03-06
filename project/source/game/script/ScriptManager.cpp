#include "Pch.h"
#include "GameCore.h"
#include "ScriptManager.h"
#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>
#include "TypeBuilder.h"
#include "PlayerController.h"
#include "SaveState.h"
#include "Game.h"
#include "ItemHelper.h"
#include "Level.h"
#include "City.h"
#include "InsideLocation.h"
#include "BaseLocation.h"


#ifdef _DEBUG
#	define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#	define CHECKED(x) x
#endif


ScriptManager SM;


ScriptException::ScriptException(cstring msg)
{
	SM.SetException(msg);
}

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
	SM.Log(level, Format("(%d:%d) %s", msg->row, msg->col, msg->message));
}


ScriptManager::ScriptManager() : engine(nullptr), module(nullptr)
{
}

void ScriptManager::InitOnce()
{
	Info("Initializing ScriptManager...");

	engine = asCreateScriptEngine();
	module = engine->GetModule("Core", asGM_CREATE_IF_NOT_EXISTS);

	CHECKED(engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL));

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	RegisterStdStringUtils(engine);
	RegisterCommon();
	RegisterGame();
}

void ScriptManager::Cleanup()
{
	if(engine)
		engine->ShutDownAndRelease();
	DeleteElements(unit_vars);
}

static void FormatStrGeneric(asIScriptGeneric* gen)
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
		/*default:
			{
				part = ToString(gen, adr, type_id);
				s = part.c_str();
			}
			break;*/
		}

		result += s;
		i = pos;
	}

	new(gen->GetAddressOfReturnLocation()) string(result);
}

static void ScriptInfo(const string& str)
{
	SM.Log(Logger::L_INFO, str.c_str());
}
static void ScriptWarn(const string& str)
{
	SM.Log(Logger::L_WARN, str.c_str());
}
static void ScriptError(const string& str)
{
	SM.Log(Logger::L_ERROR, str.c_str());
}

void ScriptManager::RegisterCommon()
{
	AddFunction("void Info(const string& in)", asFUNCTION(ScriptInfo));
	AddFunction("void Warn(const string& in)", asFUNCTION(ScriptWarn));
	AddFunction("void Error(const string& in)", asFUNCTION(ScriptError));

	string func_sign = "string Format(const string& in)";
	for(int i = 1; i <= 9; ++i)
	{
		CHECKED(engine->RegisterGlobalFunction(func_sign.c_str(), asFUNCTION(FormatStrGeneric), asCALL_GENERIC));
		func_sign.pop_back();
		func_sign += ", ?& in)";
	}

	AddFunction("int Random(int, int)", asFUNCTIONPR(Random, (int, int), int));
	AddFunction("int Rand()", asFUNCTIONPR(Rand, (), int));

	/*sm.AddStruct<Int2>("Int2")
		.Constructor<int, int>("void f(int, int)")
		.Constructor<const Int2&>("void f(const Int2& in)")
		.Member("int x", offsetof(Int2, x))
		.Member("int y", offsetof(Int2, y));

	sm.AddStruct<Vec2>("Vec2")
		.Constructor<float, float>("void f(float, float)")
		.Constructor<const Vec2&>("void f(const Vec2& in)")
		.Member("float x", offsetof(Vec2, x))
		.Member("float y", offsetof(Vec2, y));

	sm.AddStruct<Vec3>("Vec3")
		.Constructor<float, float, float>("void f(float, float, float)")
		.Constructor<const Vec3&>("void f(const Vec3& in)")
		.Member("float x", offsetof(Vec3, x))
		.Member("float y", offsetof(Vec3, y))
		.Member("float z", offsetof(Vec3, z));

	sm.AddStruct<Vec4>("Vec4")
		.Constructor<float, float, float, float>("void f(float, float, float, float)")
		.Constructor<const Vec4&>("void f(const Vec4& in)")
		.Member("float x", offsetof(Vec4, x))
		.Member("float y", offsetof(Vec4, y))
		.Member("float z", offsetof(Vec4, z))
		.Member("float w", offsetof(Vec4, w));*/
}

#include "PlayerInfo.h"

VarsContainer globals;
VarsContainer* p_globals = &globals;

VarsContainer* Unit_GetVars(Unit* unit)
{
	return SM.GetVars(unit);
}

const string& Item_GetName(const Item* item)
{
	return item->name;
}

const Item* ItemList_GetByIndex(ItemList* lis, int index)
{
	if(index < 0 || index >= (int)lis->items.size())
		throw ScriptException("Invalid index.");
	return lis->items[index];
}

int ItemList_Size(ItemList* lis)
{
	return lis->items.size();
}

void Unit_RemoveItem(Unit* unit, const string& id)
{
	Item* item = Item::TryGet(id);
	if(item)
		unit->RemoveItem(item, 1u);
}

void Unit_AddItem(Unit* unit, const Item* item)
{
	unit->AddItem2(item, 1u, 0u);
}

void Team_AddGold(uint gold)
{
	Game::Get().AddGold(gold, nullptr, true);
}

bool Level_IsCity()
{
	return L.location->type == L_CITY && ((City*)L.location)->settlement_type == City::SettlementType::City;
}

bool Level_IsVillage()
{
	return L.location->type == L_CITY && ((City*)L.location)->settlement_type == City::SettlementType::Village;
}

bool Level_IsTutorial()
{
	return L.location->type == L_DUNGEON && ((InsideLocation*)L.location)->target == TUTORIAL_FORT;
}

void StockScript_AddItem(const Item* item, uint count)
{
	vector<ItemSlot>* stock = SM.GetContext().stock;
	if(!stock)
		throw ScriptException("This method must be called from StockScript.");
	InsertItemBare(*stock, item, count);
}

void StockScript_AddRandomItem(ITEM_TYPE type, int price_limit, int flags, uint count)
{
	vector<ItemSlot>* stock = SM.GetContext().stock;
	if(!stock)
		throw ScriptException("This method must be called from StockScript.");
	ItemHelper::AddRandomItem(*stock, type, price_limit, flags, count);
}

void ScriptManager::RegisterGame()
{
	// use generic type ??? for get is set

	script_type_infos["bool"] = ScriptTypeInfo(Var::Type::Bool, false);
	script_type_infos["int"] = ScriptTypeInfo(Var::Type::Int, false);
	script_type_infos["float"] = ScriptTypeInfo(Var::Type::Float, false);
	script_type_infos["Item"] = ScriptTypeInfo(Var::Type::Item, true);

	AddType("Var")
		.Method("bool IsNone() const", asMETHOD(Var, IsNone))
		.Method("bool IsBool() const", asMETHODPR(Var, IsBool, () const, bool))
		.Method("bool IsBool(bool) const", asMETHODPR(Var, IsBool, (bool) const, bool))
		.Method("bool IsInt() const", asMETHODPR(Var, IsInt, () const, bool))
		.Method("bool IsInt(int) const", asMETHODPR(Var, IsInt, (int) const, bool))
		.Method("bool IsFloat() const", asMETHODPR(Var, IsFloat, () const, bool))
		.Method("bool IsFloat(float) const", asMETHODPR(Var, IsFloat, (float) const, bool))
		.Method("bool IsVar(Var@) const", asMETHOD(Var, IsVar))
		.Method("bool opEquals(bool) const", asMETHODPR(Var, IsBool, (bool) const, bool))
		.Method("bool opEquals(int) const", asMETHODPR(Var, IsInt, (int) const, bool))
		.Method("bool opEquals(float) const", asMETHODPR(Var, IsFloat, (float) const, bool))
		.Method("bool opEquals(Var@) const", asMETHOD(Var, IsVar))
		.Method("Var@ SetBool(bool)", asMETHOD(Var, SetBool))
		.Method("Var@ SetInt(int)", asMETHOD(Var, SetInt))
		.Method("Var@ SetFloat(float)", asMETHOD(Var, SetFloat))
		.Method("Var@ opAssign(bool)", asMETHOD(Var, SetBool))
		.Method("Var@ opAssign(int)", asMETHOD(Var, SetInt))
		.Method("Var@ opAssign(float)", asMETHOD(Var, SetFloat))
		.Method("Var@ opAssign(Var@)", asMETHOD(Var, SetVar))
		.Method("bool GetBool() const", asMETHOD(Var, GetBool))
		.Method("int GetInt() const", asMETHOD(Var, GetInt))
		.Method("float GetFloat() const", asMETHOD(Var, GetFloat));

	AddType("VarsContainer")
		.Method("Var@ opIndex(const string& in)", asMETHOD(VarsContainer, Get))
		.WithInstance("VarsContainer@ globals", &p_globals);

	AddEnum("ITEM_TYPE", {
		{ "IT_WEAPON", IT_WEAPON },
		{ "IT_BOW", IT_BOW },
		{ "IT_SHIELD", IT_SHIELD },
		{ "IT_ARMOR", IT_ARMOR },
		{ "IT_OTHER", IT_OTHER },
		{ "IT_CONSUMABLE", IT_CONSUMABLE },
		{ "IT_BOOK", IT_BOOK }
		});

	AddEnum("ITEM_FLAGS", {
		{ "ITEM_NOT_SHOP", ITEM_NOT_SHOP },
		{ "ITEM_NOT_MERCHANT", ITEM_NOT_MERCHANT },
		{ "ITEM_NOT_BLACKSMITH", ITEM_NOT_BLACKSMITH },
		{ "ITEM_NOT_ALCHEMIST", ITEM_NOT_ALCHEMIST }
	});

	AddType("Item")
		.Member("const int value", offsetof(Item, value))
		.Method("const string& get_name() const", asFUNCTION(Item_GetName))
		.WithNamespace()
		.AddFunction("Item@ Get(const string& in)", asFUNCTION(Item::GetS))
		.AddFunction("Item@ GetRandom(int)", asFUNCTION(ItemHelper::GetRandomItem));

	AddType("ItemList")
		.Method("Item@ Get()", asMETHODPR(ItemList, Get, () const, const Item*))
		.Method("Item@ Get(int)", asFUNCTION(ItemList_GetByIndex))
		.Method("int Size()", asFUNCTION(ItemList_Size))
		.WithNamespace()
		.AddFunction("ItemList@ Get(const string& in)", asFUNCTION(ItemList::GetS));

	AddType("Unit")
		.Method("int get_gold() const", asMETHOD(Unit, GetGold))
		.Method("void set_gold(int)", asMETHOD(Unit, SetGold))
		.Method("VarsContainer@ get_vars()", asFUNCTION(Unit_GetVars))
		.Method("void AddItem(Item@)", asFUNCTION(Unit_AddItem))
		.Method("void RemoveItem(const string& in)", asFUNCTION(Unit_RemoveItem))
		.WithInstance("Unit@ target", &ctx.target);

	AddType("Player")
		.Member("Unit@ unit", offsetof(PlayerController, unit))
		.WithInstance("Player@ pc", &ctx.pc);

	WithNamespace("Team")
		.AddFunction("void AddGold(uint)", asFUNCTION(Team_AddGold));

	WithNamespace("Level")
		.AddFunction("bool IsCity()", asFUNCTION(Level_IsCity))
		.AddFunction("bool IsVillage()", asFUNCTION(Level_IsVillage))
		.AddFunction("bool IsTutorial()", asFUNCTION(Level_IsTutorial));

	WithNamespace("StockScript")
		.AddFunction("void AddItem(Item@, uint = 1)", asFUNCTION(StockScript_AddItem))
		.AddFunction("void AddRandomItem(ITEM_TYPE, int, int, uint = 1)", asFUNCTION(StockScript_AddRandomItem));
}

void ScriptManager::SetContext(PlayerController* pc, Unit* target)
{
	ctx.pc = pc;
	ctx.target = target;
}

bool ScriptManager::RunScript(cstring code, bool validate)
{
	assert(code);

	// compile
	asIScriptModule* tmp_module = engine->GetModule("RunScriptModule", asGM_ALWAYS_CREATE);
	cstring packed_code = Format("void f() { %s; }", code);
	asIScriptFunction* func;
	int r = tmp_module->CompileFunction("RunScript", packed_code, -1, 0, &func);
	if(r < 0)
	{
		Log(Logger::L_ERROR, Format("Failed to parse script (%d).", r), code);
		return false;
	}

	if(validate)
	{
		func->Release();
		return true;
	}

	// run
	asIScriptContext* tmp_context = engine->RequestContext();
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
			cstring msg = last_exception ? last_exception : tmp_context->GetExceptionString();
			Log(Logger::L_ERROR, Format("Script exception thrown \"%s\" in %s(%d).", msg, tmp_context->GetExceptionFunction()->GetName(),
				tmp_context->GetExceptionFunction()), code);
		}
		else
			Log(Logger::L_ERROR, Format("Script execution failed (%d).", r), code);
	}

	func->Release();
	engine->ReturnContext(tmp_context);

	return finished;
}

bool ScriptManager::RunIfScript(cstring code, bool validate)
{
	assert(code);

	// compile
	asIScriptModule* tmp_module = engine->GetModule("RunScriptModule", asGM_ALWAYS_CREATE);
	cstring packed_code = Format("bool f() { return (%s); }", code);
	asIScriptFunction* func;
	int r = tmp_module->CompileFunction("RunScript", packed_code, -1, 0, &func);
	if(r < 0)
	{
		Log(Logger::L_ERROR, Format("Failed to parse if script (%d).", r), code);
		return false;
	}

	if(validate)
	{
		func->Release();
		return true;
	}

	// run
	asIScriptContext* tmp_context = engine->RequestContext();
	r = tmp_context->Prepare(func);
	if(r >= 0)
	{
		last_exception = nullptr;
		r = tmp_context->Execute();
	}

	bool ok;
	bool finished = (r == asEXECUTION_FINISHED);
	if(!finished)
	{
		ok = false;
		if(r == asEXECUTION_EXCEPTION)
		{
			cstring msg = last_exception ? last_exception : tmp_context->GetExceptionString();
			Log(Logger::L_ERROR, Format("Script exception thrown \"%s\" in %s(%d).", msg, tmp_context->GetExceptionFunction()->GetName(),
				tmp_context->GetExceptionFunction()), code);
		}
		else
			Log(Logger::L_ERROR, Format("Script execution failed (%d).", r), code);
	}
	else
		ok = (tmp_context->GetReturnByte() != 0);

	func->Release();
	engine->ReturnContext(tmp_context);

	return ok;
}

bool ScriptManager::RunStringScript(cstring code, string& str, bool validate)
{
	assert(code);

	// compile
	asIScriptModule* tmp_module = engine->GetModule("RunScriptModule", asGM_ALWAYS_CREATE);
	cstring packed_code = Format("string f() { return (%s); }", code);
	asIScriptFunction* func;
	int r = tmp_module->CompileFunction("RunScript", packed_code, -1, 0, &func);
	if(r < 0)
	{
		Log(Logger::L_ERROR, Format("Failed to parse string script (%d).", r), code);
		return false;
	}

	if(validate)
	{
		func->Release();
		return true;
	}

	// run
	asIScriptContext* tmp_context = engine->RequestContext();
	r = tmp_context->Prepare(func);
	if(r >= 0)
	{
		last_exception = nullptr;
		r = tmp_context->Execute();
	}

	bool ok;
	bool finished = (r == asEXECUTION_FINISHED);
	if(!finished)
	{
		ok = false;
		if(r == asEXECUTION_EXCEPTION)
		{
			cstring msg = last_exception ? last_exception : tmp_context->GetExceptionString();
			Log(Logger::L_ERROR, Format("Script exception thrown \"%s\" in %s(%d).", msg, tmp_context->GetExceptionFunction()->GetName(),
				tmp_context->GetExceptionFunction()), code);
		}
		else
			Log(Logger::L_ERROR, Format("Script execution failed (%d).", r), code);
	}
	else
	{
		void* ptr = tmp_context->GetAddressOfReturnValue();
		str = *(string*)ptr;
		ok = true;
	}

	func->Release();
	engine->ReturnContext(tmp_context);

	return ok;
}

asIScriptFunction* ScriptManager::PrepareScript(cstring name, cstring code)
{
	assert(code);

	// compile
	asIScriptModule* tmp_module = engine->GetModule("RunScriptModule", asGM_ALWAYS_CREATE);
	if(!name)
		name = "f";
	cstring packed_code = Format("void %s() { %s; }", name, code);
	asIScriptFunction* func;
	int r = tmp_module->CompileFunction("RunScript", packed_code, -1, 0, &func);
	if(r < 0)
	{
		Log(Logger::L_ERROR, Format("Failed to prepare script (%d).", r), code);
		return nullptr;
	}

	return func;
}

bool ScriptManager::RunScript(asIScriptFunction* func)
{
	// run
	asIScriptContext* tmp_context = engine->RequestContext();
	int r = tmp_context->Prepare(func);
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
			cstring msg = last_exception ? last_exception : tmp_context->GetExceptionString();
			Log(Logger::L_ERROR, Format("Script exception thrown \"%s\" in %s(%d).", msg, tmp_context->GetExceptionFunction()->GetName(),
				tmp_context->GetExceptionFunction()));
		}
		else
			Log(Logger::L_ERROR, Format("Script execution failed (%d).", r));
	}

	engine->ReturnContext(tmp_context);

	return finished;
}

string& ScriptManager::OpenOutput()
{
	gather_output = true;
	return output;
}

void ScriptManager::CloseOutput()
{
	gather_output = false;
	output.clear();
}

void ScriptManager::Log(Logger::Level level, cstring msg, cstring code)
{
	if(code)
		Logger::global->Log(level, Format("ScriptManager: %s Code:\n%s", msg, code));
	if(gather_output)
	{
		if(!output.empty())
			output += '\n';
		switch(level)
		{
		case Logger::L_INFO:
			output += "INFO:";
			break;
		case Logger::L_WARN:
			output += "WARN:";
			break;
		case Logger::L_ERROR:
			output += "ERROR:";
			break;
		}
		output += msg;
	}
	else
		Logger::global->Log(level, msg);
}

void ScriptManager::AddFunction(cstring decl, const asSFuncPtr& funcPointer)
{
	assert(decl);
	CHECKED(engine->RegisterGlobalFunction(decl, funcPointer, asCALL_CDECL));
}

void ScriptManager::AddEnum(cstring name, std::initializer_list<std::pair<cstring, int>> const& values)
{
	assert(name);
	CHECKED(engine->RegisterEnum(name));
	for(const std::pair<cstring, int>& value : values)
		CHECKED(engine->RegisterEnumValue(name, value.first, value.second));
}

TypeBuilder ScriptManager::AddType(cstring name)
{
	assert(name);
	CHECKED(engine->RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOCOUNT));
	return ForType(name);
}

TypeBuilder ScriptManager::ForType(cstring name)
{
	assert(name);
	TypeBuilder builder(name, engine);
	return builder;
}

NamespaceBuilder ScriptManager::WithNamespace(cstring name, void* auxiliary)
{
	assert(name);
	return NamespaceBuilder(engine, name, auxiliary);
}

VarsContainer* ScriptManager::GetVars(Unit* unit)
{
	assert(unit);
	auto it = unit_vars.lower_bound(unit);
	if(it == unit_vars.end() || it->first != unit)
	{
		VarsContainer* vars = new VarsContainer;
		unit_vars.insert(it, std::unordered_map<Unit*, VarsContainer*>::value_type(unit, vars));
		return vars;
	}
	else
		return it->second;
}

Var& ScriptManager::GetVar(cstring name)
{
	return *globals.Get(name);
}

void ScriptManager::Reset()
{
	globals.Clear();
	DeleteElements(unit_vars);
}

void ScriptManager::Save(FileWriter& f)
{
	// global vars
	globals.Save(f);

	// unit vars
	uint count = 0;
	uint pos = f.BeginPatch(count);
	for(auto& e : unit_vars)
	{
		if(e.second->IsEmpty())
			continue;
		++count;
		f << e.first->refid;
		e.second->Save(f);
	}
	if(count > 0)
		f.Patch(pos, count);
}

void ScriptManager::Load(FileReader& f)
{
	if(LOAD_VERSION < V_0_7)
		return;

	// global vars
	if(LOAD_VERSION >= V_DEV)
		globals.Load(f);

	// unit vars
	uint count;
	f >> count;
	for(uint i = 0; i < count; ++i)
	{
		Unit* unit = Unit::refid_table[f.Read<int>()];
		VarsContainer* vars = new VarsContainer;
		vars->Load(f);
		unit_vars[unit] = vars;
	}
}

ScriptManager::RegisterResult ScriptManager::RegisterGlobalVar(const string& type, bool is_ref, const string& name)
{
	auto it = script_type_infos.find(type);
	if(it == script_type_infos.end() || it->second.require_ref != is_ref)
		return InvalidType;
	Var* var = globals.TryGet(name);
	if(var)
		return AlreadyExists;
	var = globals.Add(it->second.type, name, true);
	var->_int = 0;
	cstring decl = Format("%s%s %s", type.c_str(), is_ref ? "@" : "", name.c_str());
	CHECKED(engine->RegisterGlobalProperty(decl, &var->ptr));
	return Ok;
}
