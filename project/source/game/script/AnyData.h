#pragma once

// Angelscript CScriptAny that have multiple fields
class AnyData
{
	// The structure for holding the values
	struct valueStruct
	{
		union
		{
			asINT64 valueInt;
			double  valueFlt;
			void   *valueObj;
		};
		int   typeId;
	};

public:
	// Constructors
	AnyData(asIScriptEngine *engine);

	// Memory management
	int AddRef() const;
	int Release() const;

	// Copy the stored value from another any object
	AnyData &operator=(const AnyData&);
	int CopyFrom(const AnyData *other);

	// Store the value, either as variable type, integer number, or real number
	void Store(const string& name, void* ref, cstring type_name);
	void Store(const string& name, void *ref, int refTypeId);
	template<typename T>
	void Store(const string& name, T& value) = delete;
	void Store(const string& name, asINT64 &value);
	void Store(const string& name, double &value);

	// Retrieve the stored value, either as variable type, integer number, or real number
	bool Retrieve(const string& name, void *ref, int refTypeId) const;
	bool Retrieve(const string& name, asINT64 &value) const;
	bool Retrieve(const string& name, double &value) const;

	// Get the type id of the stored value
	bool Exists(const string& name) const;
	int GetTypeId(const string& name) const;

	// GC methods
	int  GetRefCount();
	void SetFlag();
	bool GetFlag();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseAllHandles(asIScriptEngine *engine);

	static void Register(asIScriptEngine* engine);

protected:
	virtual ~AnyData();
	void FreeObjects();
	void FreeObject(valueStruct& value);

	mutable int refCount;
	mutable bool gcFlag;
	asIScriptEngine* engine;	
	std::map<string, valueStruct> values;
};
