#pragma once

namespace internal
{
	template<typename E>
	using is_scoped_enum = std::integral_constant<
		bool,
		std::is_enum<E>::value && !std::is_convertible<E, int>::value>;
}

struct ScriptException
{
	ScriptException(cstring msg) {}
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

	void AddEnum(cstring name, std::initializer_list<EnumToRegister> const& items);

	template<typename T>
	void AddEnum(cstring name, std::initializer_list<EnumClassToRegister<T>> const& items)
	{
		AddEnum(name, (std::initializer_list<ScriptManager::EnumToRegister> const&)items);
	}

	asIScriptEngine* GetEngine() { return engine; }

private:
	asIScriptEngine* engine;
	asIScriptModule* module;
};

#ifdef _DEBUG
#define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#define CHECKED(x) x
#endif
