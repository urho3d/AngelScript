//
// This test shows how to register the std::string to be used in the scripts.
// It also used to verify that objects are always constructed before destructed.
//
// Author: Andreas Jonsson
//

#include "utils.h"
#include <string>
using namespace std;

namespace Test_Addon_StdString
{
	bool Test()
	{
		bool fail = false;
		int r;

		COutStream out;

		{
			asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
			engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
			RegisterStdString(engine);
			engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

			// Trivial allocation of string constant and subsequent de-allocation
			r = ExecuteString(engine, "string a = 'hello'");
			if (r != asEXECUTION_FINISHED) TEST_FAILED;

			// Test type conversions
			r = ExecuteString(engine,
				"string a = 123; assert( a == '123' ); \n"
				"a += 123; assert( a == '123123' ); \n");
			if (r != asEXECUTION_FINISHED) TEST_FAILED;

			r = ExecuteString(engine,
				"string a = 123.4; \n"
				"assert( a == '123.4' ); \n"
				"a += 123.4; \n"
				"assert( a == '123.4123.4' ); \n");
			if (r != asEXECUTION_FINISHED) TEST_FAILED;

			// Test find routines
			r = ExecuteString(engine,
				"string a = 'The brown fox jumped the white fox'; \n"
				"assert( a.findFirst('fox') == 10); \n"
				"assert( a.findFirstOf('fjq') == 10); \n"
				"assert( a.findFirstNotOf('The') == 3); \n"
				"assert( a.findLast('fox') == 31); \n"
				"assert( a.findLastOf('fjq') == 31); \n"
				"assert( a.findLastNotOf('fox') == 33); \n");
			if (r != asEXECUTION_FINISHED) TEST_FAILED;

			if( strstr(asGetLibraryOptions(), "AS_NO_EXCEPTIONS") )
			{
				PRINTF("Test on line %d in %s skipped due to AS_NO_EXCEPTIONS\n", __LINE__, __FILE__);
			}
			else
			{
				// Test insert and erase
				r = ExecuteString(engine,
					"string a; \n"
					"a.insert(5, 'hello'); \n");  // attempt to insert beyond the size of the string
				if (r != asEXECUTION_EXCEPTION) TEST_FAILED;
			}

			r = ExecuteString(engine,
				"string a; \n"
				"a.insert(0, 'hello'); \n" // at index 1 beyond the last it is allowed
				"a.erase(2,2); \n"
				"assert( a == 'heo' ); \n");
			if (r != asEXECUTION_FINISHED) TEST_FAILED;

			engine->ShutDownAndRelease();
		}

		// Test saving and loading bytecode with std:string
		{
			asIScriptEngine *engine = asCreateScriptEngine();
			engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
			RegisterStdString(engine);
			engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

			// Trivial allocation of string constant and subsequent de-allocation
			asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
			mod->AddScriptSection("test", "string a = 'hello';");
			r = mod->Build();
			if (r < 0)
				TEST_FAILED;

			CBytecodeStream bcStream("bc");
			r = mod->SaveByteCode(&bcStream);
			if (r < 0)
				TEST_FAILED;

			mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
			r = mod->LoadByteCode(&bcStream);
			if (r < 0)
				TEST_FAILED;

			string *str = reinterpret_cast<string*>(mod->GetAddressOfGlobalVar(0));
			if ((*str) != "hello")
				TEST_FAILED;

			engine->ShutDownAndRelease();
		}

		// Test formatInt
		// https://www.gamedev.net/forums/topic/692989-constant-integer-corruption-with-vs2010/
		{
			asIScriptEngine *engine = asCreateScriptEngine();

			engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
			RegisterStdString(engine);
			engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

			r = ExecuteString(engine,
				"const int64 a1 = 4287660160; assert(formatInt(a1, '0h', 8) == 'ff908080'); \n"
				"int64 a2 = 4287660160; assert(formatInt(a2, '0h', 8) == 'ff908080'); \n"
				"const int64 a3 = 4287660160.0f; assert(formatInt(a3, '0h', 8) == 'ff908000'); \n" // due to float precision the result is different, but correct
				"int64 a4 = 4287660160.0f; assert(formatInt(a4, '0h', 8) == 'ff908000'); \n" // due to float precision the result is different, but correct
				"const int64 a5 = 4287660160.0; assert(formatInt(a5, '0h', 8) == 'ff908080'); \n"
				"int64 a6 = 4287660160.0; assert(formatInt(a6, '0h', 8) == 'ff908080'); \n"
				);
			if (r != asEXECUTION_FINISHED) TEST_FAILED;

			engine->ShutDownAndRelease();
		}

		// Test string methods
		{
			asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
			engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

			RegisterStdString(engine);
			RegisterScriptArray(engine, false);
			RegisterStdStringUtils(engine);
			engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

			r = ExecuteString(engine,
				"  string str = 'hello world'; \n"
				"  assert( str.substr(6, 5) == 'world' ); \n"
				"  assert( str.findFirst('o') == 4 ); \n"
				"  assert( str.findLast('o') == 7 ); \n"
				"  array<string> @arr = 'A|B||D'.split('|'); \n"
				"  assert( arr.length() == 4 && \n"
				"      arr[0] == 'A' && \n"
				"      arr[1] == 'B' && \n"
				"      arr[2] == ''  && \n"
				"      arr[3] == 'D' ); \n"
				"  assert( join(arr, ';') == 'A;B;;D' ); \n"
				"  assert( formatInt(123456789012345, 'l', 20).length() == 20 ); \n"
				"  assert( formatFloat(123.456, '', 20, 10).length() == 20 ); \n"
				"  assert( parseInt('1234') == 1234 ); \n"
				"  assert( parseInt('-123') == -123 ); \n"
				"  assert( parseFloat('123.456') == 123.456 ); \n"
				"  assert( parseUInt('-1') == 0 ); \n"
				"  assert( parseUInt('ABC', 16) == 0xABC ); \n"
				"  assert( '12345'.length == 5 ); \n");
			if (r != asEXECUTION_FINISHED)
				TEST_FAILED;

			// Test problem with formatInt and condition expression
			// Reported by neorej16
			r = ExecuteString(engine,
				"true ? formatInt(5, ' ', 4) : '';\n"
				"formatInt(5, ' ', 4);\n");
			if (r != asEXECUTION_FINISHED)
				TEST_FAILED;

			engine->Release();
		}

		return fail;
	}
}