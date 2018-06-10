/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2018 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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
 
	 file:CLangCodeTokeniser.cpp
	 
		Implementation of code tokeniser for C++
 
 *************************************************************************************/

#include "CLangCodeTokeniser.h"
#include "../Settings.h"

namespace ape
{

	static juce::CodeEditorComponent::ColourScheme GetColourScheme(const cpl::string_ref language, const ape::Settings& settings)
	{
		juce::CodeEditorComponent::ColourScheme cs;

		auto set = [&](juce::String name, juce::Colour def)
		{
			auto colour = settings.lookUpValue(def,
				"languages",
				language.c_str(),
				"syntax_highlight",
				name.toLowerCase().removeCharacters(" ").getCharPointer().getAddress()
			);

			cs.set(name, colour);
		};

		set("Error", juce::Colours::darkred);
		set("Comment", juce::Colours::green);
		set("Keyword", juce::Colour(105, 173, 238));
		set("Operator",{ 166, 206, 46 });
		set("Identifier",{ 0xC8, 0xC8, 0xC8 });
		set("Integer",{ 0x8E, 0x9E, 0xB3 });
		set("Float",{ 0x9E, 0x8E, 0xB3 });
		set("String",{ 0xD6, 0x9D, 0x85 });
		set("Bracket",{ 0xcc, 0xb7, 0xc0 });
		set("Punctuation",{ 0xc9, 0xe6, 0xbd });
		set("Preprocessor Text", juce::Colours::darkolivegreen);

		return cs;
	}


	CLangCodeTokeniser::CLangCodeTokeniser(const Settings& settings)
		: settings(settings)
	{
	}

	int CLangCodeTokeniser::readNextToken(juce::CodeDocument::Iterator& source)
	{
		return juce::CppTokeniserFunctions::readNextToken(source);
	}

	juce::CodeEditorComponent::ColourScheme CLangCodeTokeniser::getDefaultColourScheme()
	{
		// TODO: Differentiate between C and C++
		return GetColourScheme("cpp", settings);
	}

	bool CLangCodeTokeniser::isReservedKeyword(const juce::String& token) noexcept
	{
		return juce::CppTokeniserFunctions::isReservedKeyword(token.getCharPointer(), token.length());
	}

}