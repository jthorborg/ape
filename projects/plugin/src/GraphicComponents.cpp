

#include "GraphicComponents.h"
#include <cpl/Misc.h>


namespace ape
{
	/*********************************************************************************************

		CImage

	*********************************************************************************************/
	CImage::CImage(const std::string & inPath)
		: path(inPath)
	{
	
	}

	CImage::CImage() 
	{
	}

	void CImage::setPath(const std::string & inPath)
	{
		path = inPath;

	}

	bool CImage::load()
	{
		juce::File f(path);

		internalImage = juce::ImageFileFormat::loadFrom(f);

		if (internalImage.isValid()) {
			drawableImage.setImage(internalImage);
			return true;
		}
		else
		{
			drawableImage.setImage(juce::Image::null);
		}
		return false;
	}

	juce::Image & CImage::getImage()
	{
		return internalImage;

	}

	juce::Drawable * CImage::getDrawable()
	{
		return &drawableImage;
	}

	CImage::~CImage()
	{
	}
	/*********************************************************************************************

		CResourceManager

	*********************************************************************************************/

	static const char * resourceNames[] = { "background" , "button_down", "button_up", "knob", "checkbox", "textbox"};

	juce::Drawable * CResourceManager::operator [] (const std::string & name)
	{
		loadResources();
		return resources[name].getDrawable();
	}

	bool CResourceManager::loadResources()
	{
		if (!isResourcesLoaded)
		{
			isResourcesLoaded = true;
			std::string dir = cpl::Misc::DirectoryPath() + "/resources/";
			for (auto name : resourceNames)
			{
				auto & image = resources[name];
				std::string path = (dir + name) + ".png";
				image.setPath(path);
				if (!image.load())
				{
					cpl::Misc::MsgBox("Error loading resource " + path, cpl::programInfo.programAbbr + " error!", cpl::Misc::MsgIcon::iStop);
					return false;
				}
			}
		}
		return true;
	}

	CResourceManager::CResourceManager() 
	{
	};

	juce::Drawable * CResourceManager::getResource(const std::string & name)
	{
		return instance()[name];
	}

	const juce::Image & CResourceManager::getImage(const std::string & name)
	{
		instance().loadResources();
		return instance().resources[name].getImage();

	}

	CResourceManager & CResourceManager::instance()
	{
		static CResourceManager ins;
		return ins;
	}
	/*********************************************************************************************

		CButton

	*********************************************************************************************/
	CButton::CButton()
		: DrawableButton("Button", ButtonStyle::ImageRaw), CBaseControl(this), multiToggle(false)
	{
		setSize(CResourceManager::getResource("button_up")->getWidth(),
			CResourceManager::getResource("button_up")->getHeight());
		setImages(CResourceManager::getResource("button_up"), nullptr, CResourceManager::getResource("button_down"), nullptr, nullptr, nullptr, CResourceManager::getResource("button_down"));
		setVisible(true);
		addListener(this);
	}

	CButton::CButton(const std::string & text, const std::string & textToggled, CCtrlListener * list)
		: DrawableButton("Button", ButtonStyle::ImageRaw), CBaseControl(this), multiToggle(false)
	{
		setSize(CResourceManager::getResource("button_up")->getWidth(),
			CResourceManager::getResource("button_up")->getHeight());
		setImages(CResourceManager::getResource("button_up"), nullptr, CResourceManager::getResource("button_down"), nullptr, nullptr, nullptr, CResourceManager::getResource("button_down"));
		setVisible(true);
		texts[0] = text;
		texts[1] = textToggled;
		addListener(this);
		bSetListener(list);
	}

	void CButton::setMultiToggle(bool toggle)
	{
		multiToggle = toggle;
		if (multiToggle)
			this->setClickingTogglesState(multiToggle);
	}

	void CButton::paintOverChildren(juce::Graphics & g)
	{
		g.setFont(TextSize::largeText);

		g.setColour(juce::Colours::lightgoldenrodyellow);
		if (multiToggle && !this->getToggleState())
			g.drawText(texts[1], CRect(getWidth(), getHeight()), juce::Justification::centred, false);
		else
			g.drawText(texts[0], CRect(getWidth(), getHeight()), juce::Justification::centred, false);
	}

	float CButton::bGetValue()
	{
		return getToggleState() ? 1.f : 0.f;
	}
	void CButton::bSetValue(float newValue)
	{
		getToggleStateValue().setValue(newValue > 0.1f ? true : false);
	}

	void CButton::bSetInternal(float newValue)
	{
		removeListener(this);
		setToggleState(newValue > 0.1f ? true : false, juce::NotificationType::dontSendNotification);
		addListener(this);
	}
	/*********************************************************************************************

		CKnob

	*********************************************************************************************/
	CKnob::CKnob()
		: Slider("Knob"), knobGraphics(CResourceManager::getImage("knob")), CBaseControl(this)
	{
		this->addListener(this);
		numFrames = knobGraphics.getHeight() / knobGraphics.getWidth();
		sideLength = knobGraphics.getWidth();
		setTextBoxStyle(NoTextBox, 0, 0, 0);
		setSize(APE_DEPRECATED_CONTROL_SIZE, APE_DEPRECATED_CONTROL_SIZE);
		this->setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);

	}

	void CKnob::paint(juce::Graphics& g)
	{

		if (knobGraphics.isValid()) {
			// quantize
			int value = static_cast<int>((getValue() - getMinimum()) / (getMaximum() - getMinimum()) * (numFrames - 1));
			// limit and cap
			//if (value * numFrames * sideLength > getHeight())
			//	value = getHeight() / ((numFrames - 1) * sideLength);
			//else if (0 > value )
			//	value = 0;
			//g.drawImage(knobGraphics, 0, 0, getWidth(), getHeight(), 0, value * sideLength, sideLength, sideLength);
			g.drawImage(knobGraphics, APE_DEPRECATED_CONTROL_SIZE / 4, APE_DEPRECATED_CONTROL_SIZE / 4, sideLength, sideLength, 0, value * sideLength, sideLength, sideLength);
		}
		g.setFont(TextSize::smallerText);
		g.setColour(juce::Colours::lightgoldenrodyellow);

		g.drawText(title, CRect(getWidth(), APE_DEPRECATED_CONTROL_SIZE / 4), juce::Justification::horizontallyCentred, false);
		g.drawText(text, CRect(0, APE_DEPRECATED_CONTROL_SIZE - (APE_DEPRECATED_CONTROL_SIZE / 4), getWidth(), APE_DEPRECATED_CONTROL_SIZE / 4), juce::Justification::centred, false);
	}

	float CKnob::bGetValue()
	{
		return static_cast<float>((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
	}
	void CKnob::bSetText(const std::string & in)
	{
		text = in;
	}
	void CKnob::bSetTitle(const std::string & in)
	{
		title = in;
	}
	void CKnob::bSetValue(float newValue)
	{
		setValue(newValue * (getMaximum() - getMinimum()) + getMinimum());
	}

	/*********************************************************************************************

		CToggle

	*********************************************************************************************/
	CToggle::CToggle()
		: CBaseControl(this), cbox(CResourceManager::getImage("checkbox"))
	{
		addListener(this);
		setSize(APE_DEPRECATED_CONTROL_SIZE, 20);
	}

	void CToggle::paint(juce::Graphics & g)
	{
		cpl::CMutex lockGuard(this);
		auto width = cbox.getWidth();
		bool toggled = getToggleState();
		g.drawImage(cbox, 0, 0, width, width, 0, toggled ? width : 0, width, width);
		g.setColour(juce::Colours::lightgoldenrodyellow);
		g.setFont(TextSize::normalText);
		g.drawText(text, CRect(width + 5, 0, getWidth() - width, width), juce::Justification::verticallyCentred | juce::Justification::left, true);
	}

	void CToggle::bSetText(const std::string & in)
	{
		cpl::CMutex lockGuard(this);
		text = in;
	}

	float CToggle::bGetValue()
	{
		auto retVal = this->getToggleStateValue().getValue();
		return retVal ? 1.0f : .0f;
	}

	void CToggle::bSetInternal(float newValue)
	{
		removeListener(this);
		getToggleStateValue().setValue(newValue > 0.1f ? true : false);
		addListener(this);
	}

	void CToggle::bSetValue(float newValue)
	{
		getToggleStateValue().setValue(newValue > 0.1f ? true : false);
	}
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
		: Component("CScrollableLineContainer"), CBaseControl(this)
	{
		virtualContainer = new Component();
		addAndMakeVisible(virtualContainer);
		scb = new juce::ScrollBar(true);
		scb->addListener(this);
		scb->setColour(juce::ScrollBar::ColourIds::trackColourId, juce::Colours::lightsteelblue);
		addAndMakeVisible(scb);
	}

	void CScrollableContainer::bSetSize(const CRect & in)
	{
		setSize(in.getWidth(), in.getHeight());
		scb->setBounds(in.getWidth() - 20, 0, 20, in.getHeight());
		virtualContainer->setBounds(0, 0, in.getWidth() - scb->getWidth(), 1300);
		CBaseControl::bSetPos(in.getX(), in.getY());
	}

	void CScrollableContainer::paint(juce::Graphics & g)
	{
		if (background)
			g.drawImage(*background, 0, 0, getWidth() - scb->getWidth(), getHeight(),
			0, 0, background->getWidth(), background->getHeight());
	}

	int CScrollableContainer::getVirtualHeight()
	{
		return virtualContainer->getHeight();
	}

	void CScrollableContainer::setVirtualHeight(int height)
	{
		virtualContainer->setSize(virtualContainer->getWidth(), height);
	}

	float CScrollableContainer::bGetValue()
	{
		double start = scb->getCurrentRangeStart();
		auto delta = 1.0 / (1 - scb->getCurrentRangeSize());
		return static_cast<float>(start * delta);
	}

	void CScrollableContainer::bSetValue(float newVal)
	{
		double delta = 1.0 / (1 - scb->getCurrentRangeSize());
		scb->setCurrentRangeStart(newVal / delta);
	}

	void CScrollableContainer::scrollBarMoved(juce::ScrollBar * b, double newRange)
	{
		virtualContainer->setBounds(
			0,
			static_cast<signed int>(-bGetValue() * (virtualContainer->getHeight() - getHeight())),
			virtualContainer->getWidth(),
			virtualContainer->getHeight());
	}

	CScrollableContainer:: ~CScrollableContainer()
	{
		if (virtualContainer)
			delete virtualContainer;
		if (scb)
			delete scb;
	}

	/*********************************************************************************************

		CTextControl

	*********************************************************************************************/
	CTextControl::CTextControl()
		: CBaseControl(this)
	{

	}
	void CTextControl::bSetText(const std::string & newText)
	{
		cpl::CMutex lockGuard(this);
		CTextLabel::setText(newText);
	}
	const std::string CTextControl::bGetText()
	{
		cpl::CMutex lockGuard(this);
		return text.toStdString();
	}
	void CTextControl::paint(juce::Graphics & g)
	{
		cpl::CMutex lockGuard(this);
		CTextLabel::paint(g);
	}
};