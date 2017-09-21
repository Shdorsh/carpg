#include "Pch.h"
#include "Core.h"
#include "ScriptManager.h"

static std::map<int, asIScriptFunction*> tostring_map;

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

static void ScriptInfo(const string& str)
{
	Info(str.c_str());
}
static void ScriptWarn(const string& str)
{
	Warn(str.c_str());
}
static void ScriptError(const string& str)
{
	Error(str.c_str());
}

static void IntToStringCtor(void* mem, int value)
{
	new(mem) string(Format("%d", value));
}
static void FloatToStringCtor(void* mem, float value)
{
	new(mem) string(Format("%g", value));
}

void RegisterScriptHelpers()
{
	ScriptManager& sm = ScriptManager::Get();
	auto engine = sm.GetEngine();

	sm.AddFunction("void Info(const string& in)", asFUNCTION(ScriptInfo));
	sm.AddFunction("void Warn(const string& in)", asFUNCTION(ScriptWarn));
	sm.AddFunction("void Error(const string& in)", asFUNCTION(ScriptError));

	sm.ForType("string")
		.Constructor("void f(int)", asFUNCTION(IntToStringCtor))
		.Constructor("void f(float)", asFUNCTION(FloatToStringCtor));

	string func_sign = "string Format(const string& in)";
	for(int i = 1; i <= 9; ++i)
	{
		func_sign.pop_back();
		func_sign += ", ?& in)";
		CHECKED(engine->RegisterGlobalFunction(func_sign.c_str(), asFUNCTION(FormatStr), asCALL_GENERIC));
	}

	sm.AddStruct<Int2>("Int2")
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
		.Member("float w", offsetof(Vec4, w));
}
