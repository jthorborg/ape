#include "stdafx.h"
#include "SharedInterfaceEx.h"
#include <cpl/stdext.h>
#include "BindingsHelper.h"

TEST_CASE("All bindings are resolved", "[SharedInterface]")
{
	using namespace ape;
	using namespace cpl;

	BindingsInterfaceResolver resolver;

	typedef void (APE_API * function)();

	auto const functionSize = sizeof(function);

	const char * start = reinterpret_cast<const char*>(&resolver);
	const char * end = reinterpret_cast<const char*>(&resolver.extra);

	int n = 0;

	for (; start != end; start += functionSize)
	{
		auto f = reinterpret_noub_cast<function*>(*start);

		REQUIRE(f != nullptr);
	}

}

TEST_CASE("All API functions throws without instance pointers", "[SharedInterface]")
{
	using namespace ape;
	using namespace cpl;

	BindingsInterfaceResolver resolver;
	size_t cf = 0, numFunctions = BindingsHelper::numFunctions();

	cf++; REQUIRE_THROWS(resolver.abortPlugin(nullptr, ""));
	cf++; REQUIRE_THROWS(resolver.alloc(nullptr, 0));
	cf++; REQUIRE_THROWS(resolver.createKnob(nullptr, nullptr, nullptr, 0));
	cf++; REQUIRE_THROWS(resolver.createKnobEx(nullptr, nullptr, nullptr, nullptr, nullptr));
	cf++; REQUIRE_THROWS(resolver.createLabel(nullptr, nullptr, nullptr));
	cf++; REQUIRE_THROWS(resolver.createMeter(nullptr, nullptr, nullptr));
	cf++; REQUIRE_THROWS(resolver.createPlot(nullptr, nullptr, nullptr, 0));
	cf++; REQUIRE_THROWS(resolver.createRangeKnob(nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0));
	cf++; REQUIRE_THROWS(resolver.createToggle(nullptr, nullptr, nullptr));
	cf++; REQUIRE_THROWS(resolver.free(nullptr, nullptr));
	cf++; REQUIRE_THROWS(resolver.getBPM(nullptr));
	cf++; REQUIRE_THROWS(resolver.getCtrlValue(nullptr, 0));
	cf++; REQUIRE_THROWS(resolver.getNumInputs(nullptr));
	cf++; REQUIRE_THROWS(resolver.getNumOutputs(nullptr));
	cf++; REQUIRE_THROWS(resolver.getSampleRate(nullptr));
	cf++; REQUIRE_THROWS(resolver.msgBox(nullptr, nullptr, nullptr, 0, 0));
	cf++; REQUIRE_THROWS(resolver.printLine(nullptr, 0, nullptr));
	cf++; REQUIRE_THROWS(resolver.setCtrlValue(nullptr, 0, 0));
	cf++; REQUIRE_THROWS(resolver.setInitialDelay(nullptr, 0));
	cf++; REQUIRE_THROWS(resolver.setStatus(nullptr, (APE_Status)0));
	cf++; REQUIRE_THROWS(resolver.timerDiff(nullptr, 0));
	cf++; REQUIRE_THROWS(resolver.timerGet(nullptr));

	// update above if you get an assertion here
	REQUIRE(cf == numFunctions, "Not all functions were tested!");

}