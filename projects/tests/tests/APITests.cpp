#include "stdafx.h"
#include "SharedInterfaceEx.h"
#include <cpl/stdext.h>
#include "Engine.h"
#include "CState.h"
#include <memory>
#include "BindingsHelper.h"

TEST_CASE("All API functions are callable without crashes with invalid arguments", "[SharedInterface]")
{
	using namespace APE;
	using namespace cpl;

	auto engine = std::make_unique<APE::Engine>();

	auto csys = engine->getCState();

	auto & sharedInterface = csys->getSharedInterface();
	
	std::size_t numFunctions = BindingsHelper::numFunctions();
	SECTION("Bindings from CState")
	{
		std::size_t cf = 0;
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

	SECTION("Bindings from CState")
	{
		std::size_t cf = 0;
		auto p = &sharedInterface;
		cf++; REQUIRE_NOTHROW(sharedInterface.alloc(p, 0));
		/*cf++; REQUIRE_THROWS(sharedInterface.createKnob(nullptr, nullptr, nullptr, 0));
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
		cf++; REQUIRE_THROWS(sharedInterface.timerGet(nullptr)); */

		REQUIRE(cf == numFunctions);
	}

} 