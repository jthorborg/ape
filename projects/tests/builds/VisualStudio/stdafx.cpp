// stdafx.cpp : source file that includes just the standard includes
// $safeprojectname$.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#define CATCH_CONFIG_MAIN
#include "stdafx.h"
#include <cpl/Common.h>


namespace cpl
{
	const ProgramInfo programInfo
	{
		"ape Tests",
		cpl::Version::fromParts(0, 1, 0),
		"Janus Thorborg",
		"ape.tests",
		false,
		nullptr,
		""
	}; 
};