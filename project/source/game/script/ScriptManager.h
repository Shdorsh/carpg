#pragma once

#ifdef _DEBUG
#	define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#	define CHECKED(x) x
#endif

class ScriptManager : public Singleton<ScriptManager>
{
public:
	ScriptManager();

	void Init();
	void Release();
	bool RunScript(cstring code);
	bool RunIfScript(cstring code);

private:
	asIScriptEngine* engine;
	asIScriptModule* module;
};
