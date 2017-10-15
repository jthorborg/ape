/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:vstguiex.h
	
		This file implements interfaces to most controls used in this program, especially
		user-constructable controls. Most are extended VSTGUI controls.

*************************************************************************************/


#ifndef _VSTGUIEX_H
	#define _VSTGUIEX_H

	//#include "vstgui.h"
	#include <vector>
	#include <string>
	#include "Misc.h"
	#include "CApi.h"
	#include "CValueLabel.h"

	namespace VSTGUI {

		// constants
		#define hzLimit 8000.f
		#define msLimit 1000
		// forward declarations
		class CBaseControl;

		//-----------------------------------------------------------------------------
		// CReferenceCounter Declaration (Reference Counting)
		// This class is from VSTGUI 3.6 (removed in 4+)
		//-----------------------------------------------------------------------------
		class CReferenceCounter2
		{
		public:
			CReferenceCounter2() : nbReference(1) {}
			virtual ~CReferenceCounter2() {}

			virtual void forget() { nbReference--; if (nbReference == 0) delete this; }
			virtual void remember() { nbReference++; }
			long getNbReference() const { return nbReference; }
	
		private:
			long nbReference;
		};

		/*********************************************************************************************

			CCtrlNotifier -- deprecated

		 *********************************************************************************************/
		class CCtrlNotifier
		{
			public: virtual APE::Status plugCtrlChanged(CDrawContext* context, CControl* control) = 0;
		};

		// reference counter used for all controls
		typedef CReferenceCounter2 refCounter;

		/*********************************************************************************************

			CBaseControl - base class for all user-implementable controls.

		 *********************************************************************************************/
		class CBaseControl : public refCounter
		{
		protected:
			long tag; // tag
			bool bIsAttached; // is attached to something?
			CRect size; // size of control
		public:
			bool isAttached() { return bIsAttached; }
			CBaseControl() : bIsAttached(false) {}
			CBaseControl(const CRect & rect, long tag, bool bIsAttached = false) 
				: bIsAttached(bIsAttached), size(rect), tag(tag) {}
			virtual float getValue() { return 0.f;};
			virtual void setValue(float val) {};
			virtual void setTitle(const char * text) {};
			virtual void getTitle(char * text) {};
			virtual void setText(const char * text) {};
			virtual void getText(char * text) {};
			virtual long getTag() { return tag; };
			virtual CRect getSize() { return size; };
			virtual void addToParent(CFrame * frame) = 0;
			virtual void removeFromParent(CFrame * frame) = 0;
			virtual ~CBaseControl() {}
			virtual bool resize(const CRect & in) { return false; }
			virtual void valueChanged(CControl* control) {};
			virtual void update() {}; // alias to setDirty() in most implementations
		};
		/*********************************************************************************************

			CLabelDisplay - a label showing a text with parameters and a title.

		 *********************************************************************************************/
		class CLabelDisplay : public CBaseControl
		{
			APE::CValueLabel text;
			std::string title;
			CTextLabel * labels[2];
		public:
			CLabelDisplay(const CRect & _where, long tag);
			void setTitle(const char * name);
			void setFormat(const char * fmt, va_list args);
			void addToParent(CFrame * frame);
			void removeFromParent(CFrame * frame);

			virtual void update();

		};
		/*********************************************************************************************

			CCheckBox - an on and off button, implemented as an checkbox.

		 *********************************************************************************************/
		class CCheckBoxButton : public COnOffButton
		{
			CRect _size;
			const int sizeofline;
			CTextLabel * name;
		public:
			CCheckBoxButton(const CRect & size, const char * _name, CControlListener * l, long nTag, CBitmap * chk);
			CRect getSize();
			CView * getText();
			void addToParent(CFrame * frame)
			{
				frame->addView(this);
				frame->addView(name);
			}
			/*********************************************************************************************/
			void removeFromParent(CFrame * frame)
			{
				frame->removeView(this, false);
				frame->removeView(name, false);
			}
		};
		/*********************************************************************************************

			CKnobEx - an automatable knob which can display different units depending on type.

		 *********************************************************************************************/
		class CKnobEx : public CBaseControl {

		public:
			enum type {
				pct,
				hz,
				db,
				ft,
				ms
			};

		protected:
			CBitmap * _circle;
			CBitmap * _indicator;
			CKnob * _knob;
			CTextLabel * labels[2];
			type _type;
			float * pVal;
			std::vector<CView*> views;
		public:

			CKnobEx(CControlListener * list, const CRect & _where, long tag, CBitmap * _indicator, CBitmap * _circle, const char * name = NULL, type _type = pct, float * extVal = NULL);
			void addToParent(CFrame * frame);
			void removeFromParent(CFrame * frame);
			CView * getView();
			virtual float getValue();
			virtual void setValue(float val);
			virtual void setText(const char * text);
			virtual void getText(char * text);
			virtual void valueChanged(CControl* control);
			virtual void update();
			virtual ~CKnobEx();
		};
		/*********************************************************************************************

			CValueKnobEx - an extended knob that shows a list of values instead.

		 *********************************************************************************************/
		class CValueKnobEx : public CKnobEx
		{
			std::vector<std::string> _values;
			std::string _unit;
		public:
			CValueKnobEx(CControlListener * list, const CRect & _where, long tag, CBitmap * _indicator, CBitmap * _circle, 
				const char * name, float * extVal, char * values, char * unit);
			virtual void valueChanged(CControl * control);
		};
		/*********************************************************************************************

			CTransparentBitmap - a CBitmap that forces transparent drawing.
			After vstgui v. 4 this is redundant (bitmaps automatically draws alpha channel)

		 *********************************************************************************************/
		#if VSTGUI_VERSION_MAJOR < 4
			class CTransparentBitmap : public CBitmap
			{
				virtual void draw(CDrawContext *context, CRect &rect, const CPoint & offset = CPoint (0, 0)) {
					drawTransparent(context, rect, offset);
				}
			public:
				CTransparentBitmap(long resourceID) : CBitmap(resourceID) 
				{
					::
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
		/*********************************************************************************************

			CMeterDisplay - displays a bar that moves according to it's value.

		 *********************************************************************************************/
		class CMeterDisplay : public CBaseControl 
		{
		protected:
			CTextLabel * text;
			CImage * meter;
			float * val;
			double pole;
			double smoothVal;
		public:
			CMeterDisplay(const CRect & _where, long tag, float * extVal, const char * name);
			void addToParent(CFrame * frame);
			void removeFromParent(CFrame * frame);
			void update();
			~CMeterDisplay();
		};
		/*********************************************************************************************

			CToggle - basically a button, may use any graphic fit

		 *********************************************************************************************/
		class CToggle : public CBaseControl
		{
		protected:
			CCheckBoxButton * _button;
			CTextLabel * _text;
			float * extVal;
		public:
			CToggle(const CRect & _where, CControlListener * lst, long tag, CBitmap * chk, float * extVal, const char * name);
			
			void addToParent(CFrame * frame);
			void removeFromParent(CFrame * frame);
			void update();
			virtual void valueChanged(CControl * control);
			~CToggle();
		};
		/*********************************************************************************************

			CPlots - Plots a range of values into a small graph.

		 *********************************************************************************************/
		class CPlot : public CBaseControl
		{
		protected:
			class CPlotView;
			friend class CPlotView;
			CPoint * points;
			unsigned numPoints;
			const float * const values;
			class CPlotView : public CView
			{
				CPlot * base;
			public:
				CPlotView(CPlot * base, const CRect & size) :  CView(size), base(base) {};
				virtual void draw(VSTGUI::CDrawContext * ctx);
			};

			CPlotView * plot;
			CTextLabel * title;
			
		public:
			CPlot(const CRect & _where, long tag, const char * name,
				const float * const values, const int numValues);

			void addToParent(CFrame * frame);
			void removeFromParent(CFrame * frame);
			void update();
			~CPlot();
		};

		/*********************************************************************************************

		CRangeKnob - an automatable knob that formats it's value using a callback

		*********************************************************************************************/
		class CRangeKnob : public CKnobEx {
			APE::ScaleFunc extScale;
			char unit[10];
			float _min, _max;

		public:

			CRangeKnob(CControlListener * list, const CRect & _where, long tag, CBitmap * _indicator, CBitmap * _circle, const char * name, 
				const char * unit, float * extVal, APE::ScaleFunc scaleCB, float min, float max);

			virtual void valueChanged( CControl* control);
		};

	};
#endif