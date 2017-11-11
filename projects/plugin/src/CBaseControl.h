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

	file:CBaseControl.h
		
		CBaseControl - base class for all implementable controls.
		all methods prefixed with 'b' to avoid future nameclashing.
		This class provides a common interface for all 'controls' to support
		normalized value-, string and position get/set.
		Also encapsulates listeners into a single one.
		Provides a cheap RAII mutex lock as well, as well as optional reference counting.
		Does not derive from the system's base class for graphics to avoid the diamond
		problem (classes deriving from this should set the base class in their constructor)

*************************************************************************************/

#ifndef _CBASECONTROL_H
	#define _CBASECONTROL_H

	#include <cpl/CMutex.h>
	#include "Common.h"

	namespace ape
	{
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

		// reference counter used for all controls
		typedef CReferenceCounter2 refCounter;


		class CBaseControl 
		: 
			public refCounter,
			public cpl::CMutex::Lockable,
			public juce::Slider::Listener, 
			public juce::Button::Listener,
			public juce::ScrollBar::Listener
		{
		public:

			/*********************************************************************************************

				A CBaseControl can call a listener back on events.
				see bSetListener
				A listener can override the controls own event handler by returning true from valueChanged
				A return value of false will cause the control to handle the event itself.

			*********************************************************************************************/
			class CListener
			{
			public:
				virtual bool valueChanged(CBaseControl *) = 0;
				virtual ~CListener() {};
			};
		protected:
			/*
				All controls have a unique associated tag.
			*/
			long tag;
			/*
				Whether the control is attached to anything
			*/
			bool isAttached; 
			/*
				The implementation specific context of controls
			*/
			GraphicComponent * base;
			/*
				The callback listener
			*/
			CListener * list;
			
		public:
			/*********************************************************************************************

				Returns whether the control is attached to anything

			*********************************************************************************************/
			bool bIsAttached() 
			{ 
				return isAttached; 
			}
			/*********************************************************************************************

				Constructor

			*********************************************************************************************/
			CBaseControl(GraphicComponent * b) 
				: base(b), isAttached(false), tag(0), list(nullptr)
			{
			
			}
			/*********************************************************************************************

				Constructor

			*********************************************************************************************/
			CBaseControl(GraphicComponent * b,  int tag, bool bIsAttached = false) 
				: base(b), isAttached(bIsAttached), tag(tag), list(nullptr)
			{
			
			}
			/*********************************************************************************************

				Virtual destructor, of course

			*********************************************************************************************/
			virtual ~CBaseControl()
			{
			}
			/*********************************************************************************************

				Set visibility of control

			*********************************************************************************************/
			void bSetVisible(bool bVisibility)
			{
				if(base)
					base->setVisible(bVisibility);
			}
			/*********************************************************************************************

				Attach to a parent 

			*********************************************************************************************/
			virtual void addToParent(GraphicComponent * parent)
			{
				parent->addChildComponent(base);
			}
			/*********************************************************************************************

				Remove from a parent

			*********************************************************************************************/
			virtual void removeFromParent(GraphicComponent * parent)
			{
				parent->removeChildComponent(base);
			}
			/*********************************************************************************************

				Gets the internal value of the control.
				Ranges from 0.0 to 1.0f

			*********************************************************************************************/
			virtual float bGetValue()
			{ 
				return 0.f;
			}
			/*********************************************************************************************

				Sets the value of the control.
				Input must be between 0.0f and 1.0f, inclusive

			*********************************************************************************************/
			virtual void bSetValue(float val) 
			{

			}
			/*********************************************************************************************

				Sets the internal value of the control.
				Input must be between 0.0f and 1.0f, inclusive.
				Guaranteed to have no sideeffects (ie. doesn't call listeners)

			*********************************************************************************************/
			virtual void bSetInternal(float val)
			{
			
			}
			/*********************************************************************************************

				Sets the title of a control.

			*********************************************************************************************/
			virtual void bSetTitle(const std::string & text)
			{
			
			}
			/*********************************************************************************************

				Gets the title of a control

			*********************************************************************************************/
			virtual std::string bGetTitle()
			{ 
				return ""; 
			}
			/*********************************************************************************************

				Sets the text of the control

			*********************************************************************************************/
			virtual void bSetText(const std::string & text) 
			{
			
			}
			/*********************************************************************************************

				Returns the text of a control

			*********************************************************************************************/
			virtual const std::string bGetText() 
			{ 
				return ""; 
			}
			/*********************************************************************************************

				Returns the associated tag with this control

			*********************************************************************************************/
			virtual int bGetTag() 
			{ 
				return tag; 
			}
			/*********************************************************************************************

				Sets the tag of this control

			*********************************************************************************************/
			virtual void bSetTag(int newTag) 
			{
				tag = newTag;
			}
			/*********************************************************************************************

				Returns the size of this control.
				X and Y are absolute coordinates.

			*********************************************************************************************/
			virtual CRect bGetSize()
			{
				if (base)
					return base->getBounds();
				return CRect();
			}
			/*********************************************************************************************

				Sets the x, y position of this control

			*********************************************************************************************/
			virtual void bSetPos(int x, int y)
			{
				if (base)
					base->setBounds(x, y, base->getWidth(), base->getHeight());
			}
			/*********************************************************************************************

				Sets the size of this control

			*********************************************************************************************/
			virtual void bSetSize(const CRect & size)
			{
				if (base)
					base->setBounds(size);
				// set pos?
			}
			/*********************************************************************************************

				Called before any repainting is done.
				If a control wishes to repaint itself, it should call the relevant repaint command
				(usually base->repaint())

			*********************************************************************************************/
			virtual void bRedraw() 
			{ 
			
			}
			/*********************************************************************************************

				Internal use

			*********************************************************************************************/
			void setDirty()
			{
				bRedraw();
			}
			/*********************************************************************************************

				Sets the listener to be called back on value changes

			*********************************************************************************************/
			void bSetListener(CListener * listener)
			{
				list = listener;
			}
			/*********************************************************************************************

				Force a valueChanged event, and a callback

			*********************************************************************************************/
			virtual void bForceEvent()
			{

				if (list && list->valueChanged(this))
					return;
				onValueChange();
			}
			/*********************************************************************************************

				The control's internal event callback. This is called whenever the controls value is changed.

			*********************************************************************************************/
			virtual void onValueChange() 
			{

			}
			#ifdef APE_JUCE
			protected:
				/*
					some layers of indirection is needed here, since juce doesn't
					share the same class for listeners.
					the base object works as a proxy that turns all the events
					into cbasecontrol listener callbacks.
				*/

				virtual void buttonClicked(juce::Button * c)
				{
					// check if theres a listener?
					// if, check if they handle the event
					if (list && list->valueChanged(this))
						// if, return early
						return;
					// else: let the control do the formatting
					onValueChange();
				}
				virtual void sliderValueChanged(juce::Slider * c)
				{
					if (list && list->valueChanged(this))
						return;

					onValueChange();
				}
				virtual void scrollBarMoved(juce::ScrollBar * c, double newRange)
				{
					if (list && list->valueChanged(this))
						return;
					onValueChange();
				}
			#endif
		};

		#ifdef APE_VST
			typedef CControlListener CCtrlListener;
		#elif defined(APE_JUCE)
			typedef CBaseControl::CListener CCtrlListener;
		#endif
	}
#endif