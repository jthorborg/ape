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

	file:VstGuiEx.cpp
		
		Implementation of VstGuiEx.h
		Most of this code is really horrible, and have to be rewritten.

*************************************************************************************/


#include "Common.h"
#include "vstguiex.h"

namespace VSTGUI {
	/*********************************************************************************************

		CLabelDisplay

	 *********************************************************************************************/
	CLabelDisplay::CLabelDisplay(const CRect & _where, long tag) 
		:  CBaseControl(_where, tag)
	{
		labels[0] = new CTextLabel( CRect(_where.left + 5, _where.top  + 5, _where.right - 5, _where.top + 22),
									0, 0, kShadowText);
		labels[1] = new CTextLabel( CRect(_where.left + 5, _where.top + 22, _where.right - 5, _where.bottom),
									0, 0, kShadowText);
		labels[1]->setHoriAlign(VSTGUI::CHoriTxtAlign::kLeftText);
		labels[0]->setFont(kNormalFontSmall);
		labels[1]->setFont(VSTGUI::kNormalFontSmaller);
		for(auto label : labels) {
			label->setTransparency(true);
			label->setShadowColor(kBlueCColor);
		}
	}
	/*********************************************************************************************/
	void CLabelDisplay::setTitle(const char * name)
	{
		title = name;
		labels[0]->setText(name);
		labels[0]->setDirty();
	}
	/*********************************************************************************************/
	void CLabelDisplay::setFormat(const char * fmt, va_list args)
	{
		text.setFormat(fmt, args);
	}
	/*********************************************************************************************/
	void CLabelDisplay::addToParent(CFrame * frame)
	{
		frame->addView(labels[0]);
		frame->addView(labels[1]);
	}
	/*********************************************************************************************/
	void CLabelDisplay::removeFromParent(CFrame * frame)
	{
		frame->removeView(labels[0], false);
		frame->removeView(labels[1], false);
	}
	/*********************************************************************************************/
	void CLabelDisplay::update()
	{
		labels[1]->setText(text.get().c_str());
		labels[1]->setDirty();
	}
	/*********************************************************************************************

		CKnobEx

	 *********************************************************************************************/
	CKnobEx::CKnobEx(CControlListener * list, const CRect & _where, long tag, CBitmap * _indicator, CBitmap * _circle, const char * name, type _type, float * extVal) 
		:  _type(_type), pVal(extVal), _indicator(_indicator), _circle(_circle), CBaseControl(_where, tag)
	{
		_indicator->remember();
		_circle->remember();
		// create the name of the knob
		//								5 away from corner, and is 17 in height (bottom = 17+5).
		labels[0] = new CTextLabel(CRect(size.left + 5, size.top + 5, size.right - 5, size.top + 22), 
									name, NULL, kShadowText);
		views.push_back(labels[0]);
		// everything here is changed from left = x and top = y
		//temp(_where.x + 5,_where.y + _circle->getHeight() + 17 +5, _where.x +_circle->getWidth() * 2 - 5, _where.y +_circle->getHeight() + 17 * 2 + 5);
		labels[1] = new CTextLabel(CRect(size.left + 5, size.bottom - 18, size.right - 5, size.bottom - 5),
									"", NULL, kShadowText);
		views.push_back(labels[1]);
		labels[0]->setFont(kNormalFontSmall);
		labels[1]->setFont(VSTGUI::kNormalFontSmaller);

		_knob = new CKnob(CRect(size.left + int(_circle->getWidth() * 0.5), size.top + 22, size.top + int(_circle->getWidth() * 1.5) , size.top +_circle->getHeight() + 22),
							list, tag, _circle, _indicator, CPoint(0,0));
		views.push_back(_knob);
		for each (auto label in labels) {
			label->setTransparency(true);
			label->setShadowColor(kBlueCColor);
		}
		// null + valid range check
		if(extVal && *extVal >= 0.f && *extVal <= 1.f)
			_knob->setValue(*extVal);
		valueChanged(_knob);
	}
	/*********************************************************************************************/
	void CKnobEx::addToParent(CFrame * frame)
	{

		for(auto it = views.begin(); it != views.end(); ++it) {
			frame->addView((*it));
		}
	}
	/*********************************************************************************************/
	void CKnobEx::removeFromParent(CFrame * frame)
	{
		for(auto it = views.begin(); it != views.end(); ++it) {
			// NO idea why this is required, plugin crashes after 3 reopens?
			// seems like the frame doesn't add as many references as it deletes.
			frame->removeView((*it), false);
		}
	}
	/*********************************************************************************************/
	CView * CKnobEx::getView()
	{
		return _knob;
	}
	/*********************************************************************************************/
	float CKnobEx::getValue()
	{
		return _knob->getValue();
	}
	/*********************************************************************************************/
	void CKnobEx::setValue(float val)
	{
		return _knob->setValue(val);
	}
	/*********************************************************************************************/
	void CKnobEx::setText(const char * text)
	{
		labels[1]->setText(text);
	}
	/*********************************************************************************************/
	void CKnobEx::getText(char * text)
	{
		::strcpy(text, labels[1]->getText());
	}
	/*********************************************************************************************/
	void CKnobEx::valueChanged(CControl* control) 
	{
		if(control->getTag() != tag)
			return;
		float val = control->getValue();
		char buf[100];
		switch(_type) {
		case pct:
			sprintf_s(buf, "%d %%", int(val * 100));
			labels[1]->setText(buf);
			break;
		case hz:
			sprintf_s(buf, "%.1f Hz", (float)(val * hzLimit));
			labels[1]->setText(buf);
			break;
		case db:
			if(val == 0.f)
				sprintf_s(buf,"-oo dB");
			else
				sprintf_s(buf, "%.3f dB", (float)(20. * log10 (val)));
			labels[1]->setText(buf);
			break;
		case ft:
			sprintf_s(buf, "%.3f", val);
			labels[1]->setText(buf);
			break;
		case ms:
			sprintf_s(buf, "%d ms", int(val * msLimit));
			labels[1]->setText(buf);
			break;
		};
		if(pVal)
			*pVal = val;

	}
	/*********************************************************************************************/
	CKnobEx::~CKnobEx()
	{
		_circle->forget();
		_indicator->forget();
		_knob->forget();
		for(auto label : labels) 
			label->forget();
	}
	void CKnobEx::update()
	{
		labels[1]->setDirty();
	}
	/*********************************************************************************************

		CValueKnobEx

	 *********************************************************************************************/
	CValueKnobEx::CValueKnobEx(CControlListener * list, const CRect & _where, long tag, CBitmap * _indicator, CBitmap * _circle, 
		const char * name, float * extVal, char * values, char * unit)
		: CKnobEx(list, _where, tag, _indicator, _circle, name, pct, extVal) 
	{
		if(unit && unit[0])
			_unit = unit;
		char * iter_pos;
		int len = strlen(values);
		int i;
		for(i = 0, iter_pos = values; i < len; i++) {
			if(values[i] == '|') {
				_values.push_back(std::string(iter_pos, values + i));
				iter_pos = values + i + 1;
			} else if(i == (len - 1)) {
				_values.push_back(std::string(iter_pos, values + i + 1));

			}

		}

		CValueKnobEx::valueChanged(_knob);
		CKnobEx::update();
	}
	/*********************************************************************************************/
	void CValueKnobEx::valueChanged(CControl * control)
	{
			if(control->getTag() != tag)
				return;

			float val = control->getValue();

			int idx = APE::Misc::Round(val * (_values.size() ? _values.size() - 1 : 0));
			std::string t;
			t = (_values[idx] + " ")  + _unit;
			labels[1]->setText(t.c_str());
			if(pVal)
				*pVal = val;
			

	}
	/*********************************************************************************************

		CCheckBox - a check box class. Not a CBaseControl yet!

	 *********************************************************************************************/
	CCheckBoxButton::CCheckBoxButton(const CRect & size, const char * _name, CControlListener * l, long nTag, CBitmap * chk)
		: COnOffButton(CRect(size.left, size.top, size.left + chk->getWidth(), size.top + chk->getHeight() / 2), l, nTag, chk),
		_size(size), sizeofline(14), name(NULL)
	{
		int yoff = (size.height() - sizeofline) / 2;
		int top = size.top + yoff;
		int left = size.left + chk->getWidth() + 5;
		int right = size.right - 5;
		int bot = size.bottom - yoff;

		name = new CTextLabel(CRect(left, top, right, bot), _name, NULL, kShadowText);
		name->setHoriAlign(VSTGUI::CHoriTxtAlign::kLeftText);
		name->setTransparency(true);
		name->setShadowColor(kBlueCColor);
	}
	/*********************************************************************************************/
	CRect CCheckBoxButton::getSize()
	{
		return _size;
	}
	/*********************************************************************************************/
	CView * CCheckBoxButton::getText()
	{
		return name;
	}
	/*********************************************************************************************

		CMeterDisplay

	 *********************************************************************************************/
	CMeterDisplay::CMeterDisplay(const CRect & _where, long tag, float * extVal, const char * name)
		: CBaseControl(_where, tag), val(extVal), smoothVal(0.0)
	{

		meter = new CImage(GUI_RESOURCE::kMeter, CRect(size.right - 15, size.top + 10, size.right - 5, size.bottom - 10));
		text = new CTextLabel(CRect(size.left, size.top + 5, size.right - 10, size.top + 20), name, nullptr, kShadowText);
		text->setHoriAlign(VSTGUI::CHoriTxtAlign::kLeftText);
		text->setFont(kNormalFontSmall);
		text->setTransparency(true);
		text->setShadowColor(kBlueCColor);
		// the pole constant. The higher this number is, the slower the meter moves. Calculatable in ms through an exp() expression.
		pole = 0.8;
	}
	/*********************************************************************************************/
	void CMeterDisplay::addToParent(CFrame * frame)
	{
		frame->addView(meter);
		frame->addView(text);
	}
	/*********************************************************************************************/
	void CMeterDisplay::removeFromParent(CFrame * frame)
	{
		frame->removeView(meter, false);
		frame->removeView(text, false);
	}
	/*********************************************************************************************/
	CMeterDisplay::~CMeterDisplay()
	{
		meter->forget();
		text->forget();
	}
	/*********************************************************************************************/
	void CMeterDisplay::update()
	{
		double value = fabs(*val);
		if(value > 1.0)
			value = 1.0;
		// using a 1 pole filter to smoothen out the movement of the meter and remove jerkiness
		smoothVal = value + pole * (value - smoothVal);

		meter->setOffset(static_cast<long>(meter->getHeight() - meter->getHeight() * smoothVal)); 
		meter->setDirty();
	}
	/*********************************************************************************************

		CImage

	 *********************************************************************************************/
	CImage::CImage(long resourceID, const CRect & size)
		: CView(size), offset(0)
	{
		image = new CBitmap(resourceID);
	}
	/*********************************************************************************************/
	void CImage::setOffset(long off)
	{
		offset = off;	
	}
	/*********************************************************************************************/
	void CImage::draw(CDrawContext *pContext)
	{
		image->draw(pContext, size,CPoint(0, -offset));
		setDirty(false);
	}
	/*********************************************************************************************/
	CImage::~CImage()
	{
		image->forget();
	}
	/*********************************************************************************************

		CButton

	 *********************************************************************************************/
	CToggle::CToggle(const CRect & _where, CControlListener * lst, long tag, CBitmap * buttonImage, float * extVal, const char * name)
		: CBaseControl(_where, tag), extVal(extVal)
	{
		_button = new CCheckBoxButton(_where, name, lst, tag, buttonImage);
		/*_text = new CTextLabel(CRect(size.x + 5, size.y + 5, size.right - 5, size.y + 22), 
									name, nullptr, kShadowText);
		_text->setFont(kNormalFontSmall);
		_text->setTransparency(true);
		_text->setShadowColor(kBlueCColor);*/
	}
	/*********************************************************************************************/
	void CToggle::addToParent(CFrame * frame)
	{
		_button->addToParent(frame);
	}
	/*********************************************************************************************/
	void CToggle::removeFromParent(CFrame * frame)
	{
		_button->removeFromParent(frame);
	}
	/*********************************************************************************************/
	CToggle::~CToggle()
	{
		_button->forget();
	}
	/*********************************************************************************************/
	void CToggle::valueChanged(CControl * control) 
	{
		if(extVal)
			*extVal = control->getValue();
	}
	/*********************************************************************************************/
	void CToggle::update()
	{

	}
	/*********************************************************************************************/
	CPlot::~CPlot() {
		if (plot)
			delete plot;
		if (title)
			title->forget();
		if (points)
			delete[] points;
	}
	/*********************************************************************************************/
	void CPlot::update() 
	{
		plot->setDirty();
	}
	/*********************************************************************************************/
	void CPlot::removeFromParent(CFrame * frame)
	{
		frame->removeView(plot, false);
		frame->removeView(title, false);
	}
	/*********************************************************************************************/
	void CPlot::addToParent(CFrame * frame) 
	{
		frame->addView(plot);
		frame->addView(title);
	}
	/*********************************************************************************************/
	CPlot::CPlot(const CRect & _where, long tag, const char * name,
		const float * const values, const int numValues)
		: numPoints(numValues), CBaseControl(_where, tag), values(values)
	{
		points = new CPoint[numPoints];

		for (unsigned i = 0; i < numPoints; ++i)
		{
			points[i].x = static_cast<CCoord>(i + _where.left);
		}
		CRect plotSize(_where);
		plotSize.top += 22;
		plotSize.left += 2;
		plotSize.right -= 2;
		plotSize.bottom -= 2;
		plot = new CPlotView(this, plotSize);
		title = new CTextLabel(CRect(_where.left + 5, _where.top + 5, _where.right - 5, _where.top + 22),
			0, 0, kShadowText);
		title->setText(name);
		title->setTransparency(true);
		title->setShadowColor(kBlueCColor);
		title->setDirty();
	}
	/*********************************************************************************************/
	void CPlot::CPlotView::draw(VSTGUI::CDrawContext * ctx)
	{
		if (base->values) {
			int height = size.getHeight();
			ctx->setLineStyle(VSTGUI::kLineSolid);
			ctx->setLineWidth(1);
			ctx->setFrameColor(kGreenCColor);
			for (unsigned int i = 0; i < base->numPoints; ++i)
			{
				base->points[i].y = static_cast<CCoord>(-base->values[i] * height / 2 + size.top + height / 2);
			}

			ctx->drawLines(base->points, base->numPoints);
		}
	}
	/*********************************************************************************************/
	CRangeKnob::CRangeKnob(CControlListener * list, const CRect & _where, long tag, CBitmap * _indicator, CBitmap * _circle, const char * name,
		const char * _unit, float * extVal, APE::ScaleFunc scaleCB, float _min, float _max)
		: CKnobEx(list, _where, tag, _indicator, _circle, name, type::ft, extVal), extScale(scaleCB), _min(_min), _max(_max)
	{


		if (_unit && *_unit)
			strcpy_s(unit, _unit);
		else
			strcpy_s(unit, "");

		// format knob immediatly - this cannot be done in base class as valuechanged is virtual and overloaded
		this->valueChanged(_knob);
		CKnobEx::update();

	}
	/*********************************************************************************************/
	void CRangeKnob::valueChanged(CControl* control)
	{
		if (control->getTag() != tag)
			return;
		float val = control->getValue();
		char buf[100];
		float finalValue;
		if (extScale)
			finalValue = extScale(val, _min, _max);
		else
			finalValue = val;


		sprintf_s(buf, "%.3f %s", finalValue, unit);
		labels[1]->setText(buf);
		if (pVal)
			*pVal = val;

	}
};