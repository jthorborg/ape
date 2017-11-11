#include "stdafx.h"
#include "SharedInterfaceEx.h"
#include <cpl/stdext.h>
#include "Engine.h"

TEST_CASE("Plugin can be created without errors", "[Initialization]")
{
	ape::Engine * plugin = nullptr;
	
	REQUIRE_NOTHROW(plugin = new ape::Engine());

	REQUIRE(plugin != nullptr);

	REQUIRE_NOTHROW(delete plugin);
}

TEST_CASE("Plugin can be created through JUCE ape", "[Initialization]")
{
	juce::AudioProcessor * JUCE_CALLTYPE createPluginFilter();

	juce::AudioProcessor * plugin = nullptr;

	REQUIRE_NOTHROW(plugin = createPluginFilter());

	REQUIRE(plugin != nullptr);

	REQUIRE(dynamic_cast<ape::Engine *>(plugin) != nullptr);

	REQUIRE_NOTHROW(delete plugin);
}
