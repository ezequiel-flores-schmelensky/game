//
// Created by David Burchill on 2022-10-21.
//

#include <SFML/Window/Keyboard.hpp>
#include "Scene_Menu.h"
#include "GameEngine.h"
#include <memory>
#include "Entity.h"



Scene_Menu::Scene_Menu(GameEngine* gameEngine)
        : Scene(gameEngine)
{
    init();
}

void Scene_Menu::init()
{
    registerAction(sf::Keyboard::W,			"UP");
    registerAction(sf::Keyboard::Up,		"UP");

    registerAction(sf::Keyboard::S,			"DOWN");
    registerAction(sf::Keyboard::Down,	 	"DOWN");

    registerAction(sf::Keyboard::D,			"PLAY");

    registerAction(sf::Keyboard::Escape,	"QUIT");

    m_title = "Bone Rescue";


    m_menuText.setFont(m_game->assets().getFont("Climate"));

    const size_t CHAR_SIZE{ 64 };
    m_menuText.setCharacterSize(CHAR_SIZE);

}


void Scene_Menu::registerItem(SceneID key, std::string item) {
    m_menuItems.push_back(std::make_pair(key, item));
}


void Scene_Menu::update(sf::Time dt) {
    m_entityManager.update();
}


void Scene_Menu::sRender() {
    sf::View view = m_game->window().getView();
    view.setCenter(m_game->window().getSize().x / 2.f, m_game->window().getSize().y / 2.f);
    m_game->window().setView(view);

    static const sf::Color selectedColor(255, 255, 255);
    static const sf::Color normalColor(124, 59, 15);
    static const sf::Color backgroundColor(201, 174, 121);

    sf::Text footer("UP: W    DOWN: S   PLAY:D    QUIT: ESC",
                    m_game->assets().getFont("Megaman"),
                    20);
    footer.setFillColor(normalColor);
    footer.setPosition(220, 700);

    m_game->window().clear(backgroundColor);

    m_menuText.setFillColor(normalColor);
    m_menuText.setString(m_title);
    m_menuText.setPosition(200, 40);
    m_game->window().draw(m_menuText);

    for (size_t i{ 0 }; i < m_menuItems.size(); ++i)
    {
        m_menuText.setFillColor((i == m_menuIndex ? selectedColor : normalColor));
        m_menuText.setPosition(300, 100 + (i+1) * 96);
        m_menuText.setString(m_menuItems.at(i).second);
        m_game->window().draw(m_menuText);
    }

    m_game->window().draw(footer);

    m_player = m_entityManager.addEntity("player");
    m_player->addComponent<CTransform>(sf::Vector2f(220, 620), sf::Vector2f(0.f, 0.f), 0, 0);
    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("Dog"));
    auto& tfm = m_player->getComponent<CTransform>();
    auto& sprit = m_player->getComponent<CAnimation>().animation.getSprite();
    sprit.setPosition(tfm.pos);
    sprit.setRotation(tfm.rot);
    m_game->window().draw(sprit);

    m_cat = m_entityManager.addEntity("enemy");
    m_cat->addComponent<CTransform>(sf::Vector2f(340, 620), sf::Vector2f(0.f, 0.f), 0, 0);
    m_cat->addComponent<CAnimation>(m_game->assets().getAnimation("GangsterCat"));
    auto& tfm1 = m_cat->getComponent<CTransform>();
    auto& sprit1 = m_cat->getComponent<CAnimation>().animation.getSprite();
    sprit1.setPosition(tfm1.pos);
    sprit1.setRotation(tfm1.rot);
    m_game->window().draw(sprit1);

    m_bone = m_entityManager.addEntity("obstacle");
    m_bone->addComponent<CTransform>(sf::Vector2f(480, 640), sf::Vector2f(0.f, 0.f), 0, 0);
    m_bone->addComponent<CAnimation>(m_game->assets().getAnimation("Bone"));
    auto& tfm2 = m_bone->getComponent<CTransform>();
    auto& sprit2 = m_bone->getComponent<CAnimation>().animation.getSprite();
    sprit2.setPosition(tfm2.pos);
    sprit2.setRotation(tfm2.rot);
    m_game->window().draw(sprit2);

    m_big_cat = m_entityManager.addEntity("enemy2");
    m_big_cat->addComponent<CTransform>(sf::Vector2f(700, 580), sf::Vector2f(0.f, 0.f), 0, 0);
    m_big_cat->addComponent<CAnimation>(m_game->assets().getAnimation("BigCat"));
    auto& tfm3 = m_big_cat->getComponent<CTransform>();
    auto& sprit3 = m_big_cat->getComponent<CAnimation>().animation.getSprite();
    sprit3.setPosition(tfm3.pos);
    sprit3.setRotation(tfm3.rot);
    m_game->window().draw(sprit3);

}


void Scene_Menu::sDoAction(const Action &action) {
    if (action.type() == "START")
    {
        if (action.name() == "UP")
        {
            m_menuIndex = (m_menuIndex + m_menuItems.size() - 1) % m_menuItems.size();
        }
        else if (action.name() == "DOWN")
        {
            m_menuIndex = (m_menuIndex + 1) % m_menuItems.size();
        }
        // TODO generalize
        else if (action.name() == "PLAY")
        {
           m_game->changeScene( m_menuItems.at(m_menuIndex).first );
        }
        else if (action.name() == "QUIT")
        {
            onEnd();
        }
    }
}



void Scene_Menu::onEnd()
{
    m_game->window().close();
}
