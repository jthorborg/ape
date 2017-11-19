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

	file:CControlManager.h
		
		Declares the interface for the manager of the plugin-associated controls, 
		classes for controlling the grid and ultimately the interface for creating
		user-controls.

*************************************************************************************/

#ifndef APE_CCONTROL_MANAGER_H
	#define APE_CCONTROL_MANAGER_H

	#include <ape/APE.h>
	#include "Common.h"
	#include <list>
	#include <queue>
	#include <map>
	#include <vector>
	#include "UserControls.h"

	namespace ape 
	{

		class Engine;
		class CConsole;

		/*********************************************************************************************

			Responsible for managing positions of controls and a grid

		*********************************************************************************************/
		class CGridManager 
		{
			CGridManager & operator =(const CGridManager &);
		public:
			const int _grid_side;
			CPoint pos;
			CRect size;
			std::vector<std::vector<bool>> grids;
			bool full();
			bool empty();
			void rectFromXY(CRect & _in, int x, int y);
			std::pair<int,int> xyFromRect(const CRect & in);
			CRect getFirstEmpty();
			bool removeGrid(const CRect & in);
			CGridManager(const CRect & size);
			void clear();
		}; 
		/*********************************************************************************************

			The interface for managing controls.

		*********************************************************************************************/
		class CControlManager 
		{
		protected:
			std::list<CBaseControl*> _controls;
			std::queue<int> unusedTags;
			juce::Component * parent;
			CGridManager gridManager;
			std::vector<CBaseControl*> pendingControls;
			int tagCounter;
		public:

			CControlManager(const CRect & _size, int nTags);
			void createPendingControls();
			void detach();
			void removeControls();
			void attach(GraphicComponent  * frame);
			void addControl(CBaseControl* _ctrl);
			CBaseControl * getControl(int nTag);
			void deleteControl(CBaseControl * _ctrl);
			void deleteControls();
			void setParent(GraphicComponent * frame);
			const std::list<CBaseControl *> & getControls() { return _controls; }
			virtual void reset();
			// injects a event and calls all listeners to it
			// gives listeners a chance to edit the control
			void callListeners();
			// issues a redraw to all controls
			void updateControls();
			int getNextTag();
			virtual ~CControlManager();
		};

		/*********************************************************************************************

			Holds a group of toggles. Toggles are grouped together to conserve space and relevance.

		*********************************************************************************************/
		template < typename Ty = CToggle *>
			class CToggleGroup
			{
				typedef Ty type;
				std::vector<type> toggles;
				int max_elements;
			public:
				CToggleGroup(int size = 3) : max_elements(size) {};
				void add(const type & ins) { if(!isFull()) toggles.push_back(ins); }
				bool isFull() { return (max_elements == getSize()); }
				void setFull() { max_elements = getSize(); }
				int getSize() { return static_cast<int>(toggles.size()); }
				int getMax() { return max_elements; }
				void erase(const type & el)
				{
					for(auto it = toggles.begin(); it != toggles.end(); ++it)
						if((*it) == el)
							toggles.erase(it);
				}
				CRect pos;
			};
		/*********************************************************************************************

			The class that manages plugin controls, holds implementation details about available control types.
			The interface for adding user controls.

		*********************************************************************************************/
		class CPluginCtrlManager : public CControlManager
		{
			CBaseControl::CListener * list;
			std::vector<CToggleGroup<>> togglegroups;
		public:
			// all controls in VST's have tags (so they can be automated). Our parent gives us an tag, from which it is presumed
			// the parent wont use. All controls > tag are ours.
			CPluginCtrlManager(CBaseControl::CListener * list, const CRect & _size, int nControlTagStart);
			int addKnob(const char * name, float * val = nullptr, CKnobEx::type _type = CKnobEx::type::pct);
			int addKnob(const char * name, float * val, char * values, char * unit);
			int addLabel(const char * name, const char * fmt, va_list args);
			int addMeter(const char * name, float * extVal);
			int addToggle(const char * name, float * extVal);
			int addPlot(const char * name, const float * const vals, unsigned int numVals);
			int addKnob(const char * name, const char * unit, float * val, ape::ScaleFunc cb, float min, float max);
			
			void reset() override;
		};
	};
#endif