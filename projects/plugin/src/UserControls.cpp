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

	file:UserControls.cpp
		
		Implementation of UserControls.h
		Most of this code is really horrible, and have to be rewritten.

*************************************************************************************/


#include "Common.h"
#include "UserControls.h"
#include "Engine.h"
#include <cpl/CMutex.h>
#include <cpl/Mathext.h>

namespace ape {
	
	
	#define __CPLOT_ADD_BORDERS
	
	/*********************************************************************************************

		CLabelDisplay

	 *********************************************************************************************/
	CLabelDisplay::CLabelDisplay(const CRect & _where) 
		: CBaseControl(this)
	{
		// set bounds of our display
		setBounds(_where);
		// create texts
		labels[0] = new CTextLabel();
		labels[1] = new CTextLabel();
		// set their bounds
		labels[0]->setBounds(0, 0, _where.getWidth(), getHeight()/4);
		labels[1]->setBounds(5, labels[0]->getHeight(), _where.getWidth()-5, getHeight());
		// set justification (normal is centred | left)
		labels[0]->setJustification(juce::Justification::centred);
		// set font sizes
		labels[1]->setFontSize(TextSize::smallerText);
		labels[0]->setFontSize(TextSize::smallerText);
		// make them visible
		for (auto label : labels)
		{
			label->setColour(juce::Colours::lightgoldenrodyellow);
			addAndMakeVisible(label);
		}
		// make ourselves visible
		setVisible(true);

	}
	/*********************************************************************************************/
	void CLabelDisplay::bSetTitle(const std::string  & in)
	{
		cpl::CMutex lockGuard(this);
		title = in;
	}
	/*********************************************************************************************/
	void CLabelDisplay::setFormat(const char * fmt, va_list args)
	{
		cpl::CMutex lockGuard(this);
	}
	/*********************************************************************************************/
	void CLabelDisplay::bRedraw()
	{
		cpl::CMutex lockGuard(this);

	}
	CLabelDisplay::~CLabelDisplay()
	{
		delete labels[0];
		delete labels[1];
	}
	/*********************************************************************************************

		CKnobEx

	 *********************************************************************************************/
	CKnobEx::CKnobEx(const CRect & _where, const char * name, type _type, float * extVal) 
		:  _type(_type), pVal(extVal)
	{
		setBounds(_where);
		// null + valid range check
		if(extVal && *extVal >= 0.f && *extVal <= 1.f)
			bSetValue(*extVal);
		bSetTitle(name);
		//bForceEvent();
		setVisible(true);
	}
	/*********************************************************************************************/
	void CKnobEx::onValueChange()
	{
		float val = bGetValue();
		char buf[100];
		switch(_type) {
		case pct:
			sprintf_s(buf, "%d %%", int(val * 100));
			bSetText(buf);
			break;
		case hz:
			sprintf_s(buf, "%.1f Hz", (float)(val * hzLimit));
			bSetText(buf);
			break;
		case db:
			if(val == 0.f)
				sprintf_s(buf,"-oo dB");
			else
				sprintf_s(buf, "%.3f dB", (float)(20. * log10 (val)));
			bSetText(buf);
			break;
		case ft:
			sprintf_s(buf, "%.3f", val);
			bSetText(buf);
			break;
		case ms:
			sprintf_s(buf, "%d ms", int(val * msLimit));
			bSetText(buf);
			break;
		};
		if(pVal)
			*pVal = val;

	}
	/*********************************************************************************************

		CValueKnobEx

	 *********************************************************************************************/
	CValueKnobEx::CValueKnobEx(const CRect & _where, const char * name, float * extVal, char * values, char * unit)
		: CKnobEx(_where, name, pct, extVal) 
	{
		if(unit && unit[0])
			_unit = unit;
		char * iter_pos;
		auto len = strlen(values);
		int i;
		for(i = 0, iter_pos = values; i < len; i++) {
			if(values[i] == '|') {
				_values.push_back(std::string(iter_pos, values + i));
				iter_pos = values + i + 1;
			} else if(i == (len - 1)) {
				_values.push_back(std::string(iter_pos, values + i + 1));

			}

		}

		//bForceEvent();
		//CKnobEx::bRedraw();
	}
	/*********************************************************************************************/
	void CValueKnobEx::onValueChange()
	{
		float val = bGetValue();

		int idx = cpl::Math::round<int>(val * (_values.size() ? _values.size() - 1 : 0));
		std::string t;
		t = (_values[idx] + " ")  + _unit;
		bSetText(t.c_str());
		if(pVal)
			*pVal = val;
	}
	/*********************************************************************************************

		CMeterDisplay

	 *********************************************************************************************/
	CMeterDisplay::CMeterDisplay(const CRect & _where, float * extVal, const char * name)
		: CBaseControl(this), val(extVal), smoothVal(0.0), meter(CResourceManager::getImage("meter"))
	{
		setBounds(_where);
		title = new CTextLabel();
		title->setBounds(0, 0, _where.getWidth(), getHeight() / 4);
		title->setText(name);
		// set justification (normal is centred | left)
		title->setJustification(juce::Justification::centred);
		// set font sizes
		title->setFontSize(TextSize::smallerText);
		title->setColour(juce::Colours::lightgoldenrodyellow);
		addAndMakeVisible(title);
		setVisible(true);
		// the pole constant. The higher this number is, the slower the meter moves. Calculatable in ms through an exp() expression.
		pole = 0.8;
	}

	/*********************************************************************************************/
	CMeterDisplay::~CMeterDisplay()
	{
		delete title;
	}
	/*********************************************************************************************/
	void CMeterDisplay::bRedraw()
	{
		double value = fabs(*val);
		if(value > 1.0)
			value = 1.0;
		// using a 1 pole filter to smoothen out the movement of the meter and remove jerkiness
		smoothVal = value /*+ pole * (value - smoothVal)*/;
		repaint();
	}
	void CMeterDisplay::paint(juce::Graphics & g)
	{
		int left;
		left = getWidth() - meter.getWidth() / 2;
		g.setColour(CColours::green);
		CRect r(30, static_cast<int>( 20 + (1 - smoothVal) * 60 ), 20, 80);
		g.fillRect(r);
		// -- fix this some day
		//g.drawImage(meter, left, getHeight() - meter.getHeight() * smoothVal, meter.getWidth(), meter.getHeight() * smoothVal, 0, 0, meter.getWidth(), meter.getHeight());

	}

	/*********************************************************************************************

		CButton

	 *********************************************************************************************/
	CToggleCtrl::CToggleCtrl(const CRect & _where, float * extVal, const char * name)
		:  extVal(extVal)
	{
		setBounds(_where);

		setVisible(true);
		bSetText(name);
	}

	/*********************************************************************************************/
	void CToggleCtrl::onValueChange() 
	{
		if(extVal)
			*extVal = bGetValue();
	}

	/*********************************************************************************************/
	CPlot::~CPlot() {
		if (title)
			delete title;
		if (points)
			delete[] points;
	}
	/*********************************************************************************************/
	CPlot::CPlot(const CRect & _where,const char * name,
		const float * const values, const int numValues)
		: numPoints(numValues), CBaseControl(this), values(values), pst(2)
	{
		setBounds(_where);
		points = new CPoint[numPoints];

		for (unsigned i = 0; i < numPoints; ++i)
		{
			points[i].x = static_cast<CCoord>(i);
		}
		plotSize = _where;
		#ifdef __CPLOT_ADD_BORDERS
			plotSize.setY(22);
			plotSize.setX(2);
			plotSize.setRight(getWidth()-2);
			plotSize.setBottom(getHeight() - 2);
		#else
			plotSize.setY(20);
			plotSize.setX(0);
			plotSize.setRight(getWidth());
			plotSize.setBottom(getHeight());
		#endif
		
		horzScale = (double)numPoints / (double)plotSize.getWidth();
		
		title = new CTextLabel();
		title->setBounds(CRect(0, 0, _where.getWidth(), getHeight()/4));
		title->setText(name);
		title->setColour(juce::Colours::lightgoldenrodyellow);
		title->setFontSize(TextSize::smallerText);
		title->setJustification(juce::Justification::centred);
		addAndMakeVisible(title);
		setVisible(true);
	}
	/*********************************************************************************************/
	void CPlot::paint(juce::Graphics & g)
	{
		g.setColour(juce::Colours::green);
		

		float halfHeight = plotSize.getHeight() / 2.f, offset = static_cast<float>(plotSize.getY());

		juce::Path p;
		p.startNewSubPath(0, -values[0] * halfHeight + offset + halfHeight);
		bool shouldScale(false); // we scale any values over [-1, 1] to fit, otherwise we wont.
		float tempVal;
		for(unsigned i = 1; i < numPoints; i++)
		{
			tempVal = -values[i] * halfHeight + offset + halfHeight;
			if (!std::isnormal(tempVal)) 
			{
				// 'error' on NaN values
				// this fucking shit just never works, does it
				g.setColour(CColours::red);
				g.setFont(TextSize::normalText);
				g.drawText("NaN values", CRect(0, 0, getWidth(), getHeight()), juce::Justification::centred, false);
				return;
			}
			p.lineTo(static_cast<float>(i), tempVal);
			if (!shouldScale && (std::fabs(values[i]) > 1))
				shouldScale = true;
		}
		//p.closeSubPath();
		if (shouldScale)
		{
			p.scaleToFit(static_cast<float>(plotSize.getX()), static_cast<float>(plotSize.getY()),
				static_cast<float>(plotSize.getWidth()), static_cast<float>(plotSize.getHeight()), false);
			g.strokePath(p, pst);
			g.setColour(CColours::red);
			g.drawText("+1", CRect(5, plotSize.getY(), 20, 20), juce::Justification::centred, false);
		}
		else
		{
			g.strokePath(p, pst);
		}

		/*
		 
		 unsigned end = plotSize.getRight() - 1;
		 unsigned bufferEnd = numPoints - 1;
		 // these are used for linear interpolation
		 float y1, y2, z, mu;
		 int x1, x2;
		 // number of points matches, no interpolation needed
		if(numPoints == plotSize.getWidth())
		{
			for (unsigned i = 0, x = plotSize.getX(); (x < end) || (i < bufferEnd); ++x, ++i)
			{
				g.drawLine(x, -values[i] * halfHeight + offset + halfHeight, x + 1, -values[i + 1] * halfHeight + offset + halfHeight, 2);

			}
		}
		// number of points doesn't match, we need to interpolate
		else
		{
			
			for (unsigned i = 0, x = plotSize.getX(); (x < end) || (i < bufferEnd); ++x, ++i)
			{
				// floor()
				z  = x *  horzScale;
				x1 = static_cast<int>(z);
				mu = z - x1;
				x2 = x1 + 1;
				if(x2 >= numPoints)
					x2 = x1;
				
				y1 = (values[x1] * (1-mu) + values[x2] * mu);
				
				z  = (x + 1) *  horzScale;
				x1 = static_cast<int>(z);
				mu = z - x1;
				x2 = x1 + 1;
				if(x1 >= numPoints)
					x1 = numPoints - 1;
				if(x2 >= numPoints)
					x2 = x1;
				
				y2 = (values[x1] * (1-mu) + values[x2] * mu);
				
				g.drawLine(x, -y1 * halfHeight + offset + halfHeight, x + 1, -y2 * halfHeight + offset + halfHeight, 2);
				
			}
			   
		}
		 */
	}
	void CPlot::bRedraw()
	{
		repaint();
	}
	/*********************************************************************************************/
	CRangeKnob::CRangeKnob(const CRect & _where, const char * name, const char * _unit, float * extVal, ape::ScaleFunc scaleCB, float _min, float _max)
		: CKnobEx(_where, name, type::ft, extVal), extScale(scaleCB), _min(_min), _max(_max)
	{


		if (_unit && *_unit)
			strcpy_s(unit, _unit);
		else
			strcpy_s(unit, "");

		// format knob immediatly - this cannot be done in base class as onValueChange is virtual and overloaded
		//bForceEvent();
		//CKnobEx::bRedraw();

	}
	/*********************************************************************************************/

	void CRangeKnob::onValueChange()
	{
		float val = bGetValue();
		char buf[100];
		float finalValue;
		if (extScale)
			finalValue = extScale(val, _min, _max);
		else
			finalValue = val;


		sprintf_s(buf, "%.3f %s", finalValue, unit);
		bSetText(buf);
		if (pVal)
			*pVal = val;

	}
};