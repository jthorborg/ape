#include "stdafx.h"
#include "SharedInterfaceEx.h"
#include <cpl/stdext.h>
#include "Engine.h"

TEST_CASE("Plugin can be created without errors", "[Initialization]")
{
	APE::Engine * plugin = nullptr;
	
	REQUIRE_NOTHROW(plugin = new APE::Engine());

	REQUIRE(plugin != nullptr);

	REQUIRE_NOTHROW(delete plugin);
}

TEST_CASE("Plugin can be created through JUCE APE", "[Initialization]")
{
	juce::AudioProcessor * JUCE_CALLTYPE createPluginFilter();

	juce::AudioProcessor * plugin = nullptr;

	REQUIRE_NOTHROW(plugin = createPluginFilter());

	REQUIRE(plugin != nullptr);

	REQUIRE(dynamic_cast<APE::Engine *>(plugin) != nullptr);

	REQUIRE_NOTHROW(delete plugin);
}
