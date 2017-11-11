#include "stdafx.h"
#include "SharedInterfaceEx.h"
#include <cpl/stdext.h>
#include "Engine.h"
#include "CState.h"
#include <memory>
#include "BindingsHelper.h"

TEST_CASE("All API functions are callable without crashes with invalid arguments", "[SharedInterface]")
{
	using namespace ape;
	using namespace cpl;

	auto engine = std::make_unique<ape::Engine>();

	auto csys = engine->getCState();

	auto & sharedInterface = csys->getSharedInterface();
	
	std::size_t numFunctions = BindingsHelper::numFunctions();
	SECTION("Bindings from CState, with null shared interface")
	{
		std::size_t cf = 0;
		cf++; REQUIRE_THROWS(sharedInterface.abortPlugin(nullptr, ""));
		cf++; REQUIRE_THROWS(sharedInterface.alloc(nullptr, 0));
		cf++; REQUIRE_THROWS(sharedInterface.createKnob(nullptr, nullptr, nullptr, 0));
		cf++; REQUIRE_THROWS(sharedInterface.createKnobEx(nullptr, nullptr, nullptr, nullptr, nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.createLabel(nullptr, nullptr, nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.createMeter(nullptr, nullptr, nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.createPlot(nullptr, nullptr, nullptr, 0));
		cf++; REQUIRE_THROWS(sharedInterface.createRangeKnob(nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0));
		cf++; REQUIRE_THROWS(sharedInterface.createToggle(nullptr, nullptr, nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.free(nullptr, nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.getBPM(nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.getCtrlValue(nullptr, 0));
		cf++; REQUIRE_THROWS(sharedInterface.getNumInputs(nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.getNumOutputs(nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.getSampleRate(nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.msgBox(nullptr, nullptr, nullptr, 0, 0));
		cf++; REQUIRE_THROWS(sharedInterface.printLine(nullptr, 0, nullptr));
		cf++; REQUIRE_THROWS(sharedInterface.setCtrlValue(nullptr, 0, 0));
		cf++; REQUIRE_THROWS(sharedInterface.setInitialDelay(nullptr, 0));
		cf++; REQUIRE_THROWS(sharedInterface.setStatus(nullptr, (APE_Status)0));
		cf++; REQUIRE_THROWS(sharedInterface.timerDiff(nullptr, 0));
		cf++; REQUIRE_THROWS(sharedInterface.timerGet(nullptr));

		REQUIRE(cf == numFunctions);
	}

	SECTION("Bindings from CState, with valid shared interface")
	{
		std::size_t cf = 0;
		auto p = &sharedInterface;
		// throws by design
		cf++; REQUIRE_THROWS(sharedInterface.abortPlugin(p, ""));
		// no possible configuration where it can throw
		cf++; REQUIRE_NOTHROW(sharedInterface.alloc(p, 0));
		// throws due to null ptrs
		cf++; REQUIRE_THROWS(sharedInterface.createKnob(p, nullptr, nullptr, 0));
		// throws due to null ptrs
		cf++; REQUIRE_THROWS(sharedInterface.createKnobEx(p, nullptr, nullptr, nullptr, nullptr));
		// throws due to null ptrs
		cf++; REQUIRE_THROWS(sharedInterface.createLabel(p, nullptr, nullptr));
		// throws due to null ptrs
		cf++; REQUIRE_THROWS(sharedInterface.createMeter(p, nullptr, nullptr));
		// throws due to null ptrs
		cf++; REQUIRE_THROWS(sharedInterface.createPlot(p, nullptr, nullptr, 0));
		// throws due to null ptrs
		cf++; REQUIRE_THROWS(sharedInterface.createRangeKnob(p, nullptr, nullptr, nullptr, nullptr, 0, 0));
		// throws due to null ptrs
		cf++; REQUIRE_THROWS(sharedInterface.createToggle(p, nullptr, nullptr));
		// no possible configuration where it can throw
		cf++; REQUIRE_NOTHROW(sharedInterface.free(p, nullptr));
		// throws due to call stack not originating from a processing callback
		cf++; REQUIRE_THROWS(sharedInterface.getBPM(p));
		// throws due to no such control
		cf++; REQUIRE_THROWS(sharedInterface.getCtrlValue(p, 0));
		// no possible configuration where it can throw
		cf++; REQUIRE_NOTHROW(sharedInterface.getNumInputs(p));
		// no possible configuration where it can throw
		cf++; REQUIRE_NOTHROW(sharedInterface.getNumOutputs(p));
		// no possible configuration where it can throw
		cf++; REQUIRE_NOTHROW(sharedInterface.getSampleRate(p));
		// throws due to null ptrs
		cf++; REQUIRE_THROWS(sharedInterface.msgBox(p, nullptr, nullptr, 0, 0));
		// throws due to null ptrs
		cf++; REQUIRE_THROWS(sharedInterface.printLine(p, 0, nullptr));
		// throws due to no such control
		cf++; REQUIRE_THROWS(sharedInterface.setCtrlValue(p, 0, 0));
		// no possible configuration where it can throw
		cf++; REQUIRE_NOTHROW(sharedInterface.setInitialDelay(p, 0));
		// no possible configuration where it can throw
		cf++; REQUIRE_NOTHROW(sharedInterface.setStatus(p, (APE_Status)0));
		// no possible configuration where it can throw
		cf++; REQUIRE_NOTHROW(sharedInterface.timerDiff(p, 0));
		// no possible configuration where it can throw
		cf++; REQUIRE_NOTHROW(sharedInterface.timerGet(p)); 

		REQUIRE(cf == numFunctions);
	}

} 