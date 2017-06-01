#include "Pch.h"
#include "Base.h"
#include "AnyData.h"

// We'll use the generic interface for the factories as we need the engine pointer
static void ScriptAnyFactory_Generic(asIScriptGeneric *gen)
{
	asIScriptEngine *engine = gen->GetEngine();

	*(AnyData**)gen->GetAddressOfReturnLocation() = new AnyData(engine);
}

static AnyData &ScriptAnyAssignment(AnyData *other, AnyData *self)
{
	return *self = *other;
}

void AnyData::Register(asIScriptEngine* engine)
{
	int r;
	r = engine->RegisterObjectType("AnyData", sizeof(AnyData), asOBJ_REF | asOBJ_GC); assert(r >= 0);

	// We'll use the generic interface for the constructor as we need the engine pointer
	r = engine->RegisterObjectBehaviour("AnyData", asBEHAVE_FACTORY, "AnyData@ f()", asFUNCTION(ScriptAnyFactory_Generic), asCALL_GENERIC); assert(r >= 0);

	r = engine->RegisterObjectBehaviour("AnyData", asBEHAVE_ADDREF, "void f()", asMETHOD(AnyData, AddRef), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("AnyData", asBEHAVE_RELEASE, "void f()", asMETHOD(AnyData, Release), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("AnyData", "AnyData& opAssign(AnyData& in)", asFUNCTION(ScriptAnyAssignment), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("AnyData", "void store(const string& in, ?&in)", asMETHODPR(AnyData, Store, (const string&, void*, int), void), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("AnyData", "void store(const string& in, const int64&in)", asMETHODPR(AnyData, Store, (const string&, asINT64&), void), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("AnyData", "void store(const string& in, const double&in)", asMETHODPR(AnyData, Store, (const string&, double&), void), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("AnyData", "bool retrieve(const string& in, ?&out)", asMETHODPR(AnyData, Retrieve, (const string&, void*, int) const, bool), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("AnyData", "bool retrieve(const string& in, int64&out)", asMETHODPR(AnyData, Retrieve, (const string&, asINT64&) const, bool), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("AnyData", "bool retrieve(const string& in, double&out)", asMETHODPR(AnyData, Retrieve, (const string&, double&) const, bool), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("AnyData", "bool exists(const string& in)", asMETHOD(AnyData, Exists), asCALL_THISCALL); assert(r >= 0);

	// Register GC behaviours
	r = engine->RegisterObjectBehaviour("AnyData", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(AnyData, GetRefCount), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("AnyData", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(AnyData, SetFlag), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("AnyData", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(AnyData, GetFlag), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("AnyData", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(AnyData, EnumReferences), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("AnyData", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(AnyData, ReleaseAllHandles), asCALL_THISCALL); assert(r >= 0);
}

AnyData &AnyData::operator=(const AnyData &other)
{
	// Hold on to the object type reference so it isn't destroyed too early
	for(auto& v : other.values)
	{
		auto& value = v.second;
		if((value.typeId & asTYPEID_MASK_OBJECT))
		{
			asITypeInfo *ti = engine->GetTypeInfoById(value.typeId);
			if(ti)
				ti->AddRef();
		}
	}
	
	FreeObjects();

	for(auto& v : other.values)
	{
		auto& value = values[v.first];
		auto& other_value = v.second;

		value.typeId = other_value.typeId;
		if(value.typeId & asTYPEID_OBJHANDLE)
		{
			// For handles, copy the pointer and increment the reference count
			value.valueObj = other_value.valueObj;
			engine->AddRefScriptObject(value.valueObj, engine->GetTypeInfoById(value.typeId));
		}
		else if(value.typeId & asTYPEID_MASK_OBJECT)
		{
			// Create a copy of the object
			value.valueObj = engine->CreateScriptObjectCopy(other_value.valueObj, engine->GetTypeInfoById(value.typeId));
		}
		else
		{
			// Primitives can be copied directly
			value.valueInt = other_value.valueInt;
		}
	}

	return *this;
}

int AnyData::CopyFrom(const AnyData *other)
{
	if(other == 0) return asINVALID_ARG;

	*this = *(AnyData*)other;

	return 0;
}

AnyData::AnyData(asIScriptEngine *engine) : engine(engine), refCount(1), gcFlag(false)
{	
	// Notify the garbage collector of this object
	engine->NotifyGarbageCollectorOfNewObject(this, engine->GetTypeInfoByName("AnyData"));
}

AnyData::~AnyData()
{
	FreeObjects();
}

void AnyData::Store(const string& name, void *ref, cstring type_name)
{
	assert(type_name);
	int typeId = engine->GetTypeIdByDecl(type_name);
	Store(name, ref, typeId);
}

void AnyData::Store(const string& name, void *ref, int refTypeId)
{
	// This method is not expected to be used for primitive types, except for bool, int64, or double
	assert(refTypeId > asTYPEID_DOUBLE || refTypeId == asTYPEID_VOID || refTypeId == asTYPEID_BOOL || refTypeId == asTYPEID_INT64 || refTypeId == asTYPEID_DOUBLE);

	// Hold on to the object type reference so it isn't destroyed too early
	if((refTypeId & asTYPEID_MASK_OBJECT))
	{
		asITypeInfo *ti = engine->GetTypeInfoById(refTypeId);
		if(ti)
			ti->AddRef();
	}

	valueStruct* value_ptr;
	auto it = values.find(name);
	if(it != values.end())
	{
		FreeObject(it->second);
		value_ptr = &it->second;
	}
	else
		value_ptr = &values[name];
	auto& value = *value_ptr;

	value.typeId = refTypeId;
	if(value.typeId & asTYPEID_OBJHANDLE)
	{
		// We're receiving a reference to the handle, so we need to dereference it
		value.valueObj = *(void**)ref;
		engine->AddRefScriptObject(value.valueObj, engine->GetTypeInfoById(value.typeId));
	}
	else if(value.typeId & asTYPEID_MASK_OBJECT)
	{
		// Create a copy of the object
		value.valueObj = engine->CreateScriptObjectCopy(ref, engine->GetTypeInfoById(value.typeId));
	}
	else
	{
		// Primitives can be copied directly
		value.valueInt = 0;

		// Copy the primitive value
		// We receive a pointer to the value.
		int size = engine->GetSizeOfPrimitiveType(value.typeId);
		memcpy(&value.valueInt, ref, size);
	}
}

void AnyData::Store(const string& name, double &ref)
{
	Store(name, &ref, asTYPEID_DOUBLE);
}

void AnyData::Store(const string& name, asINT64 &ref)
{
	Store(name, &ref, asTYPEID_INT64);
}

bool AnyData::Retrieve(const string& name, void *ref, int refTypeId) const
{
	// This method is not expected to be used for primitive types, except for bool, int64, or double
	assert(refTypeId > asTYPEID_DOUBLE || refTypeId == asTYPEID_BOOL || refTypeId == asTYPEID_INT64 || refTypeId == asTYPEID_DOUBLE);

	auto it = values.find(name);
	if(it == values.end())
		return false;

	auto& value = it->second;
	if(refTypeId & asTYPEID_OBJHANDLE)
	{
		// Is the handle type compatible with the stored value?

		// A handle can be retrieved if the stored type is a handle of same or compatible type
		// or if the stored type is an object that implements the interface that the handle refer to.
		if((value.typeId & asTYPEID_MASK_OBJECT))
		{
			// Don't allow the retrieval if the stored handle is to a const object but not the wanted handle
			if((value.typeId & asTYPEID_HANDLETOCONST) && !(refTypeId & asTYPEID_HANDLETOCONST))
				return false;

			// RefCastObject will increment the refCount of the returned pointer if successful
			engine->RefCastObject(value.valueObj, engine->GetTypeInfoById(value.typeId), engine->GetTypeInfoById(refTypeId), reinterpret_cast<void**>(ref));
			if(*(asPWORD*)ref == 0)
				return false;
			return true;
		}
	}
	else if(refTypeId & asTYPEID_MASK_OBJECT)
	{
		// Is the object type compatible with the stored value?

		// Copy the object into the given reference
		if(value.typeId == refTypeId)
		{
			engine->AssignScriptObject(ref, value.valueObj, engine->GetTypeInfoById(value.typeId));
			return true;
		}
	}
	else
	{
		// Is the primitive type compatible with the stored value?

		if(value.typeId == refTypeId)
		{
			int size = engine->GetSizeOfPrimitiveType(refTypeId);
			memcpy(ref, &value.valueInt, size);
			return true;
		}

		// We know all numbers are stored as either int64 or double, since we register overloaded functions for those
		if(value.typeId == asTYPEID_INT64 && refTypeId == asTYPEID_DOUBLE)
		{
			*(double*)ref = double(value.valueInt);
			return true;
		}
		else if(value.typeId == asTYPEID_DOUBLE && refTypeId == asTYPEID_INT64)
		{
			*(asINT64*)ref = asINT64(value.valueFlt);
			return true;
		}
	}

	return false;
}

bool AnyData::Retrieve(const string& name, asINT64 &outValue) const
{
	return Retrieve(name, &outValue, asTYPEID_INT64);
}

bool AnyData::Retrieve(const string& name, double &outValue) const
{
	return Retrieve(name, &outValue, asTYPEID_DOUBLE);
}

bool AnyData::Exists(const string& name) const
{
	auto it = values.find(name);
	return it != values.end();
}

int AnyData::GetTypeId(const string& name) const
{
	auto it = values.find(name);
	if(it != values.end())
		return it->second.typeId;
	else
		return 0;
}

void AnyData::FreeObjects()
{
	for(auto& v : values)
		FreeObject(v.second);
	values.clear();
}

void AnyData::FreeObject(valueStruct& value)
{
	// If it is a handle or a ref counted object, call release
	if(value.typeId & asTYPEID_MASK_OBJECT)
	{
		// Let the engine release the object
		asITypeInfo *ti = engine->GetTypeInfoById(value.typeId);
		engine->ReleaseScriptObject(value.valueObj, ti);

		// Release the object type info
		if(ti)
			ti->Release();

		value.valueObj = 0;
		value.typeId = 0;
	}

	// For primitives, there's nothing to do
}

void AnyData::EnumReferences(asIScriptEngine *inEngine)
{
	for(auto& v : values)
	{
		auto& value = v.second;
		// If we're holding a reference, we'll notify the garbage collector of it
		if(value.valueObj && (value.typeId & asTYPEID_MASK_OBJECT))
		{
			inEngine->GCEnumCallback(value.valueObj);

			// The object type itself is also garbage collected
			asITypeInfo *ti = inEngine->GetTypeInfoById(value.typeId);
			if(ti)
				inEngine->GCEnumCallback(ti);
		}
	}
}

void AnyData::ReleaseAllHandles(asIScriptEngine * /*engine*/)
{
	FreeObjects();
}

int AnyData::AddRef() const
{
	// Increase counter and clear flag set by GC
	gcFlag = false;
	return asAtomicInc(refCount);
}

int AnyData::Release() const
{
	// Decrease the ref counter
	gcFlag = false;
	if(asAtomicDec(refCount) == 0)
	{
		// Delete this object as no more references to it exists
		delete this;
		return 0;
	}

	return refCount;
}

int AnyData::GetRefCount()
{
	return refCount;
}

void AnyData::SetFlag()
{
	gcFlag = true;
}

bool AnyData::GetFlag()
{
	return gcFlag;
}
