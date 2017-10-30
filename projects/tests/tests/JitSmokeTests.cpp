#include "stdafx.h"
#include <Engine.h>
#include <libtcc.h>
#include "CommonHelpers.h"
#include <cpl/Protected.h>

typedef int (APE_API * IntRetArgFunction)(int arg);
typedef int (APE_API * CallbackFunction)(void * state, int arg);
typedef int (APE_API * CallbackInvoker)(CallbackFunction fn, void * state, int arg);
typedef float (APE_API * FloatFunc)(float a, float b, float c);
typedef double (APE_API * DoubleFunc)(double a, double b, double c);
typedef int (APE_API * TestCallbacksFunction)();

const std::string program = 
R"(

typedef int (* CallbackFunction)(void * state, int arg);
typedef float (* FloatFunc)(float a, float b, float c);
typedef double (* DoubleFunc)(double a, double b, double c);

FloatFunc floatFunc;
DoubleFunc doubleFunc;

int IntRetArg(int argument)
{
	return 2 * argument;
}

int CallbackInvoker(CallbackFunction cb, void * state, int arg)
{
	return cb(state, arg);
}

float FloatCallback(float a, float b, float c)
{
	if(floatFunc == 0)
		return -1;

	return floatFunc(a, b, c);
}

double DoubleCallback(double a, double b, double c)
{
	if(doubleFunc == 0)
		return -1;

	return doubleFunc(a, b, c);
}

int TestCallbacksAreSet()
{
	return floatFunc && doubleFunc;
}

)";

const std::string invalidProgram = program + "z";
const auto libpath = tests::RepositoryRoot() / "make" / "skeleton" / "compilers" / "TCC4APE";

TEST_CASE("TCC is linked and can create states", "[JitCodeGen]")
{
	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	TCCState * tcc = tcc_new();
	REQUIRE(tcc != nullptr);
	tcc_set_error_func(tcc, nullptr, error);
	tcc_delete(tcc);
}

TEST_CASE("TCC can compile", "[JitCodeGen]")
{
	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, nullptr, error);
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);
	REQUIRE(tcc_compile_string(tcc, program.c_str()) != -1);
	tcc_delete(tcc);
}

void * const opaque = reinterpret_cast<void * const>(0xDEADBEEF);
int nErrorCalls = 0;

TEST_CASE("TCC calls back error on bad compilation", "[JitCodeGen]")
{
	nErrorCalls = 0;
	auto error = [](void * op, const char * msg)
	{
		nErrorCalls++;
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);
	REQUIRE(tcc_compile_string(tcc, invalidProgram.c_str()) == -1);
	tcc_delete(tcc);
}

TEST_CASE("TCC preserves opaque", "[JitCodeGen]")
{
	nErrorCalls = 0;
	auto error = [](void * op, const char * msg)
	{
		REQUIRE(op == opaque);
		nErrorCalls++;
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);
	REQUIRE(tcc_compile_string(tcc, invalidProgram.c_str()) == -1);
	REQUIRE(nErrorCalls > 0);
	tcc_delete(tcc);
}

TEST_CASE("TCC can relocate", "[JitCodeGen]")
{
	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	tcc_set_lib_path(tcc, libpath.string().c_str());
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);

	REQUIRE(tcc_compile_string(tcc, program.c_str()) != -1);
	REQUIRE(tcc_relocate(tcc, TCC_RELOCATE_AUTO) != -1);

	tcc_delete(tcc);
}

TEST_CASE("TCC can return data symbols", "[JitCodeGen]")
{
	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	tcc_set_lib_path(tcc, libpath.string().c_str());
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);

	REQUIRE(tcc_compile_string(tcc, program.c_str()) != -1);
	REQUIRE(tcc_relocate(tcc, TCC_RELOCATE_AUTO) != -1);

	REQUIRE(tcc_get_symbol(tcc, "floatFunc") != nullptr);
	REQUIRE(tcc_get_symbol(tcc, "doubleFunc") != nullptr);

	tcc_delete(tcc);
}

TEST_CASE("TCC can return function symbols", "[JitCodeGen]")
{
	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	tcc_set_lib_path(tcc, libpath.string().c_str());
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);
	REQUIRE(tcc_compile_string(tcc, program.c_str()) != -1);
	REQUIRE(tcc_relocate(tcc, TCC_RELOCATE_AUTO) != -1);

	REQUIRE(tcc_get_symbol(tcc, "IntRetArg") != nullptr);
	REQUIRE(tcc_get_symbol(tcc, "CallbackInvoker") != nullptr);
	REQUIRE(tcc_get_symbol(tcc, "FloatCallback") != nullptr);
	REQUIRE(tcc_get_symbol(tcc, "DoubleCallback") != nullptr);

	tcc_delete(tcc);
}


TEST_CASE("Can invoke TCC IntRetArgFunction", "[JitCodeGen]")
{
	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	tcc_set_lib_path(tcc, libpath.string().c_str());
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);
	REQUIRE(tcc_compile_string(tcc, program.c_str()) != -1);
	REQUIRE(tcc_relocate(tcc, TCC_RELOCATE_AUTO) != -1);

	IntRetArgFunction f = (IntRetArgFunction) tcc_get_symbol(tcc, "IntRetArg");

	const int halfIntRange = std::numeric_limits<int>::max() >> 1;

	REQUIRE(f != nullptr);

	REQUIRE(f(0) == 0);
	REQUIRE(f(1) == 2);
	REQUIRE(f(-1) == -2);
	REQUIRE(f(70) == 140);
	REQUIRE(f(halfIntRange) == halfIntRange * 2);

	tcc_delete(tcc);
}

int nCallbacks = 0;

TEST_CASE("Can invoke TCC JIT callback", "[JitCodeGen]")
{
	nCallbacks = 0;

	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	auto callback = [](void * op, int arg)
	{
		nCallbacks++;
		if (op != opaque)
			return -1;

		return arg;
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	tcc_set_lib_path(tcc, libpath.string().c_str());
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);
	REQUIRE(tcc_compile_string(tcc, program.c_str()) != -1);
	REQUIRE(tcc_relocate(tcc, TCC_RELOCATE_AUTO) != -1);

	CallbackInvoker cbi = (CallbackInvoker)tcc_get_symbol(tcc, "CallbackInvoker");

	REQUIRE(cbi != nullptr);

	REQUIRE(cbi(callback, opaque, 1) == 1);
	REQUIRE(nCallbacks == 1);
	REQUIRE(cbi(callback, opaque, 2) == 2);
	REQUIRE(nCallbacks == 2);
	REQUIRE(cbi(callback, opaque, 377) == 377);
	REQUIRE(nCallbacks == 3);
	REQUIRE(cbi(callback, nullptr, 1) == -1);
	REQUIRE(nCallbacks == 4);

	tcc_delete(tcc);
}

TEST_CASE("Can set TCC symbols", "[JitCodeGen]")
{
	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	auto floatCallback = [](float a, float b, float c)
	{
		return a * b * c;
	};

	auto doubleCallback = [](double a, double b, double c)
	{
		return a * b * c;
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	tcc_set_lib_path(tcc, libpath.string().c_str());
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);
	REQUIRE(tcc_compile_string(tcc, program.c_str()) != -1);
	REQUIRE(tcc_relocate(tcc, TCC_RELOCATE_AUTO) != -1);
	
	TestCallbacksFunction test = (TestCallbacksFunction)tcc_get_symbol(tcc, "TestCallbacksAreSet");

	REQUIRE(test != nullptr);

	REQUIRE(test() == 0);

	FloatFunc * ff = (FloatFunc *)tcc_get_symbol(tcc, "floatFunc");
	DoubleFunc * df = (DoubleFunc *)tcc_get_symbol(tcc, "doubleFunc");

	REQUIRE(ff != nullptr);
	REQUIRE(df != nullptr);
	REQUIRE(*ff == nullptr);
	REQUIRE(*df == nullptr);

	*ff = floatCallback;
	*df = doubleCallback;

	REQUIRE(test() == 1);

	tcc_delete(tcc);
}

int nFloatCallbacks = 0;
int nDoubleCallbacks = 0;

TEST_CASE("Can invoke TCC float/double function callback", "[JitCodeGen]")
{
	nFloatCallbacks = 0;
	nDoubleCallbacks = 0;

	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	auto floatCallback = [](float a, float b, float c)
	{
		nFloatCallbacks++;
		auto val = a * b + c;
		return val;
	};

	auto doubleCallback = [](double a, double b, double c)
	{
		nDoubleCallbacks++;
		auto val = a * b + c;
		return val;
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	tcc_set_lib_path(tcc, libpath.string().c_str());
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);
	REQUIRE(tcc_compile_string(tcc, program.c_str()) != -1);
	REQUIRE(tcc_relocate(tcc, TCC_RELOCATE_AUTO) != -1);

	TestCallbacksFunction test = (TestCallbacksFunction)tcc_get_symbol(tcc, "TestCallbacksAreSet");

	REQUIRE(test != nullptr);

	REQUIRE(test() == 0);

	FloatFunc * ff = (FloatFunc *)tcc_get_symbol(tcc, "floatFunc");
	DoubleFunc * df = (DoubleFunc *)tcc_get_symbol(tcc, "doubleFunc");
	FloatFunc tccFF = (FloatFunc)tcc_get_symbol(tcc, "FloatCallback");
	DoubleFunc tccDF = (DoubleFunc)tcc_get_symbol(tcc, "DoubleCallback");

	REQUIRE(ff != nullptr);
	REQUIRE(df != nullptr);
	REQUIRE(tccFF != nullptr);
	REQUIRE(tccDF != nullptr);
	REQUIRE(*ff == nullptr);
	REQUIRE(*df == nullptr);

	REQUIRE(nFloatCallbacks == 0);
	REQUIRE((*tccFF)(2, 2, 2) == -1);
	REQUIRE(nFloatCallbacks == 0);
	REQUIRE(nDoubleCallbacks == 0);
	REQUIRE((*tccDF)(3, 6, 9) == -1);
	REQUIRE(nDoubleCallbacks == 0);

	*ff = floatCallback;
	*df = doubleCallback;

	REQUIRE(test() == 1);

	REQUIRE((*tccFF)(2, 2, 2) == 2 * 2 + 2);
	REQUIRE((*tccDF)(2, 2, 2) == 2 * 2 + 2);

	REQUIRE((*tccFF)(0, 2, 2) == 0 * 2 + 2);
	REQUIRE((*tccDF)(0, 2, 2) == 0 * 2 + 2);

	REQUIRE((*tccFF)(1, 2, 2) == 1 * 2 + 2);
	REQUIRE((*tccDF)(1, 2, 2) == 1 * 2 + 2);

	REQUIRE((*tccFF)(0.5f, 2, 2) == 0.5f * 2 + 2);
	REQUIRE((*tccDF)(0.5, 2, 2) == 0.5 * 2 + 2);

	REQUIRE((*tccFF)(1 / 3.0f, 2, 1 / 3.0f) == 1 / 3.0f * 2 + 1 / 3.0f);
	REQUIRE((*tccDF)(1 / 3.0, 2, 1 / 3.0) == 1 / 3.0 * 2 + 1 / 3.0);

	REQUIRE(nDoubleCallbacks == 5);
	REQUIRE(nFloatCallbacks == 5);

	tcc_delete(tcc);
}

TEST_CASE("Can catch hardware exception across JIT", "[JitCodeGen]")
{
	nCallbacks = 0;

	auto error = [](void * op, const char * msg)
	{
		FAIL(msg);
	};

	auto callback = [](void * op, int arg)
	{
		nCallbacks++;
		if (op != opaque)
			return -1;

		return 10 / arg;
	};

	TCCState * tcc = tcc_new();
	tcc_set_error_func(tcc, opaque, error);
	tcc_set_lib_path(tcc, libpath.string().c_str());
	REQUIRE(tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY) != -1);
	REQUIRE(tcc_compile_string(tcc, program.c_str()) != -1);
	REQUIRE(tcc_relocate(tcc, TCC_RELOCATE_AUTO) != -1);

	CallbackInvoker cbi = (CallbackInvoker)tcc_get_symbol(tcc, "CallbackInvoker");

	REQUIRE(cbi != nullptr);

	REQUIRE(cbi(callback, opaque, 1) == 10);
	REQUIRE(nCallbacks == 1);
	
	//REQUIRE_THROWS_AS(
	cpl::CProtected::instance().runProtectedCode([&]() { cbi(callback, opaque, 0); })//, 
//	cpl::CProtected::CSystemException
//;


; REQUIRE(nCallbacks == 2);


	tcc_delete(tcc);
}