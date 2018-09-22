

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
		setBounds(in);
		scb->setBounds(in.getWidth() - 20, 0, 20, in.getHeight());
		virtualContainer->setBounds(0, 0, in.getWidth() - scb->getWidth(), 1300);
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