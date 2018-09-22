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
 
	 file:GraphicComponents.h
		
		Wrappers and graphic classes that can be used for controls, images, etc.
	
 *************************************************************************************/

#ifndef _GRAPHICCOMPONENTS_H
	#define _GRAPHICCOMPONENTS_H

	#include "Common.h"
	#include <map>
	#include <mutex>

	namespace ape
	{

		/*********************************************************************************************

			RAII wrapper around images, loaded at runtime

		*********************************************************************************************/
		class CImage
		{
			std::string path;
			juce::Image internalImage;
			juce::DrawableImage drawableImage;
		public:
			CImage(const std::string & inPath);
			CImage();
			void setPath(const std::string & inPath);
			bool load();
			juce::Image & getImage();
			juce::Drawable * getDrawable();
			virtual ~CImage();
		};
		/*********************************************************************************************

			Manages all resources used by this program, statically

		*********************************************************************************************/
		class CResourceManager
		{
		private:
			std::map<std::string, CImage> resources;
			bool isResourcesLoaded;
			CResourceManager();

		public:
			juce::Drawable * operator [] (const std::string & name);
			bool loadResources();
			// singleton instance
			static juce::Drawable * getResource(const std::string & name);
			static juce::Drawable * getCopyOfDrawable(const std::string & name);
			static const juce::Image & getImage(const std::string & name);
			static CResourceManager & instance();

		};

		/*********************************************************************************************

			Textlabel interface

		*********************************************************************************************/
		class CTextLabel : public juce::Component
		{
		protected:
			juce::String text;
			float size;
			CColour colour;
			juce::Justification just;
			juce::String fontName;
		public:
			CTextLabel();
			void setFontName(const juce::String& name);
			void setFontSize(float newSize);
			void setColour(CColour newColour);
			virtual void setText(const std::string & newText);
			virtual void paint(juce::Graphics & g) override;
			void setPos(int x, int y);
			void setJustification(juce::Justification j) { just = j; }
		};
		/*********************************************************************************************

			Greenlinetester. Derive from this if you are uncertain that you are getting painted -
			Will draw a green line.

		*********************************************************************************************/
		class CGreenLineTester : public juce::Component
		{
			void paint(juce::Graphics & g)
			{
				g.setColour(juce::Colours::green);
				g.drawLine(juce::Line<float>(0.f, 0.f, static_cast<float>(getWidth()), static_cast<float>(getHeight())),1);
			}
		};
		/*********************************************************************************************

			Name says it all. Holds a virtual container of larger size, that is scrollable.

		*********************************************************************************************/
		class CScrollableContainer : public juce::Component, juce::ScrollBar::Listener
		{
		protected:
			juce::ScrollBar * scb;
			Component * virtualContainer;
			const juce::Image * background;
		public:
			CScrollableContainer();
			void bSetSize(const CRect & in);
			int getVirtualHeight();
			void setVirtualHeight(int height);
			void bSetValue(float newVal);
			float bGetValue();
			void setBackground(const juce::Image * b) { background = b;}
			void setBackground(const juce::Image & b) {	background = &b; }
			juce::ScrollBar * getSCB() { return scb; }
			juce::Component * getVContainer() { return virtualContainer; }
			void scrollBarMoved(juce::ScrollBar * b, double newRange) override;
			virtual void paint(juce::Graphics & g) override;
			virtual ~CScrollableContainer();

		};
		/*********************************************************************************************

			Same as CTextLabel, however is protected using a mutex

		*********************************************************************************************/
		class CTextControl : public CTextLabel, private std::mutex
		{
		public:
			void bSetText(const std::string & newText);
			const std::string bGetText();
			void paint(juce::Graphics & g) override;

		};
	};
#endif