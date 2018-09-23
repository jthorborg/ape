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
 
	file:SignalizerWindow.cpp
	
		Implementation of the API functions.

*************************************************************************************/

#include <cpl/Common.h>
#include "SignalizerWindow.h"
#include "Common/SignalizerDesign.h"
#include "Engine.h"

namespace ape 
{

	class SignalizerWindow : public juce::Component
	{
	public:

		SignalizerWindow(OscilloscopeData& engine);
		~SignalizerWindow();

		void resized() override;

		void visibilityChanged() override
		{
			if (!scope)
			{
				scope = std::make_unique<Signalizer::Oscilloscope>(data.getBehaviour(), data.getName(), data.getStream(), &data.getContent());
				addAndMakeVisible(scope.get());
				scope->attachToOpenGL(context);

			}
		}

	private:
		OscilloscopeData& data;
		std::unique_ptr<Signalizer::Oscilloscope> scope;
		juce::OpenGLContext context;
	};


	SignalizerWindow::SignalizerWindow(OscilloscopeData& oData)
		: juce::Component("Oscilloscope")
		, data(oData)
	{
		setVisible(false);

		context.setMultisamplingEnabled(true);

		juce::OpenGLPixelFormat format;
		format.multisamplingLevel = 16;
		context.setPixelFormat(format);

	}

	void SignalizerWindow::resized()
	{
		if(scope)
			scope->setSize(getWidth(), getHeight());
	}

	SignalizerWindow::~SignalizerWindow()
	{
		if(scope)
			scope->detachFromOpenGL(context);
	}

	std::unique_ptr<juce::Component> OscilloscopeData::createEditor()
	{
		auto component = getContent().createEditor();
		component->setName("Scope settings");
		return std::unique_ptr<juce::Component>{ component.release() };
	}

	std::unique_ptr<juce::Component> OscilloscopeData::createWindow()
	{
		auto component = std::make_unique<SignalizerWindow>(*this);

		return std::unique_ptr<juce::Component>{ component.release() };
	}

}