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

	file:CLangCodeTokeniser.h
		
		Tokeniser for different C-style language

*************************************************************************************/

#ifndef APE_CODETOKENISER_H
	#define APE_CODETOKENISER_H

	#include <cpl/Common.h>
	#include "../Settings.h"

	namespace ape
	{

		/// <summary>
		/// Reimplementation of juce::CPPCodeTokeniser
		/// </summary>
		class CLangCodeTokeniser : public juce::CodeTokeniser
		{
		public:
			CLangCodeTokeniser(const Settings& settings);

			int readNextToken(juce::CodeDocument::Iterator&) override;
			juce::CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

			static bool isReservedKeyword(const juce::String& token) noexcept;

		private:

			const Settings& settings;
		}; 

	}
#endif