/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
	 This program is free software: you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.
	 
	 This program is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.
	 
	 You should have received a copy of the GNU General Public License
	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
	 
	 See \licenses\ for additional details on licenses associated with this program.
 
 **************************************************************************************

	file:CompilerBinding.cpp
		Implementation of classes in CompilerBinding.h

*************************************************************************************/

#include "CompilerBinding.h"
#include <sstream>
#include <cpl/Misc.h>

namespace ape
{
	/*
        The exported symbol names for external compilers
	*/
	static const char * g_sExports[] =
	{
		"CreateProject",
		"CompileProject",
		"InitProject",
		"ActivateProject",
		"DisableProject",
		"ProcessReplacing",
		"OnEvent",
		"ReleaseProject",
		"CleanCache"
	};

	/*
		Inits all bindings in this CBinding from module.
		If Setting is valid, it is expected to be of this format:
			...
			exports:
			{
				errorFunc = "_@4errorFunc";
				...
			}
		And allows to specify different names/decorations than default.
		If key and value is not found, it will default to the same value in
		g_sExports.
	*/
	void CompilerBinding::Bindings::loadBindings(cpl::CModule& module, const libconfig::Setting & exportSettings)
	{
		bool settingsIsValid = exportSettings.isGroup() && !strcmp(exportSettings.getName(), "exports");
		
		cpl::foreach_uenum<ExportIndex>(
			[&](auto i)
			{
				const char * name = nullptr;
				// see if we can get a valid name out of our settings
				if (settingsIsValid && exportSettings.exists(g_sExports[i]))
				{
					name = exportSettings[g_sExports[i]].c_str();
				}
                
				// shortcircuiting avoids null-dereferencing
				if (!name || (name && !name[0]))
					name = g_sExports[i];
                
				_table[i] = module.getFuncAddress(name);

				if (!_table[i]) 
				{
					std::stringstream fmt;
					fmt << "Error retrieving pointer for function " << g_sExports[i] << ". ";
					fmt << "Specific name: " << name << ". Was settings valid? " << std::boolalpha << settingsIsValid << ".";
					throw std::runtime_error(fmt.str());
				}
			}
		);
	}
    
	CompilerBinding::CompilerBinding(const libconfig::Setting& languageSettings)
    {
        using namespace std::string_literals;
        
        language = languageSettings.getParent().getName();
        compilerPath = languageSettings["path"].c_str();
        compilerName = languageSettings["name"].c_str();

        if(!compilerPath.length())
            throw std::runtime_error(cpl::format("Invalid or empty path for compiler \'%s\' for language \'%s\'.", compilerName.c_str(), language.c_str()));
        
        const auto canon = cpl::Misc::DirFSPath() / compilerPath;
        const auto stem = canon.stem().string();
        const auto dir = canon.parent_path();
        

        module.addSearchPath(dir);
        
        for (const auto& pre : {""s, "lib"s})
        {
            for (const auto& post : {""s, ".dll"s, ".so"s, ".dylib"s, ".bundle"s})
            {
                auto lib = dir / (pre + stem + post);
                
                if(!cpl::fs::exists(lib) || cpl::fs::is_directory(lib))
                    continue;
                
				std::string errorMsg;
				
                if(auto error = module.load(lib.string(), errorMsg))
                {
                    auto fmt = cpl::format(
                        "Error loading compiler module \'%s\' for language \'%s\' at \'%s\'. OS returns %d. System message:\n%s",
                        compilerName.c_str(),
                        language.c_str(),
                        lib.string().c_str(),
                        (int)error,
						errorMsg.c_str()
                    );
                    
                    throw std::runtime_error(std::move(fmt));
                }
                
                bindings.loadBindings(module, languageSettings["exports"]);

                return;
            }
        }
        
        auto fmt = cpl::format(
            "Error loading compiler module \'%s\' for language \'%s\' at canonical path: \'%s\'.",
            compilerName.c_str(),
            language.c_str(),
            canon.string().c_str()
        );
        
        throw std::runtime_error(std::move(fmt));
	}

};
