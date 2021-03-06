#pragma once

#include "GameComponent.h"
#include "Var.h"

#ifdef _DEBUG
#	define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#	define CHECKED(x) x
#endif

class asIScriptEngine;
class asIScriptModule;
class TypeBuilder;
class NamespaceBuilder;
struct asSFuncPtr;

struct ScriptException
{
	ScriptException(cstring msg);

	template<typename... Args>
	ScriptException(cstring msg, const Args&... args) : ScriptException(Format(msg, args...)) {}
};

struct ScriptContext
{
	ScriptContext() : pc(nullptr), target(nullptr), stock(nullptr) {}

	PlayerController* pc;
	Unit* target;
	vector<ItemSlot>* stock;
};

class ScriptManager : public GameComponent
{
public:
	ScriptManager();
	void InitOnce() override;
	void Cleanup() override;
	void RegisterCommon();
	void RegisterGame();
	void SetContext(PlayerController* pc, Unit* target);
	void SetException(cstring ex) { last_exception = ex; }
	bool RunScript(cstring code, bool validate = false);
	bool RunIfScript(cstring code, bool validate = false);
	bool RunStringScript(cstring code, string& str, bool validate = false);
	asIScriptFunction* PrepareScript(cstring name, cstring code);
	bool RunScript(asIScriptFunction* func);
	string& OpenOutput();
	void CloseOutput();
	void Log(Logger::Level level, cstring msg, cstring code = nullptr);
	void AddFunction(cstring decl, const asSFuncPtr& funcPointer);
	// add enum with values {name, value}
	void AddEnum(cstring name, std::initializer_list<std::pair<cstring, int>> const& values);
	TypeBuilder AddType(cstring name);
	TypeBuilder ForType(cstring name);
	VarsContainer* GetVars(Unit* unit);
	NamespaceBuilder WithNamespace(cstring name, void* auxiliary = nullptr);
	Var& GetVar(cstring name);
	void Reset();
	void Save(FileWriter& f);
	void Load(FileReader& f);
	enum RegisterResult
	{
		Ok,
		InvalidType,
		AlreadyExists
	};
	RegisterResult RegisterGlobalVar(const string& type, bool is_ref, const string& name);
	ScriptContext& GetContext() { return ctx; }

private:
	struct ScriptTypeInfo
	{
		ScriptTypeInfo() {}
		ScriptTypeInfo(Var::Type type, bool require_ref) : type(type), require_ref(require_ref) {}

		Var::Type type;
		bool require_ref;
	};

	asIScriptEngine* engine;
	asIScriptModule* module;
	string output;
	cstring last_exception;
	bool gather_output;
	std::map<string, ScriptTypeInfo> script_type_infos;
	std::unordered_map<Unit*, VarsContainer*> unit_vars;
	ScriptContext ctx;
};

extern ScriptManager SM;
