#pragma once


#include <TGUI/TGUI.hpp>
#include <TGUI/Any.hpp>
#include <TGUI/Backends/SFML.hpp>

#include <SFML/Graphics.hpp>

constexpr const char* CHARS_ARRAY =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    ":!? ";
constexpr size_t CHARS_ARRAY_LEN = 67;

struct CharSpriteSheet
{

    sf::Sprite* sprites[256] = { nullptr };
    sf::RenderTexture texFinal{};
    sf::Vector2i sizeFinal = { 0, 0 };

    int padding = 10;
    int characterSize = -1;

    CharSpriteSheet()
    {

    }

    void init(sf::Font& font, sf::Text text = {}, int padding = 5)
    {
        this->padding = padding;
        text.setFont(font);

        sf::Vector2i texSize = { padding, 0 };
        float lineTop = -1;
        characterSize = text.getCharacterSize();

        for (size_t i = 0; i < CHARS_ARRAY_LEN; ++i)
        {
            text.setString(CHARS_ARRAY[i]);
            sf::FloatRect bounds = text.getLocalBounds();
            int letterWidth = (int)(bounds.width + bounds.left*0);
            int letterHeight = (int)(bounds.height + bounds.top);
            if (sizeFinal.y < letterHeight)
                sizeFinal.y = letterHeight;
            
            sizeFinal.x += letterWidth;
            sizeFinal.y = letterHeight;

            texSize.x += letterWidth;
            texSize.x += padding;
            texSize.y = letterHeight;

            if (lineTop == -1)
                lineTop = bounds.top;
            else if (lineTop > bounds.top)
                lineTop = bounds.top;
        }
        texSize.y += 2 * padding;

        texFinal.create(
            (unsigned)texSize.x,
            (unsigned)texSize.y);



        texFinal.draw(text);
        texFinal.display();
        texFinal.clear(sf::Color::Transparent);

        texFinal.setSmooth(false);

        text.setString("");

        sf::Vector2f lastSize = { 0.0f, 0.0f };
        sf::Vector2f letterPos = { (float)padding, (float)padding };

        for (size_t i = 0; i < CHARS_ARRAY_LEN; ++i)
        {
            text.setString(CHARS_ARRAY[i]);

            sf::FloatRect bounds = text.getLocalBounds();
            
            sf::Vector2f size = { bounds.width, bounds.height };

            text.setPosition(letterPos);
            texFinal.draw(text);
            
            sf::Sprite* sprite = nullptr;
            sprites[CHARS_ARRAY[i]] = new sf::Sprite(
                texFinal.getTexture(),
                sf::IntRect{
                    (int)(bounds.left + letterPos.x),
                    (int)(letterPos.y + lineTop),
                    (int)size.x, (int)(lineTop - bounds.top + sizeFinal.y)
                }
            );

            letterPos.x += size.x;
            letterPos.x += padding;
        }
    }

    ~CharSpriteSheet()
    {
        for (size_t i = 0; i < CHARS_ARRAY_LEN; ++i)
        {
            if (sprites[i])
                delete sprites[i];
        }
    }

    sf::Sprite& operator[](char c) const
    {
        assert(sprites[c]);
        return *sprites[c];
    }

    sf::Sprite& getSprite(char c) const
    {
        assert(sprites[c]);
        return *sprites[c];
    }

    sf::IntRect getRect(char c) const
    {
        assert(sprites[c]);
        return sprites[c]->getTextureRect();
    }

    sf::Vector2i getSize(char c) const
    {
        assert(sprites[c]);
        sf::IntRect rect = getRect(c);
        return { rect.width, rect.height };
    }

    int getMaxHeight() const
    {
        return this->sizeFinal.y;
    }

    int getCharacterSize()
    {
        return this->characterSize;
    }
};

class FastLabel : public tgui::Label
{
public:
    using Ptr = std::shared_ptr<FastLabel>;
    using ConstPtr = std::shared_ptr<const FastLabel>;

    static constexpr const char StaticWidgetType[] = "FastLabel";

    CharSpriteSheet* spriteSheet = nullptr;

    sf::RenderWindow* renderWindow = nullptr;
    int spacing = 2;
    std::string stdString = "";
    tgui::Vector2i textRect = { 0, 0 };


    FastLabel(const char* typeName = "Label", bool initRenderer = true) :
        Label{ typeName, initRenderer }
    {
        setScrollbarPolicy(tgui::Scrollbar::Policy::Never);
    }

    void set(tgui::Label::Ptr label)
    {
        
        setHorizontalAlignment(label->getHorizontalAlignment());
        setVerticalAlignment(label->getVerticalAlignment());

        setVisible(label->isVisible());
        setEnabled(label->isEnabled());
        setParent(label->getParent());
        setPosition(label->getAbsolutePosition());
        setSize(label->getSize());

        //setPosition(label->getAbsolutePosition());
    }

    static FastLabel::Ptr copy(FastLabel::ConstPtr label)
    {
        if (label)
            return std::static_pointer_cast<FastLabel>(label->clone());
        else
            return nullptr;
    }

    static Ptr create(const tgui::String& text = "")
    {
        auto label = std::make_shared<FastLabel>();

        if (!text.empty())
            label->setText(text);

        return label;
    }

    void setCharSpriteSheet(CharSpriteSheet* spriteSheet)
    {
        this->spriteSheet = spriteSheet;
    }


    void setTextSpacing(int spacing)
    {
        this->spacing = spacing;
    }

    void setText(const tgui::String& string)
    {
        stdString = string.toStdString();

        float x = 0, y;
        for (size_t i = 0; i < stdString.size(); ++i)
        {
            x += spriteSheet->getSize(stdString[i]).x + spacing;
        }
        y = (float)spriteSheet->getMaxHeight();

        this->textRect = { (int)x, (int)y };
    }

    void drawText(const tgui::RenderStates& states) const
    {
        using namespace tgui;

        const float textOffset = Text::getExtraHorizontalPadding(m_fontCached, m_textSize, m_textStyleCached);

        RenderStates movedStates = states;
        sf::RenderStates sfStates;
        // Stupid tgui stuff
        {
            const float* matrix = movedStates.transform.getMatrix();
            sfStates.transform = sf::Transform{ matrix[0], matrix[4], std::round(matrix[12]),
                                               matrix[1], matrix[5], std::floor(matrix[13] + 0.1f),
                                               matrix[3], matrix[7], matrix[15] };
            sfStates.transform.translate(
                m_paddingCached.getLeft() + textOffset,
                m_paddingCached.getTop());
        }


        tgui::Vector2f vecAlgmnt = { 0.f, 0.f };
        switch (m_verticalAlignment)
        {
            case tgui::Label::VerticalAlignment::Bottom:
                vecAlgmnt.y = (float)getSize().y - spriteSheet->getCharacterSize();
            break;

            case tgui::Label::VerticalAlignment::Center:
                vecAlgmnt.y = (float)getSize().y - spriteSheet->getCharacterSize();
                vecAlgmnt.y /= 2.f;
            break;

            case tgui::Label::VerticalAlignment::Top:
                vecAlgmnt.y = 0;
            break;
        };

        switch (m_horizontalAlignment)
        {
        case tgui::Label::HorizontalAlignment::Right:
            vecAlgmnt.x = (float)getSize().y - textRect.y;
            vecAlgmnt.x *= 2;
            break;

        case tgui::Label::HorizontalAlignment::Center:
            vecAlgmnt.x = (float)getSize().y - textRect.y;
            break;

        case tgui::Label::HorizontalAlignment::Left:
            vecAlgmnt.x = 0;
            break;
        }
        

        sfStates.transform.translate(vecAlgmnt);

        sf::Vector2i textPos = { 0, 0 };
        for (size_t i = 0; i < stdString.size(); ++i)
        {
            sf::RenderStates s = sfStates;
            s.transform.translate(
                (float)textPos.x, 
                (float)0
            );
            renderWindow->draw((*spriteSheet)[stdString[i]], s);
            
            textPos.x += spriteSheet->getSize(stdString[i]).x + spacing;
        }

        
    }

    void draw(tgui::BackendRenderTargetBase& target, tgui::RenderStates states) const
    {
        using namespace tgui;
        
        const RenderStates statesForScrollbar = states;

        Vector2f innerSize = { getSize().x - m_bordersCached.getLeft() - m_bordersCached.getRight(),
                              getSize().y - m_bordersCached.getTop() - m_bordersCached.getBottom() };
        
        // Draw the borders
        if (m_bordersCached != Borders{ 0 })
        {
            target.drawBorders(states, m_bordersCached, getSize(), Color::applyOpacity(m_borderColorCached, m_opacityCached));
            states.transform.translate({ m_bordersCached.getLeft(), m_bordersCached.getTop() });
        }

        // Draw the background
        if (m_spriteBackground.isSet())
            target.drawSprite(states, m_spriteBackground);
        else if (m_backgroundColorCached.isSet() && (m_backgroundColorCached != Color::Transparent))
            target.drawFilledRect(states, innerSize, Color::applyOpacity(m_backgroundColorCached, m_opacityCached));;
        

        /*
        const tgui::Borders borders{ 2 }; // Borders are 2 pixels thick on any side

        target.drawBorders(states, borders, getSize(), tgui::Color::applyOpacity(tgui::Color::Blue, m_opacityCached));

        states.transform.translate(borders.getOffset());

        target.drawFilledRect(states,
            { getSize().x - borders.getLeft() - borders.getRight(), getSize().y - borders.getTop() - borders.getBottom() },
            tgui::Color::applyOpacity(tgui::Color::Red, m_opacityCached));
        */


        // Draw the text
        this->drawText(states);
    }

    /*
    void draw2(tgui::BackendRenderTargetBase& target, tgui::RenderStates states) const 
    {
        const tgui::Borders borders{ 2 }; // Borders are 2 pixels thick on any side

        target.drawBorders(states, borders, getSize(), tgui::Color::applyOpacity(tgui::Color::Blue, m_opacityCached));

        states.transform.translate(borders.getOffset());

        target.drawFilledRect(states,
            { getSize().x - borders.getLeft() - borders.getRight(), getSize().y - borders.getTop() - borders.getBottom() },
            tgui::Color::applyOpacity(tgui::Color::Red, m_opacityCached));

        Label::draw(target, states);

    }
    */

protected:

    Widget::Ptr clone() const override
    {
        return std::make_shared<FastLabel>(*this);
    }

};