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

	file:UserControls.h
	
		This file implements interfaces for user-constructable controls, all of which
		derive from CBaseControl.

*************************************************************************************/


#ifndef _USERCONTROLS_H
	#define _USERCONTROLS_H

	#include <vector>
	#include <string>
	#include <cpl/Misc.h>
	#include "CApi.h"
	#include "GraphicComponents.h"

	namespace ape {

		// constants
		#define hzLimit 8000.f
		#define msLimit 1000

		/*********************************************************************************************

			CLabelDisplay - a label showing a text with parameters and a title.

		 *********************************************************************************************/
		class CLabelDisplay : public GraphicComponent, public CBaseControl
		{
			std::string title;
			CTextLabel * labels[2];
		public:
			CLabelDisplay(const CRect & _where);
			void bSetTitle(const std::string & in);
			void setFormat(const char * fmt, va_list args);
			virtual ~CLabelDisplay();
			virtual void bRedraw();

		};
		/*********************************************************************************************

			CCheckBox - an on and off button, implemented as an checkbox.

		 *********************************************************************************************/
		typedef CToggle CCheckBox;
		/*********************************************************************************************

			CKnobEx - an automatable knob which can display different units depending on type.

		 *********************************************************************************************/
		class CKnobEx : public CKnob
		{

		public:
			enum type {
				pct,
				hz,
				db,
				ft,
				ms
			};

		protected:
			type _type;
			float * pVal;
		public:

			CKnobEx(const CRect & _where, const char * name = NULL, type _type = pct, float * extVal = nullptr);
			virtual void onValueChange();
		};
		/*********************************************************************************************

			CValueKnobEx - an extended knob that shows a list of values instead.

		 *********************************************************************************************/
		class CValueKnobEx : public CKnobEx
		{
			std::vector<std::string> _values;
			std::string _unit;
		public:
			CValueKnobEx(const CRect & _where, const char * name, float * extVal, char * values, char * unit);
			virtual void onValueChange();
		};
		/*********************************************************************************************

			CTransparentBitmap - a CBitmap that forces transparent drawing.
			After vstgui v. 4 this is redundant (bitmaps automatically draws alpha channel)

		 *********************************************************************************************/
		#ifdef APE_VST
			#if VSTGUI_VERSION_MAJOR < 4
				class CTransparentBitmap : public CBitmap
				{
					virtual void draw(CDrawContext *context, CRect &rect, const CPoint & offset = CPoint (0, 0)) {
						drawTransparent(context, rect, offset);
					}
				public:
					CTransparentBitmap(long resourceID) : CBitmap(resourceID) 
					{
						
					};
				};
			#else
				typedef CBitmap CTransparentBitmap;
			#endif
			/*********************************************************************************************

				CImage - simple interface between CBitmap and CView, with an optional (negative) vertical
				offset to the image.

			 *********************************************************************************************/
			class CImage : public CView
			{
			protected:
				CBitmap * image;
				long offset;
			public:
				CImage(long resourceID, const CRect & size);
				void setOffset(long off);
				// overload CView::draw to create an offset to the painted image.
				void draw(CDrawContext *pContext);
				virtual ~CImage();
			};
		#endif
		/*********************************************************************************************

			CMeterDisplay - displays a bar that moves according to it's value.

		 *********************************************************************************************/
		class CMeterDisplay : public juce::Component, public CBaseControl
		{
		protected:
			CTextLabel * title;
			const juce::Image & meter;
			float * val;
			double pole;
			double smoothVal;
		public:
			CMeterDisplay(const CRect & _where,float * extVal, const char * name);
			void bRedraw();
			void paint(juce::Graphics & g);
			~CMeterDisplay() noexcept;
		};
		/*********************************************************************************************

			CToggle - basically a button, may use any graphic fit

		 *********************************************************************************************/
		class CToggleCtrl : public CToggle
		{
		protected:
			float * extVal;
		public:
			CToggleCtrl(const CRect & _where, float * extVal, const char * name);
			
			virtual void onValueChange();
		};
		/*********************************************************************************************

			CPlots - Plots a range of values into a small graph.

		 *********************************************************************************************/
		class CPlot : public GraphicComponent,  public CBaseControl
		{
		protected:
			CPoint * points;
			unsigned numPoints;
			const float * const values;
			CRect plotSize;
			double horzScale;
			CTextLabel * title;
			juce::PathStrokeType pst;
		public:
			CPlot(const CRect & _where, const char * name,
				const float * const values, const int numValues);
			void paint(juce::Graphics & g);
			void bRedraw() override;
			~CPlot();
		};

		/*********************************************************************************************

		CRangeKnob - an automatable knob that formats it's value using a callback

		*********************************************************************************************/
		class CRangeKnob : public CKnobEx {
			ape::ScaleFunc extScale;
			char unit[10];
			float _min, _max;

		public:

			CRangeKnob(const CRect & _where, const char * name, const char * unit, float * extVal, ape::ScaleFunc scaleCB, float _min, float _max);

			virtual void onValueChange();
		};

	};
#endif