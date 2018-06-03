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
	struct Type
	{
		const char* name;
		juce::Colour colour;
	};

	const Type types[] =
	{
		{ "Error", juce::Colours::darkred },
		{ "Comment", juce::Colours::green },
		{ "Keyword", juce::Colours::blue },
		{ "Operator", juce::Colours::darkred },
		{ "Identifier", juce::Colours::black },
		{ "Integer", juce::Colours::black },
		{ "Float", juce::Colours::black },
		{ "String", juce::Colours::grey },
		{ "Bracket", juce::Colours::darkred },
		{ "Punctuation", juce::Colours::darkred },
		{ "Preprocessor Text", juce::Colours::darkolivegreen }
	};

	int CodeTokeniser::readNextToken(juce::CodeDocument::Iterator& source)
	{
		return juce::CppTokeniserFunctions::readNextToken(source);
	}

	juce::CodeEditorComponent::ColourScheme CodeTokeniser::getDefaultColourScheme()
	{
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