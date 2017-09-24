#include "utils.h"
#include "../../../add_on/scriptdictionary/scriptdictionary.h"
#include "../../../add_on/scriptmath/scriptmathcomplex.h"
#include "../../../add_on/scripthandle/scripthandle.h"


namespace Test_Addon_Dictionary
{

const char *script =
"void Test()                       \n"
"{                                 \n"
"  dictionary dict;                \n"
// Test integer with the dictionary
"  dict.set('a', 42);            \n"
"  assert(dict.exists('a'));     \n"
"  uint u = 0;                     \n"
"  dict.get('a', u);             \n"
"  assert(u == 42);                \n"
"  dict.delete('a');             \n"
"  assert(!dict.exists('a'));    \n"
// Test array by handle
"  array<string> a = {'t'};               \n"
"  dict.set('a', @a);            \n"
"  array<string> @b;                      \n"
"  dict.get('a', @b);            \n"
"  assert(b == a);             \n"
// Test string by value
"  dict.set('a', 't');             \n"
"  string c;                       \n"
"  dict.get('a', c);             \n"
"  assert(c == 't');             \n"
// Test int8 with the dictionary
"  int8 q = 41;                    \n"
"  dict.set('b', q);             \n"
"  dict.get('b', q);             \n"
"  assert(q == 41);                \n"
// Test float with the dictionary
"  float f = 300;                  \n"
"  dict.set('c', f);             \n"
"  dict.get('c', f);             \n"
"  assert(f == 300);               \n"
// Test automatic conversion between int and float in the dictionary
"  int i;                          \n"
"  dict.get('c', i);             \n"
"  assert(i == 300);               \n"
"  dict.get('b', f);             \n"
"  assert(f == 41);                \n"
// Test booleans with the variable type
"  bool bl;                        \n"
"  dict.set('true', true);       \n"
"  dict.set('false', false);     \n"
"  bl = false;                     \n"
"  dict.get('true', bl);         \n"
"  assert( bl == true );           \n"
"  dict.get('false', bl);        \n"
"  assert( bl == false );          \n"
// Test circular reference with itself
"  dict.set('self', @dict);      \n"
// Test the keys
"  array<string> @keys = dict.getKeys(); \n"
"  assert( keys.find('a') != -1 ); \n"
"  assert( keys.length == 6 ); \n"
"}                                 \n";

// Test circular reference including a script class and the dictionary
const char *script2 = 
"class C { dictionary dict; }                \n"
"void f() { C c; c.dict.set(\"self\", @c); } \n"; 

void Print(const std::string &i)
{
	PRINTF("%s", i.c_str());
}

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;
	asIScriptEngine *engine = 0;
	asIScriptContext *ctx;
	asIScriptModule *mod;

	// Another test for bool in dictionary
	// Temporary variable was incorrectly being reused.
	// Reported by Aaron Baker
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		RegisterScriptDictionary(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(Print), asCALL_CDECL);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class foo \n"
			"{ \n"
			"	bool bar; \n"
			"	foo() \n"
			"	{ \n"
			"		bar = false; \n"
			"	} \n"
			"} \n"
			"foo[] foos(98); \n"
			"foo@ get_foo(int idx) \n"
			"{ \n"
			"	return @foos[idx]; \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"	dictionary d; \n"
			"	d.set('foos.bar', foos[96].bar); \n"
			"	string num = '96'; \n"
			"	d.get('foos.bar', get_foo(digitfunc(num)).bar); \n" // the bug was in this expression
			"	alert('Value of foos[96].bar', 'Value should be false, but is ' + foos[96].bar); \n"
			"} \n"
			"int digitfunc(string str) \n"
			"{ \n"
			"	// In the real world usecase, this would convert the string to a \n"
			"	// number, but for simplicities sake, we just return 96. \n"
			"		return 96; \n"
			"} \n"
			"void alert(const string &in a, const string &in b) \n"
			"{ \n"
//			"	print(b);\n"
			"	assert(b == 'Value should be false, but is false'); \n"
			"} \n");

		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if (r != asEXECUTION_FINISHED)
		{
			TEST_FAILED;
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("%s", GetExceptionInfo(ctx, true).c_str());
		}
		ctx->Release();

		engine->ShutDownAndRelease();
	}

	// Test bool in dictionary
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void main() \n"
			"{ \n"
			"    dictionary dict; \n"
			"    dict['true'] = true; \n"
			"    dict['false'] = false; \n"
			"    assert( bool(dict['true']) == true ); \n"
			"    assert( int(dict['true']) == 1 ); \n"
			"    assert( bool(dict['false']) == false ); \n"
			"    assert( int(dict['false']) == 0 ); \n"
			"    dict['value'] = 42; \n"
			"    assert( bool(dict['value']) == true ); \n"
			"    dict['value'] = 0; \n"
			"    assert( bool(dict['value']) == false ); \n"
			"    @dict['handle'] = dict; \n"
			"    assert( bool(dict['handle']) == true ); \n"
			"    @dict['handle'] = null; \n"
			"    assert( bool(dict['handle']) == false ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->ShutDownAndRelease();
	}

	// Storing a ref in the dictionary
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);
		RegisterScriptHandle(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class T {} \n"
			"void main() \n"
			"{ \n"
			"    dictionary dict; \n"
			"    ref @t = T(); \n"
			"    @dict['test'] = t; \n"
			"    T @obj = cast<T>(dict['test']); \n"
			"    assert( obj !is null ); \n"
			"    assert( obj is t ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		
		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PRINTF("%s\n", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
 		ctx->Release();

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Must use ShutDownAndRelease, or else manually call GC, because the dictionary with the script 
		// handle was placed on the GC, and these hold a reference to the engine. If this is not done the 
		// engine will be kept alive forever
		engine->ShutDownAndRelease();
	}

	// Storing a function pointer in the dictionary
	// http://www.gamedev.net/topic/662542-assert-assigning-from-one-dictionary-to-another/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"int count = 0; \n"
			"funcdef void CB(); \n"
			"void test() \n"
			"{ \n"
			"    count++; \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"    CB@ func = @test; \n"
			"    dictionary dict; \n"
			"    @dict['func'] = func; \n"
			"    assert( dict.exists('func') ); \n"
			"    CB@ func2 = cast<CB>(dict['func']); \n"
			"    func2(); \n"
			"    assert( count == 1 ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		
		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PRINTF("%s\n", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test alternate syntax where empty list members give error
	// http://www.gamedev.net/topic/661578-array-trailing-comma/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->SetEngineProperty(asEP_DISALLOW_EMPTY_LIST_ELEMENTS, 1);

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		// The last comma will not be ignored since the dictValue expects exactly two values
		r = ExecuteString(engine, "dictionary dict = {{'a',}};");
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "ExecuteString (1, 25) : Error   : Empty list element is not allowed\n" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test assigning one dictionary value to another
	// http://www.gamedev.net/topic/662542-assert-assigning-from-one-dictionary-to-another/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void foo() { \n"
			"dictionary d1 = {{'1','1'}};\n"
			"dictionary d2 = {{'2','2'}};\n"
			"array<string>@ keys = d2.getKeys();\n"
			"for(uint i = 0; i < keys.length(); ++i)\n"
			"{\n"
			// Try 1:
			// No appropriate opAssign method found in 'dictionaryValue' for value assignment
			// Previous error occurred while attempting to create a temporary copy of object
			"  d1[keys[i]] = d2[keys[i]];\n"
			// Try 2:
			// Assert "tempVariables.exists(offset)" at as_compiler.cpp:4895, asCCompiler::ReleaseTemporaryVariable()
			"  string key = keys[i];\n"
			"  d1[key] = d2[key];\n"
			"}\n"
			"  assert( string(d1['1']) == '1' );\n"
			"  assert( string(d1['2']) == '2' );\n"
			"}\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "foo()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test retrieving arrays from dictionaries
	// http://www.gamedev.net/topic/660363-retrieving-an-array-of-strings-from-a-dictionary/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void func() { \n"
			"  dictionary d = { {'arr', array<string>(1, 'something')} }; \n"
			"  array<string> arr1 = cast<array<string>>(d['arr']); \n"
			"  assert( arr1.length() == 1 && arr1[0] == 'something' ); \n"
			"  array<string>@ arr2 = null; \n"
			"  bool found2 = d.get('arr', @arr2); \n"
			"  assert( arr2.length() == 1 && arr2[0] == 'something' ); \n"
			"  assert( found2 ); \n"
			"  array<string> arr3; \n"
			"  bool found3 = d.get('arr', arr3); \n"
			"  assert( arr3.length() == 1 && arr3[0] == 'something' ); \n"
			"  assert( found3 ); \n"
		//	"  array<string> arr4; \n"
		//	"  bool found4 = d.get('arr', @arr4); \n" // This is not valid, because arr4 is not a handle and cannot be reassigned
		//	"  assert( arr4.length() == 1 && arr4[0] == 'something' ); \n"
		//	"  assert( found4 ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "func()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test null in initialization list
	// http://www.gamedev.net/topic/660037-crash-when-instantiating-dictionary-with-null-value/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = ExecuteString(engine, "dictionary dict = {{'test', null}};");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test enums in the dictionary
	// http://www.gamedev.net/topic/659155-inconsistent-behavior-with-dictionary-and-enums/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterEnum("MyAppDefinedEnum");

		const char *script = 
			"enum foo { a, b, c }; \n"
			"void main() \n"
			"{ \n"
			"    assert( b == 1 ); \n"
			"    { \n"
			"        dictionary d = { {'enumVal', 1} }; \n"
			"        int val = int(d['enumVal']); \n"
			"        assert( val == 1 ); \n"
			"    } \n"
			"    { \n"
			"        dictionary d = { {'enumVal', b} }; \n"
			"        int val = int(d['enumVal']); \n"
			"        assert( val == 1 ); \n"
			"    } \n"
			"    { \n"
			"        dictionary d = { {'enumVal', b} }; \n"
			"        foo val = foo(d['enumVal']); \n"
			"        assert( val == 1 ); \n"
			"    } \n"
			"    MyAppDefinedEnum mode = MyAppDefinedEnum(10); \n"
			"    assert( mode == 10 ); \n"
			"    { \n"
			"        dictionary d = { {'mode', mode} }; \n"
			"        MyAppDefinedEnum m = MyAppDefinedEnum(d['mode']); \n"
			"        assert( mode == 10 ); \n"
			"        one(d); \n"
			"        two(d); \n"
			"    } \n"
			"    { \n"
			"        dictionary d; \n"
			"        d['mode'] = mode; \n"
			"        MyAppDefinedEnum m = MyAppDefinedEnum(d['mode']); \n"
			"        assert( mode == 10 ); \n"
			"        one(d); \n"
			"        two(d); \n"
			"    } \n"
			"} \n"
			"void one(dictionary@ d) \n"
			"{ \n"
			"    MyAppDefinedEnum m = MyAppDefinedEnum(d['mode']); \n"
			"    assert( m == 10 ); \n"
			"} \n"
			"void two(dictionary@ d) \n"
			"{ \n"
			"    int m = int(d['mode']); \n"
			"    assert( m == 10 ); \n"
			"} \n";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PRINTF("%s", GetExceptionInfo(ctx, true).c_str());
			TEST_FAILED;
		}
		ctx->Release();
		engine->Release();
	}

	// Test empty initialization list
	// http://www.gamedev.net/topic/658849-empty-array-initialization/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);

		r = ExecuteString(engine, "dictionary a = {};");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test the dictionaryValue
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		
		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void func() { \n"
			"  dictionary dict; \n"
			"  dict['hello'] = 42; \n"
			"  assert( int(dict['hello']) == 42 ); \n"
			"  @dict['hello'] = dict; \n"
			"  assert( cast<dictionary>(dict['hello']) is dict ); \n"
			"  dict['hello'] = 'hello'; \n"
			"  assert( string(dict['hello']) == 'hello' ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "func()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		// Exception if attempt to add entry on const dictionary
		ctx = engine->CreateContext();
		r = ExecuteString(engine, "dictionary dict; \n"
			                      "const dictionary @d = dict; \n"
			                      "d['hello']; \n", 0, ctx);
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;
		if( std::string(ctx->GetExceptionString()) != "Invalid access to non-existing value" )
		{
			PRINTF("%s", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		engine->Release();
	}

	// Test the STL iterator
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine); // Must register the dictionary so the cache is built
		CScriptDictionary *dict = CScriptDictionary::Create(engine);
		dict->Set("a", asINT64(1));
		dict->Set("b", asINT64(2));

		std::string keys; 
		asINT64 values = 0;
#if AS_CAN_USE_CPP11
		for( auto it : *dict )
#else
		for( CScriptDictionary::CIterator it = dict->begin(); it != dict->end(); it++ )
#endif
		{
			keys += it.GetKey();
			asINT64 val;
			it.GetValue(val);
			values += val;
		}

		// With C++11 the dictionary uses an unordered_map so the order of the keys is not guaranteed
		if (keys.find("a") == std::string::npos || keys.find("b") == std::string::npos || values != 3)
		{
			PRINTF("keys = '%s', values = %d\n", keys.c_str(), int(values));
			TEST_FAILED;
		}

		dict->Release();
		engine->Release();
	}

	// Test initialization list in expression
	SKIP_ON_MAX_PORT
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		RegisterScriptDictionary(engine);
		RegisterScriptMathComplex(engine);

		r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"void main() { \n"
			"  dictionary @dict; \n"
			"  @dict = dictionary = {{'hello', 'world'}, {'answer', 42}, {'complex', complex = {1,2}}}; \n"
			"  assert( dict.getKeys().length() == 3 ); \n"
			"  complex c; \n"
			"  dict.get('complex', c); \n"
			"  assert( c == complex(1,2) ); \n"
			"  func(dictionary = {{'hello', 'world'}}); \n"
			"} \n"
			"void func(dictionary @dict) {} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( r == asEXECUTION_EXCEPTION )
			PRINTF("%s", GetExceptionInfo(ctx).c_str());

		ctx->Release();
		engine->Release();
	}

	// Basic tests for dictionary
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		RegisterScriptDictionary(engine);

		r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script, strlen(script));
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "Test()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PRINTF("%s", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		asUINT gcCurrentSize, gcTotalDestroyed, gcTotalDetected;
		engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);
		engine->GarbageCollect();
		engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);

		if( gcCurrentSize != 0 || gcTotalDestroyed != 1 || gcTotalDetected != 1 )
			TEST_FAILED;

		// Test circular references including a script class and the dictionary
		mod->AddScriptSection("script", script2, strlen(script2));
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "f()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);
		engine->GarbageCollect();
		engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);

		if( gcCurrentSize != 0 || gcTotalDestroyed != 3 || gcTotalDetected != 3  )
			TEST_FAILED;

		// Test invalid ref cast together with the variable argument
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		r = ExecuteString(engine, "dictionary d; d.set('hello', cast<int>(4));");
		if( r >= 0 ) 
			TEST_FAILED;
		if( bout.buffer != "ExecuteString (1, 35) : Error   : Illegal target type for reference cast\n" )
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->Release();
	}

	// Test initializing the dictionary with the list factory
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		RegisterScriptDictionary(engine);

		r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", 
			"void main() { \n"
			"  dictionary dict = {{'t', true}, {'a', 42}, {'b', 'test'}, {'c', }}; \n"
			"  assert( dict.getSize() == 4 ); \n"
			"  int a; \n"
			"  dict.get('a', a); \n"
			"  assert( a == 42 ); \n"
			"  string b; \n"
			"  dict.get('b', b); \n"
			"  assert( b == 'test' ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		CBytecodeStream stream("test");
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PRINTF("%s", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		engine->Release();
	}

	//-------------------------
	// Test the generic interface as well
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	RegisterStdString(engine);
	RegisterScriptArray(engine, true);
	RegisterScriptDictionary_Generic(engine);

	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	ctx = engine->CreateContext();
	r = ExecuteString(engine, "Test()", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PRINTF("%s", GetExceptionInfo(ctx).c_str());
		TEST_FAILED;
	}
	ctx->Release();

	engine->Release();

	//------------------------
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		RegisterScriptDictionary(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	
		const char *script =
			"class Test \n"
			"{ \n"
			"  Test() { dict.set('int', 1); dict.set('string', 'test'); dict.set('handle', @array<string>()); } \n"
			"  dictionary dict; \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  Test test = Test(); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PRINTF("%s", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		engine->Release();
	}

	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		RegisterScriptDictionary(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	
		const char *script =
			"void main() \n"
			"{ \n"
			" dictionary test; \n"
			" test.set('test', 'something'); \n"
			" string output; \n"
			" test.get('test', output); \n"
			" assert(output == 'something'); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PRINTF("%s", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		engine->Release();
	}

	// Test initialization lists with global variables
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		RegisterScriptDictionary(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	
		const char *script =
			"string testGlobalStr = 'hello'; \n"
			"void main() \n"
			"{ \n"
			"   dictionary dic = {{'test', testGlobalStr}}; \n"
			"   string txt; \n"
			"   dic.get('test', txt); \n"
			"   assert( txt == 'hello' ); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PRINTF("%s", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		engine->Release();
	}

	return fail;
}

} // namespace

