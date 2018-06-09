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
 
	 file:CodeTokeniser.cpp
	 
		Implementation of code tokeniser
 
 *************************************************************************************/


#include "CodeTokeniser.h"

namespace ape
{

	int CodeTokeniser::readNextToken(juce::CodeDocument::Iterator& source)
	{
		return juce::CppTokeniserFunctions::readNextToken(source);
	}

	juce::CodeEditorComponent::ColourScheme CodeTokeniser::getDefaultColourScheme()
	{
		struct Type
		{
			const char* name;
			juce::Colour colour;
		};
		0.0f;
		static const Type types[] =
		{
			{ "Error", juce::Colours::darkred },
			{ "Comment", juce::Colours::green },
			{ "Keyword", juce::Colour(105, 173, 238) },
			{ "Operator", { 166, 206, 46 }},
			{ "Identifier", {0xC8, 0xC8, 0xC8}},
			{ "Integer",{ 0x8E, 0x9E, 0xB3 } },
			//{ "Float", { 220, 128, 128 } },
			{ "Float", { 0x9E, 0x8E, 0xB3 } },
			{ "String", { 0xD6, 0x9D, 0x85 } },
			{ "Bracket", { 0xcc, 0xb7, 0xc0 } },
			{ "Punctuation", { 0xc9, 0xe6, 0xbd} },
			{ "Preprocessor Text", juce::Colours::darkolivegreen }
		};


		juce::CodeEditorComponent::ColourScheme cs;

		for (auto& type : types)
			cs.set(type.name, type.colour);

		return cs;
	}

	bool CodeTokeniser::isReservedKeyword(const juce::String& token) noexcept
	{
		return juce::CppTokeniserFunctions::isReservedKeyword(token.getCharPointer(), token.length());
	}

}