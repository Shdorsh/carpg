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

class ScriptManager
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

private:
	asIScriptEngine* engine;
	asIScriptModule* module;
};

typedef ScriptManager ScriptManagerInstance;

extern ScriptManagerInstance ScriptManager;
