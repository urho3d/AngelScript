//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestBasic
{

#define TESTNAME "TestBasic"

static const char *script =
"float N;                                  \n"
"                                          \n"
"void func5()                              \n"
"{                                         \n"
"    N += Average( N, N );                 \n"
"}                                         \n"
"                                          \n"
"void func4()                              \n"
"{                                         \n"
"    N += 2 * Average( N + 1, N + 2 );     \n"
"}                                         \n"
"                                          \n"
"void func3()                              \n"
"{                                         \n"
"    N *= 2.1f * N;                        \n"
"}                                         \n"
"                                          \n"
"void func2()                              \n"
"{                                         \n"
"    N /= 3.5f;                            \n"
"}                                         \n"
"                                          \n"
"void Recursion( int nRec )                \n"
"{                                         \n"
"    if ( nRec >= 1 )                      \n"
"        Recursion( nRec - 1 );            \n"
"                                          \n"
"    if ( nRec == 5 )                      \n"
"        func5();                          \n"
"    else if ( nRec == 4 )                 \n"
"        func4();                          \n"
"    else if ( nRec == 3 )                 \n"
"        func3();                          \n"
"    else if ( nRec == 2 )                 \n"
"        func2();                          \n"
"    else                                  \n"
"        N *= 1.5;                         \n"
"}                                         \n"
"                                          \n"
"int TestBasic()                           \n"
"{                                         \n"
"    N = 0;                                \n"
"    float i = 0;                          \n"
"                                          \n"
"    for ( i = 0; i < 250000; i += 0.25f ) \n"
"    {                                     \n"
"        Average( i, i );                  \n"
"        Recursion( 5 );                   \n"
"                                          \n"
"        if ( N > 100 ) N = 0;             \n"
"    }                                     \n"
"                                          \n"
"    return 0;                             \n"
"}                                         \n";

float Average(float a, float b)
{
	return (a+b)/2;
}

float N;                                  
                                          
void func5()                              
{                                         
    N += Average( N, N );                 
}                                         
                                          
void func4()                              
{                                         
    N += 2 * Average( N + 1, N + 2 );     
}                                         
                                          
void func3()                              
{                                         
    N *= 2.1f * N;                        
}                                         
                                          
void func2()                              
{                                         
    N /= 3.5f;                            
}                                         
                                          
void Recursion( int nRec )                
{                                         
    if ( nRec >= 1 )                      
        Recursion( nRec - 1 );            
                                          
    if ( nRec == 5 )                      
        func5();                          
    else if ( nRec == 4 )                 
        func4();                          
    else if ( nRec == 3 )                 
        func3();                          
    else if ( nRec == 2 )                 
        func2();                          
    else                                  
        N *= 1.5;                         
}                                         
                                          
int TestCPP()                                
{                                         
    N = 0;                                
    float i = 0;                          
                                          
    for ( i = 0; i < 250000; i += 0.25f ) 
    {                                     
        Average( i, i );                  
        Recursion( 5 );                   
                                          
        if ( N > 100 ) N = 0;             
    }                                     
                                          
    return 0;                             
}                                         


void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("Machine 1\n");
	printf("AngelScript 1.9.0             : 20.88 secs\n");
	printf("AngelScript 1.9.1             : 13.56 secs\n");
	printf("AngelScript 1.9.2             : 11.44 secs\n");
	printf("AngelScript 1.10.0 with C++ VM: 6.278 secs\n");
	printf("AngelScript 1.10.0 with ASM VM: 6.997 secs\n");
	printf("AngelScript 1.10.1 with ASM VM: 4.370 secs\n");
	printf("C++                           :  0.25 secs\n");
	printf("\n");
	printf("Machine 2\n");
	printf("AngelScript 1.9.0             : 8.812 secs\n");
	printf("AngelScript 1.9.1             : 5.605 secs\n");
	printf("AngelScript 1.9.2             : 4.674 secs\n");
	printf("AngelScript 1.10.0 with C++ VM: 2.450 secs\n");
	printf("AngelScript 1.10.0 with ASM VM: 3.234 secs\n");
	printf("AngelScript 1.10.1 with ASM VM: 2.245 secs\n");
	printf("AngelScript 2.0.0 with C++ VM : 2.538 secs\n");
	printf("AngelScript 2.0.0 with ASM VM : 2.335 secs\n");
	printf("C++                           :  0.14 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterGlobalFunction("float Average(float, float)", asFUNCTION(Average), asCALL_CDECL);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0, &out);

	asIScriptContext *ctx;
	engine->CreateContext(&ctx);
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "int TestBasic()"));

	printf("Executing AngelScript version...\n");

	double time = GetSystemTimer();

	int r = ctx->Execute();

	time = GetSystemTimer() - time;

	if( r != 0 )
	{
		printf("Execution didn't terminate with asEXECUTION_FINISHED\n", TESTNAME);
		if( r == asEXECUTION_EXCEPTION )
		{
			printf("Script exception\n");
			printf("Func: %s\n", engine->GetFunctionName(ctx->GetExceptionFunction()));
			printf("Line: %d\n", ctx->GetExceptionLineNumber());
			printf("Desc: %s\n", ctx->GetExceptionString());
		}
	}
	else
		printf("Time = %f secs\n", time);

	ctx->Release();
	engine->Release();

/*
	printf("Executing C++ version...\n");
	time = GetSystemTimer();
	TestCPP();
	time = GetSystemTimer() - time;
	printf("Time = %f secs\n", time);
*/
}

} // namespace



//---------------------------------------------------
// This is the same test in LUA script
//

/*

function func5()
    n = n + zfx.average( n, n )
end

function func4()
    n = n + 2 * zfx.average( n+1, n+2 )
end

function func3()
    n = n * 2.1 * n
end

function func2()
    n = n / 3.5
end

function recursion( rec )
    if rec >= 1 then
        recursion( rec - 1 )
    end

    if rec == 5 then func5()
        else if rec==4 then func4()
                else if rec==3 then func3()
                        else if rec==2 then func2()
                                else n = n * 1.5 
                                end
                        end
                end
        end
end

n = 0
i = 0

for i = 0, 249999, 0.25 do
    zfx.average( i, i + 1 ) 
    recursion( 5 )
    if n > 100 then n = 0 end
end

*/



