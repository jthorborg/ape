#include "stdafx.h"
#include "SharedInterfaceEx.h"
#include <cpl/stdext.h>
#include "Engine.h"

TEST_CASE("Basic API tests", "[Engine]")
{
	auto engine = std::make_unique<ape::Engine>();

	REQUIRE(engine->getGraphicUI() != nullptr);
	REQUIRE(engine->getCState() != nullptr);

}


TEST_CASE("Serialization roundtrip", "[Engine]")
{
	auto engine = std::make_unique<ape::Engine>();

	juce::MemoryBlock serializedData;

	REQUIRE_NOTHROW(engine->getStateInformation(serializedData));

	REQUIRE_NOTHROW(engine->setStateInformation(serializedData.getData(), (int)serializedData.getSize()));
}
