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

	file:CControlManager.cpp
	
		Implementation of the controlmanager, it's derivates and the gridmanager.

*************************************************************************************/

#include "CControlManager.h"

#include "Engine.h"

namespace ape {

	/*********************************************************************************************

	 	Tests the capacity of the container.

	 *********************************************************************************************/
	bool CGridManager::full() 
	{
		for(size_t x = 0; x < grids.size(); ++x) {
			for(size_t y = 0; y < grids[x].size(); ++y) {
				if(!grids[x][y])
					return false;
			}
		}
		return true;
	}
	/*********************************************************************************************

	 	Tests if the container is empty.

	 *********************************************************************************************/
	bool CGridManager::empty () 
	{
		for(size_t x = 0; x < grids.size(); ++x) {
			for(size_t y = 0; y < grids[x].size(); ++y) {
				if(grids[x][y])
					return false;
			}
		}
		return true;
	}
	/*********************************************************************************************

	 	Fills a CRect from a given [x,y] set of coordinates.

	 *********************************************************************************************/
	void CGridManager::rectFromXY(CRect & _in, int x, int y) 
	{
		CRect temp(size.getX() + x * _grid_side,
			size.getY() + y * _grid_side,
			_grid_side,
			_grid_side
		);
		_in = temp;
	}
	/*********************************************************************************************

	 	Returns a pair of coordinates from a rectangle.

	 *********************************************************************************************/
	std::pair<int,int> CGridManager::xyFromRect(const CRect & in)
	{
		int x = (in.getX() - size.getX()) / _grid_side;
		int y = (in.getY() - size.getY()) / _grid_side;
		return std::pair<int,int> (x,y);
	}
	/*********************************************************************************************

	 	Returns the first empty rectangle and reserves it (marks it as used).

	 *********************************************************************************************/
	CRect CGridManager::getFirstEmpty() 
	{
		CRect ret;
		for(unsigned y = 0; y < grids[0].size(); ++y) {
			for(unsigned x = 0; x < grids.size(); ++x) {
				if(!grids[x][y]) {

					rectFromXY(ret, x, y);
					grids[x][y] = true;
					return ret;
				}
			}
		}
		return ret;
	}
	/*********************************************************************************************

	 	Removes a grid and marks it as unused.

	 *********************************************************************************************/
	bool CGridManager::removeGrid(const CRect & in)
	{
		auto coords = xyFromRect(in);
		grids[coords.first][coords.second] = false;
		return true;
	}
	/*********************************************************************************************

	 	Constructor - initializes the grids from a size.

	 *********************************************************************************************/
	 CGridManager::CGridManager(const CRect & size) 
		 : size(size),
		 _grid_side(80) // side of a control. they are squares.
	 {
		int nRows = size.getWidth() / _grid_side;
		int nColumns = size.getHeight() / _grid_side;
		// resize the array so it fits. must at least be 1x1
		grids.resize(nRows ? nRows : 1, std::vector<bool>(nColumns ? nColumns : 1, false));
	}
	/*********************************************************************************************

	 	Marks all grids as unused.

	 *********************************************************************************************/
	 void CGridManager::clear() 
	 {
		for(size_t x = 0; x < grids.size(); ++x) {
			for(size_t y = 0; y < grids[x].size(); ++y) {
				grids[x][y] = false;
			}
		}		 
	 }

	//-----------------------------------------------------------------------------------------


	/*********************************************************************************************

		Constructs the base class.

	*********************************************************************************************/
	CControlManager::CControlManager(const CRect & _size, int nTags) 
		: parent(nullptr), gridManager(_size), tagCounter(nTags) 
	{ 

	};		
	/*********************************************************************************************

	 	Gets a control from a tag.

	 *********************************************************************************************/
	CBaseControl * CControlManager::getControl(int nTag) 
	{
		
		for(auto & ctrl : _controls)
			if(ctrl->bGetTag() == nTag)
				return ctrl;
		
		for(auto & ctrl : pendingControls)
			if(ctrl->bGetTag() == nTag)
				return ctrl;

		return nullptr;
	}
	/*********************************************************************************************

	 	Gets an unused tag

	 *********************************************************************************************/
	int CControlManager::getNextTag()
	{
		int _tag;
		if(!unusedTags.empty()) {
			_tag = unusedTags.front();
			unusedTags.pop();
		}
		else
		{
			_tag = tagCounter++;
		}
		return _tag;
	}
	/*********************************************************************************************

	 	Detaches from the parent frame.

	 *********************************************************************************************/
	void CControlManager::detach() 
	{
		if(parent) {
			removeControls();
			parent = nullptr;
		}
	}
	/*********************************************************************************************

	 	Detaches all the controls from the parent - they are not deleted though.	 	

	 *********************************************************************************************/
	void CControlManager::removeControls() 
	{
		if(parent) {
			for(auto it = _controls.begin(); it != _controls.end(); ++it) {
				gridManager.removeGrid((*it)->bGetSize());
				(*it)->removeFromParent(parent);
			}
		}
	}
	/*********************************************************************************************

	 	Detaches all the controls from the parent - they are not deleted though.	 	

	 *********************************************************************************************/
	void CControlManager::setParent(GraphicComponent * p) 
	{
		parent = p;
	}
	/*********************************************************************************************

	 	Attaches to a new frame.	 	

	 *********************************************************************************************/
	void CControlManager::attach(GraphicComponent * frame) {
		detach();
		parent = frame;
		if(parent) {
			for(auto it = _controls.begin(); it != _controls.end(); ++it) {

				(*it)->addToParent(parent);
			}
		}
	}
	/*********************************************************************************************

	 	Adds a CBaseControl that's already been positioned to the frame.	 	

	 *********************************************************************************************/
	void CControlManager::addControl(CBaseControl* _ctrl)
	{
		//_ctrl->remember(); should fix memory issues.
		// since we create all controls, nbreference is at 1 anyway (we have last ownership)
		//_controls.push_back(_ctrl);

		pendingControls.push_back(_ctrl);

	}
	void CControlManager::createPendingControls()
	{
		if(parent && pendingControls.size())
		{
			// remember to lock message thread
			const juce::MessageManagerLock mmLock;
			
			for(auto & ctrl : pendingControls)
			{
				ctrl->addToParent(parent);
				_controls.push_back(ctrl);
			}
			pendingControls.clear();
		}
	}
	/*********************************************************************************************

	 	Removes a control from the frame, unregisters it and erases.	 	

	 *********************************************************************************************/
	void CControlManager::deleteControl(CBaseControl * _ctrl) 
	{
		for(auto it = _controls.begin(); it != _controls.end(); ++it) {
			if(*it == _ctrl) {
				CBaseControl * _ctrl = (*it);
				gridManager.removeGrid((*it)->bGetSize());
				_controls.erase(it);
				if(parent)
					_ctrl->removeFromParent(parent);
				_ctrl->forget();
				// push tag into stack of old tags
				unusedTags.push((*it)->bGetTag());
				break;
			}
		}
	}
	/*********************************************************************************************

	 	Updates the controls so they redraw.	 	

	 *********************************************************************************************/
	void CControlManager::updateControls() 
	{
		for ( auto it = _controls.begin(); it != _controls.end(); ++it) {
			//if((*it)->isAttached()) {
				(*it)->setDirty();
			//}
		}
	}
	/*********************************************************************************************

	Updates the controls so they redraw.

	*********************************************************************************************/
	void CControlManager::callListeners()
	{
		for (auto it = _controls.begin(); it != _controls.end(); ++it) {
			//if((*it)->isAttached()) {
#pragma message cwarn("enabled isattached method again")
			(*it)->bForceEvent();

			//}
		}
	}
	/*********************************************************************************************

	 	Removes, unregisters and deletes them.	

	 *********************************************************************************************/
	void CControlManager::deleteControls() 
	{
		auto it = _controls.begin();
		auto end = _controls.end();
		while (it != end) {
			auto ctrl = *it;
			if(parent)
				ctrl->removeFromParent(parent);
			it = _controls.erase(it);
			delete ctrl;
		}
		gridManager.clear();
	}
	/*********************************************************************************************

		Resets the state.

	 *********************************************************************************************/
	void CControlManager::reset() 
	{
		removeControls();
		for(auto it = _controls.begin(); it != _controls.end(); ++it) {
			// push tag into stack of old tags
			unusedTags.push((*it)->bGetTag());
			delete (*it);
		}
		_controls.erase(_controls.begin(), _controls.end());
		gridManager.clear();
	}
	/*********************************************************************************************

		removes controls and deletes them, and clears the control list and resets the gridmanager.

	 *********************************************************************************************/
	CControlManager::~CControlManager() 
	{
		reset();

	}
	/*********************************************************************************************

		The CPluginCtrlManager provides an interface that glues the CControlManager and VstGuiEX buttons.

	 *********************************************************************************************/
	CPluginCtrlManager::CPluginCtrlManager(CCtrlListener * list, const CRect & _size, int nControlTagStart) 
		: list(list), CControlManager(_size, nControlTagStart) 
	{
	}
	/*********************************************************************************************

		Adds an automatable knob.

	 *********************************************************************************************/
	int CPluginCtrlManager::addKnob(const char * name, float * val, CKnobEx::type _type) 
	{
		if(!gridManager.full()) {
			int _tag = getNextTag();
			CRect pos = gridManager.getFirstEmpty();
			CBaseControl * ctrl = new CKnobEx(pos, name, _type, val);
			ctrl->bSetListener(list);
			ctrl->bSetTag(_tag);
			if(val)
				ctrl->bSetValue(*val);
			addControl(ctrl);
			return _tag;
		}
		// return -1 if no space is available.
		return -1;
	}
	/*********************************************************************************************

		Adds an automatable knob with a display that spans the values provided, evenly distributed.
		Values should be a list seperated with '|' markers, and unit is an optional postfixed unit.

	 *********************************************************************************************/
	int CPluginCtrlManager::addKnob(const char * name, float * val, char * values, char * unit) 
	{
		if(!gridManager.full()) {
			int _tag = getNextTag();
			CRect pos = gridManager.getFirstEmpty();
			CBaseControl * ctrl = new CValueKnobEx(pos, name, val, values, unit);
			ctrl->bSetListener(list);
			ctrl->bSetTag(_tag);
			if(val)
				ctrl->bSetValue(*val);
			addControl(ctrl);
			return _tag;
		}
		// return -1 if no space is available.
		return -1;
	}
	/*********************************************************************************************

		Adds a label display. fmt should be a printf format string, where args is a pointer to the
		arguments, which are passed with indirection (pointers to the arguments).

	 *********************************************************************************************/
	int CPluginCtrlManager::addLabel(const char * name, const char * fmt, va_list args) 
	{
		if(!gridManager.full()) {
			int _tag = getNextTag();
			CRect pos = gridManager.getFirstEmpty();
			CLabelDisplay * ctrl = new CLabelDisplay(pos);
			ctrl->bSetTag(_tag);
			ctrl->bSetTitle(name);
			ctrl->setFormat(fmt, args);
			addControl(ctrl);
			return _tag;
		}
		// return -1 if no space is available.
		return -1;
	}
	/*********************************************************************************************

		Adds a meter display. 

	 *********************************************************************************************/
	int CPluginCtrlManager::addMeter(const char * name, float * extVal) 
	{
		if(!gridManager.full()) {
			int _tag = getNextTag();
			CRect pos = gridManager.getFirstEmpty();
			CMeterDisplay * ctrl = new CMeterDisplay(pos,extVal, name);
			ctrl->bSetTag(_tag);
			addControl(ctrl);
			return _tag;
		}
		// return -1 if no space is available.
		return -1;
	}
	/*********************************************************************************************

		Adds a user button.

	 *********************************************************************************************/
	int CPluginCtrlManager::addToggle(const char * name, float * extVal)
	{
		/*
			here we iterate over all groups of toggles to find the first non-full group of toggles
			that we can insert another one into. If neither exists, we insert it
		*/
		if (gridManager.full())
			return -1;
		auto it = togglegroups.begin();
		CToggleGroup<> * grp(nullptr);
		// iterate over togglegroups, goal is to get grp to point to a valid element
		// remember .begin() returns .end() if container is empty.
		while(it != togglegroups.end())
		{
			if(!it->isFull()) {
				break;
			}
			it++;
		}
		// did we find one?
		if(it == togglegroups.end()) 
		{
			// we did not find a non-full group: make a new
			togglegroups.push_back(CToggleGroup<>());
			grp = &togglegroups.back();
		}
		else
			// this feels stupid
			grp = &(*it);

		// grp should now be valid and point to the first group in the vector that doesn't claim to be full.

		auto elements = grp->getSize();

		if(!elements)
		{
			if(!gridManager.full()) {
				// group has not yet been assigned a position if there's no elements (ie. new)
				grp->pos = gridManager.getFirstEmpty();
			}
			else
			{
				// no space available
				return -1;
			}
		}
		// calculate height
		int height = this->gridManager._grid_side / grp->getMax();
		CRect pos(grp->pos);
		// offset toggle
		pos.setY(grp->pos.getY() + height * elements);
		pos.setBottom(pos.getY() + height);
		// get tag
		int _tag = getNextTag();
		// create toggle
		CToggle * ctrl = new CToggleCtrl(pos, extVal, name);
		ctrl->bSetListener(list);
		ctrl->bSetTag(_tag);
		grp->add(ctrl);
		addControl(ctrl);
		return _tag;
	}
	/*********************************************************************************************

		Resets the state.

	 *********************************************************************************************/
	void CPluginCtrlManager::reset() 
	{
		togglegroups.clear();
		CControlManager::reset();
	}
	/*********************************************************************************************

		Adds a plot. 

	 *********************************************************************************************/
	int CPluginCtrlManager::addPlot(const char * name, const float * const vals, unsigned int numVals) 
	{
		if(!gridManager.full()) {
			int _tag = getNextTag();
			CRect pos = gridManager.getFirstEmpty();
			CPlot * ctrl = new CPlot(pos, name, vals, numVals);
			ctrl->bSetTag(_tag);
			addControl(ctrl);
			return _tag;
		}
		// return -1 if no space is available.
		return -1;
	}
	/*********************************************************************************************

		Adds a ranged knob

	 *********************************************************************************************/
	int CPluginCtrlManager::addKnob(const char * name, const char * unit, float * extVal, ape::ScaleFunc cb, float min, float max) 
	{
		if(!gridManager.full()) {
			int _tag = getNextTag();
			CRect pos = gridManager.getFirstEmpty();
			CRangeKnob * ctrl = new CRangeKnob(pos,  name, unit, extVal, cb, min, max);
			ctrl->bSetListener(list);
			ctrl->bSetTag(_tag);
			if (extVal)
				ctrl->bSetValue(*extVal);
			addControl(ctrl);
			return _tag;
		}
		// return -1 if no space is available.
		return -1;
	}
};