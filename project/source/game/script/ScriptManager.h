#pragma once

#ifdef _DEBUG
#define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#define CHECKED(x) x
#endif

namespace internal
{
	template<typename E>
	using is_scoped_enum = std::integral_constant<
		bool,
		std::is_enum<E>::value && !std::is_convertible<E, int>::value>;

	template<typename T>
	void Constructor(void* adr)
	{
		new(adr)T();
	}

	template<typename T>
	void Destructor(void* adr)
	{
		((T*)adr)->~T();
	}
}

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

struct TypeBuilder
{
	TypeBuilder(cstring name, asIScriptEngine* engine) : name(name), engine(engine)
	{

	}

	TypeBuilder& Method(cstring decl, const asSFuncPtr& funcPointer)
	{
		assert(decl);
		int flag = (funcPointer.flag == 3 ? asCALL_THISCALL : asCALL_CDECL_OBJFIRST);
		CHECKED(engine->RegisterObjectMethod(name, decl, funcPointer, flag));
		return *this;
	}

	TypeBuilder& Member(cstring decl, int offset)
	{
		assert(decl);
		CHECKED(engine->RegisterObjectProperty(name, decl, offset));
		return *this;
	}

	cstring name;
	asIScriptEngine* engine;
};

class ScriptManager : public Singleton<ScriptManager>
{
public:
	struct EnumToRegister
	{
		cstring name;
		int id;
	};

	template<typename T>
	struct EnumClassToRegister
	{
		cstring name;
		T id;
	};

	ScriptManager();

	void Init();
	void Release();

	// add enum name,id
	void AddEnum(cstring name, std::initializer_list<EnumToRegister> const& items);

	// add enum name,id
	template<typename T>
	void AddEnum(cstring name, std::initializer_list<EnumClassToRegister<T>> const& items)
	{
		AddEnum(name, (std::initializer_list<ScriptManager::EnumToRegister> const&)items);
	}

	TypeBuilder ForType(cstring name)
	{
		assert(name);
		TypeBuilder builder(name, engine);
		return builder;
	}

	TypeBuilder AddType(cstring name)
	{
		assert(name);
		CHECKED(engine->RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOCOUNT));
		return ForType(name);
	}

	template<typename T>
	TypeBuilder AddStruct(cstring name, bool is_pod = false)
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
		return ForType(name);
	}

	void AddFunction(cstring decl, const asSFuncPtr& funcPointer)
	{
		CHECKED(engine->RegisterGlobalFunction(decl, funcPointer, asCALL_CDECL));
	}

	asIScriptEngine* GetEngine() { return engine; }
	asIScriptModule* GetModule() { return module; }

	bool RunScript(cstring code);

private:
	asIScriptEngine* engine;
	asIScriptModule* module;
};
