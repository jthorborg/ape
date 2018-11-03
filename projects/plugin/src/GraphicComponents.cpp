

#include "GraphicComponents.h"
#include <cpl/Misc.h>


namespace ape
{
	/*********************************************************************************************

		CTextLabel

	*********************************************************************************************/
	CTextLabel::CTextLabel()
		: Component(), just(juce::Justification::centredLeft)
	{
		setSize(200, 20);
	}

	void CTextLabel::setFontSize(float newSize)
	{
		this->size = newSize;
		repaint();
	}

	void CTextLabel::setFontName(const juce::String& name)
	{
		fontName = name;
	}

	void CTextLabel::setColour(CColour newColour)
	{
		this->colour = newColour;
		repaint();
	}

	void CTextLabel::setText(const std::string & newText)
	{
		text = newText;
		repaint();
	}

	void CTextLabel::paint(juce::Graphics & g)
	{
		juce::Font f(fontName.length() ? fontName : juce::Font::getDefaultSansSerifFontName(), size, juce::Font::plain);
		g.setFont(f);
		g.setColour(colour);
		g.drawText(text, CRect(0, 0, getWidth(), getHeight()), just, false);
	}

	void CTextLabel::setPos(int x, int y)
	{
		setCentrePosition(x + getWidth() / 2, y + getHeight() / 2);
	}
	/*********************************************************************************************

		CScrollableContainer

	*********************************************************************************************/
	CScrollableContainer::CScrollableContainer()
		: Component("CScrollableLineContainer")
		, scb(true)
	{
		addAndMakeVisible(virtualContainer);
		scb.addListener(this);
		scb.setColour(juce::ScrollBar::ColourIds::trackColourId, juce::Colours::lightsteelblue);
		addAndMakeVisible(scb);
	}

	void CScrollableContainer::resized()
	{
		virtualContainer.setBounds(0, 0, getWidth() - scb.getWidth(), 1300);
	}

	void CScrollableContainer::paint(juce::Graphics & g)
	{
	}

	int CScrollableContainer::getVirtualHeight()
	{
		return virtualContainer.getHeight();
	}

	void CScrollableContainer::setVirtualHeight(int height)
	{
		virtualContainer.setSize(virtualContainer.getWidth(), height);
	}

	float CScrollableContainer::bGetValue()
	{
		double start = scb.getCurrentRangeStart();
		auto delta = 1.0 / (1 - scb.getCurrentRangeSize());
		return static_cast<float>(start * delta);
	}

	void CScrollableContainer::bSetValue(float newVal)
	{
		double delta = 1.0 / (1 - scb.getCurrentRangeSize());
		scb.setCurrentRangeStart(newVal / delta);
	}

	void CScrollableContainer::scrollBarMoved(juce::ScrollBar * b, double newRange)
	{
		virtualContainer.setBounds(
			0,
			static_cast<signed int>(-bGetValue() * (virtualContainer.getHeight() - getHeight())),
			virtualContainer.getWidth(),
			virtualContainer.getHeight());
	}

	void CTextControl::bSetText(const std::string & newText)
	{
		std::lock_guard<std::mutex> lockGuard(*this);
		CTextLabel::setText(newText);
	}
	const std::string CTextControl::bGetText()
	{
		std::lock_guard<std::mutex> lockGuard(*this);
		return text.toStdString();
	}
	void CTextControl::paint(juce::Graphics & g)
	{
		std::lock_guard<std::mutex> lockGuard(*this);
		CTextLabel::paint(g);
	}
};