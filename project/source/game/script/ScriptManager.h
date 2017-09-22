#pragma once

#ifdef _DEBUG
#	define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#	define CHECKED(x) x
#endif

#include "Builder.h"
#include "ScriptContext.h"

struct ScriptException
{
	ScriptException(cstring msg) : msg(msg)
	{
	}

	template<typename... Args>
	ScriptException(cstring msg, const Args&... args) : msg(Format(msg, args...))
	{
	}

	cstring msg;
};

class ScriptManager : public Singleton<ScriptManager>
{
public:
	ScriptManager();

	void Init();
	void Release();
	bool RunScript(cstring code);
	bool RunIfScript(cstring code);

	TypeBuilder ForType(cstring name)
	{
		assert(name);
		TypeBuilder builder(name, engine);
		return builder;
	}

	template<typename T>
	SpecificTypeBuilder<T> ForType(cstring name)
	{
		assert(name);
		SpecificTypeBuilder<T> builder(name, engine);
		return builder;
	}

	TypeBuilder AddType(cstring name)
	{
		assert(name);
		CHECKED(engine->RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOCOUNT));
		return ForType(name);
	}

	template<typename T>
	SpecificTypeBuilder<T> AddStruct(cstring name, bool is_pod = false)
	{
		assert(name);
		int flags = asOBJ_VALUE | asGetTypeTraits<T>();
		if(is_pod)
			flags |= asOBJ_POD;
		CHECKED(engine->RegisterObjectType(name, sizeof(T), flags));
		if(!is_pod)
		{
			CHECKED(engine->RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(internal::Constructor<T>), asCALL_CDECL_OBJFIRST));
			CHECKED(engine->RegisterObjectBehaviour(name, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(internal::Destructor<T>), asCALL_CDECL_OBJFIRST));
		}
		return ForType<T>(name);
	}

	void AddFunction(cstring decl, const asSFuncPtr& funcPointer)
	{
		assert(decl);
		CHECKED(engine->RegisterGlobalFunction(decl, funcPointer, asCALL_CDECL));
	}

	struct Enum
	{
		int value;
		cstring name;
	};

	template<typename T>
	struct EnumClass
	{
		T value;
		cstring name;
	};

	void AddEnum(cstring name, std::initializer_list<Enum> const& values);

	template<typename T>
	void AddEnum(cstring name, std::initializer_list<EnumClass<T>> const& values)
	{
		AddEnum(name, (std::initializer_list<Enum> const&)values);
	}

	ScriptContext Ctx;

	asIScriptEngine* GetEngine() { return engine; }

private:
	void RegisterAllTypes();

	asIScriptEngine* engine;
	asIScriptModule* module;
};
