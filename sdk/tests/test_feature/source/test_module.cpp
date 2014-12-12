#include "utils.h"

namespace TestModule
{

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	COutStream out;
	asIScriptContext *ctx;

	// TODO: 2.30.0: Test to verify if proper clean up of script code is done when discarding and recompiling modules
	// Reported by Michael Rubin
/*
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class Foo { private array<Foo@> m_Foo; }\n"); // circular reference between class and template instance
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asUINT gcSize = 0, gcDestroyed = 0;
		engine->GetGCStatistics(&gcSize, &gcDestroyed);
		if( gcSize != 0 || gcDestroyed != 0 )
			TEST_FAILED;

		mod->Discard();
		engine->GetGCStatistics(&gcSize, &gcDestroyed);
		if( gcSize != 0 || gcDestroyed == 0 )
			TEST_FAILED;

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class Foo { private array<Foo@>@ m_Foo = null; }\n"); // circular reference between class and template instance
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		mod->Discard();
		engine->GetGCStatistics(&gcSize, &gcDestroyed);
		if( gcSize != 0 || gcDestroyed == 0 )
			TEST_FAILED;

		engine->ShutDownAndRelease();
	}
*/
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	ctx = engine->CreateContext();
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	// Compile a single function
	asIScriptFunction *func = 0;
	r = mod->CompileFunction("My func", "void func() {}", 0, 0, &func);
	if( r < 0 )
		TEST_FAILED;

	// Execute the function
	r = ctx->Prepare(func);
	if( r < 0 )
		TEST_FAILED;

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	// The function's section name should be correct
	if( std::string(func->GetScriptSectionName()) != "My func" )
		TEST_FAILED;

	// We must release the function afterwards
	if( func )
	{
		func->Release();
		func = 0;
	}

	// It must not be allowed to include more than one function in the code
	bout.buffer = "";
	r = mod->CompileFunction("two funcs", "void func() {} void func2() {}", 0, 0, 0);
	if( r >= 0 )
		TEST_FAILED;
	r = mod->CompileFunction("no code", "", 0, 0, 0);
	if( r >= 0 )
		TEST_FAILED;
	r = mod->CompileFunction("var", "int a;", 0, 0, 0);
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "two funcs (0, 0) : Error   : The code must contain one and only one function\n"
					   "no code (1, 1) : Warning : The script section is empty\n"
					   "no code (0, 0) : Error   : The code must contain one and only one function\n"
					   "var (0, 0) : Error   : The code must contain one and only one function\n" )
	{
		PRINTF("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Compiling without giving the function pointer shouldn't leak memory
	r = mod->CompileFunction(0, "void func() {}", 0, 0, 0);
	if( r < 0 )
		TEST_FAILED;

	// If the code is not provided, a proper error should be given
	r = mod->CompileFunction(0,0,0,0,0);
	if( r != asINVALID_ARG )
		TEST_FAILED;

	// Don't permit recursive calls, unless the function is added to the module scope
	// TODO: It may be possible to compile a recursive function even without adding
	//       it to the scope, but the application needs to explicitly allows it
	bout.buffer = "";
	r = mod->CompileFunction(0, "void func() {\n func(); \n}", -1, 0, 0);
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != " (1, 2) : Error   : No matching signatures to 'func()'\n" )
	{
		PRINTF("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// It should be possible to add the compiled function to the scope of the module
	if( mod->GetFunctionCount() > 0 )
		TEST_FAILED;
	r = mod->CompileFunction(0, "void func() {}", 0, asCOMP_ADD_TO_MODULE, 0);
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetFunctionCount() != 1 )
		TEST_FAILED;

	// It should be possible to remove a function from the scope of the module
	r = mod->RemoveFunction(mod->GetFunctionByIndex(0));
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetFunctionCount() != 0 )
		TEST_FAILED;

	// Compiling recursive functions that are added to the module is OK
	r = mod->CompileFunction(0, "void func() {\n func(); \n}", -1, asCOMP_ADD_TO_MODULE, 0);
	if( r < 0 )
		TEST_FAILED;

	// It should be possible to remove global variables from the scope of the module
	mod->AddScriptSection(0, "int g_var; void func() { g_var = 1; }");
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetGlobalVarCount() != 1 )
		TEST_FAILED;
	r = mod->RemoveGlobalVar(0);
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetGlobalVarCount() != 0 )
		TEST_FAILED;
	r = ExecuteString(engine, "func()", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	// It should be possible to add new variables
	r = mod->CompileGlobalVar(0, "int g_var;", 0);
	if( r < 0 )
		TEST_FAILED;
	r = mod->CompileGlobalVar(0, "int g_var2 = g_var;", 0);
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetGlobalVarCount() != 2 )
		TEST_FAILED;

	// Shouldn't be possible to add function with the same name as a global variable
	bout.buffer = "";
	r = mod->CompileFunction(0, "void g_var() {}", 0, asCOMP_ADD_TO_MODULE, 0);
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != " (1, 1) : Error   : Name conflict. 'g_var' is a global property.\n" )
	{
		PRINTF("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	if( ctx ) 
		ctx->Release();
	engine->Release();

	// TODO: Removing a function from the scope of the module shouldn't free it 
	//       immediately if it is still used by another function. This is working.
	//       I just need a formal test for regression testing.

	// TODO: Make sure cyclic references between functions are resolved so we don't get memory leaks
	//       This is working. I just need a formal test for regression testing.

	// TODO: Do not allow adding functions that already exist in the module

	// TODO: Maybe we can allow replacing an existing function

	// TODO: It should be possible to serialize these dynamic functions

	// TODO: The dynamic functions should also be JIT compiled

	// TODO: What should happen if a function in the module scope references another function that has 
	//       been removed from the scope but is still alive, and then the byte code for the module is saved?

	// Make sure a circular reference between global variable, class, and class method is properly released
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule("script", asGM_ALWAYS_CREATE);
	const char *script = "obj o; class obj { void d() { o.val = 1; } int val; }";
	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r != 0 )
		TEST_FAILED;
	if( bout.buffer != "" )
	{
		PRINTF("%s", bout.buffer.c_str());
		TEST_FAILED;
	}
	engine->Release();

	// GetObjectTypeById must not crash even though the object type has already been removed
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterScriptArray(engine, false);

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", "class A {}");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		int typeId = mod->GetTypeIdByDecl("array<A@>");
		if( typeId < 0 )
			TEST_FAILED;

		asIObjectType *type = engine->GetObjectTypeById(typeId);
		if( type == 0 || std::string(type->GetName()) != "array" )
			TEST_FAILED;

		if( type != mod->GetObjectTypeByDecl("array<A@>") )
			TEST_FAILED;

		mod->Discard();
		engine->GarbageCollect();

		type = engine->GetObjectTypeById(typeId);
		if( type != 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Recompiling the same module over and over again without 
	// discarding shouldn't increase memory consumption
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		const char *script =
			"int global = 0; \n"
			"void func() {} \n"
			"class Cls { void mthd() {} } \n"
			"shared class SCls { void mthd() {} } \n"
			"interface Int { void mthd(); } \n";

		const char *scriptWithError = 
			"void func2() { rterte } \n";

		int memSize[5] = {0};

		for( int n = 0; n < 5; n++ )
		{
			// Try with a module that is reused without first discarding it
			bout.buffer = "";
			asIScriptModule *mod1 = engine->GetModule("script1", asGM_CREATE_IF_NOT_EXISTS);

			r = mod1->AddScriptSection("name", script);
			r = mod1->Build();
			if( r < 0 )
				TEST_FAILED;

			if( bout.buffer != "" )
			{
				PRINTF("%s", bout.buffer.c_str());
				TEST_FAILED;
			}

			// Try with a module that is compiled with error 
			bout.buffer = "";
			asIScriptModule *mod2 = engine->GetModule("script2", asGM_CREATE_IF_NOT_EXISTS);
			r = mod2->AddScriptSection("name", script);
			r = mod2->AddScriptSection("error", scriptWithError);
			r = mod2->Build();
			if( r >= 0 )
				TEST_FAILED;

			if( bout.buffer != "error (1, 1) : Info    : Compiling void func2()\n"
							   "error (1, 23) : Error   : Expected ';'\n"
							   "error (1, 23) : Error   : Instead found '}'\n" )
			{
				PRINTF("%s", bout.buffer.c_str());
				TEST_FAILED;
			}

			memSize[n] = GetAllocedMem();
		}

		if( engine->GetModuleCount() != 2 )
			TEST_FAILED;

		engine->Release();

		// The first iteration uses slightly less memory due to internal buffers
		// holding larger capacity after the second build, but afterwards the 
		// size should be constant
		if( memSize[1] != memSize[3] || memSize[3] != memSize[4] )
			TEST_FAILED;
	}

	// Recompiling the same module over and over again shouldn't increase memory consumption
	// Another test, this time with derived classes
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		const char *script =
			"class ssBlock \n"
			"{ \n"
			"    ssNode@    GetNode() { return ssCreateNode(); } \n"
			"}; \n"
			"class ssNode \n"
			"{ \n"
			"    ssBlock GetBlock() { return ssBlock(); } \n"
			"}; \n"
			"class ssNode_Float : ssNode \n"
			"{ \n"
			"} \n"
			"ssNode@ ssCreateNode() \n"
			"{ \n"
			"    ssNode_Float FloatNode(); \n"
			"    return FloatNode; \n"
			"} \n"
			;

		int memSize[5] = {0};

		for( int n = 0; n < 5; n++ )
		{
			// Try with a module that is reused without first discarding it
			bout.buffer = "";
			asIScriptModule *mod1 = engine->GetModule("script1", asGM_ALWAYS_CREATE);

			// There shouldn't be anything left in the GC 
			asUINT gcSize;
			engine->GetGCStatistics(&gcSize);
			if( gcSize != 0 )
				TEST_FAILED;

/*			for( asUINT i = 0; i < gcSize; i++ )
			{
				void *obj = 0;
				asIObjectType *type = 0;
				engine->GetObjectInGC(i, 0, &obj, &type);

				if( strcmp(type->GetName(), "_builtin_function_") == 0 )
				{
					asIScriptFunction *func = (asIScriptFunction*)obj;
					PRINTF("func: %s\n", func->GetDeclaration());
				}
				else
				{
					asIObjectType *ot = (asIObjectType*)obj;
					PRINTF("type: %s\n", ot->GetName());
				}
			}*/

			r = mod1->AddScriptSection("name", script);
			r = mod1->Build();
			if( r < 0 )
				TEST_FAILED;

			if( bout.buffer != "" )
			{
				PRINTF("%s", bout.buffer.c_str());
				TEST_FAILED;
			}

			memSize[n] = GetAllocedMem();
		}

		engine->Release();

		// The first iteration uses slightly less memory due to internal buffers
		// holding larger capacity after the second build, but afterwards the 
		// size should be constant
		if( memSize[1] != memSize[3] || memSize[3] != memSize[4] )
			TEST_FAILED;
	}

	// Success
	return fail;
}

} // namespace

