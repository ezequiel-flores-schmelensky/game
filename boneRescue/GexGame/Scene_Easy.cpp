//
// Created by David Burchill on 2022-11-09.
//

#include "Scene_Easy.h"
#include "Entity.h"
#include "MusicPlayer.h"
#include "SoundPlayer.h"

#include <fstream>
#include <iostream>
#include <random>

namespace {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution hasPickup(0, 2);
}



Scene_Easy::Scene_Easy(GameEngine *gameEngine, const std::string &configPath)
        : Scene(gameEngine), m_worldView(gameEngine->window().getDefaultView()), m_worldBounds({0, 0}, {0, 0}) {

    loadFromFile(configPath);

    m_spawnPosition = sf::Vector2f(m_worldView.getSize().x / 2.f,
                                   m_worldBounds.height - m_worldView.getSize().y / 2.f);

    m_spawnPlayerPosition = sf::Vector2f(110.f,
                                        m_worldBounds.height - 100.f);
    

    m_worldView.setCenter(m_spawnPosition);

    registerActions();
    spawnPlayer();

    //MusicPlayer::getInstance().play("gameTheme");
    //MusicPlayer::getInstance().setVolume(1);

}


void Scene_Easy::loadFromFile(const std::string &configPath) {
    std::ifstream config(configPath);
    if (config.fail()) {
        std::cerr << "Open file " << configPath << " failed\n";
        config.close();
        exit(1);
    }


    std::string token{""};
    config >> token;
    while (config) {
        if (token == "#") {
            std::string tmp;
            std::getline(config, tmp);
            std::cout << tmp << "\n";
        } else if (token == "World") {
            config >> m_worldBounds.width >> m_worldBounds.height;
        } else if (token == "ScrollSpeed") {
            config >> m_scrollSpeed;
        } else if (token == "PlayerSpeed") {
            config >> m_playerSpeed;
        } else if (token == "BarkSpeed") {
            config >> m_barkSpeed;
        } else if (token == "Obstacle") {
            std::string name;
            sf::Vector2f pos;
            float rot, cr;
            config >> name >> rot >> pos.x >> pos.y >> cr;
            auto vel = sf::Vector2f(0.f, 0.f);

            auto obsctacle = m_entityManager.addEntity("obstacle");
            obsctacle->addComponent<CTransform>(pos, vel, rot);
            obsctacle->addComponent<CAnimation>(m_game->assets().getAnimation(name));
            obsctacle->addComponent<CCollision>(cr);

            sf::FloatRect oGBounds = obsctacle->getComponent<CAnimation>().animation.getSprite().getGlobalBounds();
            auto& oPos = obsctacle->getComponent<CTransform>().pos;

            sf::Vector2f recSize, recPos;
            recSize.x = oGBounds.width - 10.f;
            recSize.y = oGBounds.height;
            recPos.x = oPos.x - oGBounds.width / 2.f + 5.f;
            recPos.y = oPos.y - oGBounds.height / 2.f;
            
            obsctacle->addComponent<CRectShape>(recSize, recPos);

        } else if (token == "Enemy") {
            std::string name;
            sf::Vector2f pos, vel;
            int flip; 
            float rot, cr;
            config >> name >> rot >> flip >> pos.x >> pos.y >> cr >> vel.x >> vel.y;
            

            auto enemy = m_entityManager.addEntity("enemy");
            enemy->addComponent<CTransform>(pos, vel, rot);
            enemy->addComponent<CAnimation>(m_game->assets().getAnimation(name));
            auto& eSprit = enemy->getComponent<CAnimation>().animation.getSprite();
            if (flip == 1)
                eSprit.setScale(-1.0f, 1.0f);
            enemy->addComponent<CCollision>(cr);
            enemy->addComponent<CHealth>(20);

            sf::FloatRect eGBounds = eSprit.getGlobalBounds();
            auto& ePos = enemy->getComponent<CTransform>().pos;

            sf::Vector2f recSize, recPos;
            recSize.x = eGBounds.width - 10.f;
            recSize.y = eGBounds.height;
            recPos.x = ePos.x - eGBounds.width / 2.f + 5.f;
            recPos.y = ePos.y - eGBounds.height / 2.f;

            enemy->addComponent<CRectShape>(recSize, recPos, name, flip == 1);

        } else if (token == "Bkg") {
            std::string name;
            sf::Vector2f pos;
            config >> name >> pos.x >> pos.y;
            auto e = m_entityManager.addEntity("bkg");

            auto &sprite = e->addComponent<CSprite>(m_game->assets().getTexture(name)).sprite;

            sprite.setOrigin(0.f, 0.f);
            sprite.setPosition(pos);
        } else if (token == "Entities") {
            sf::IntRect tr;
            std::string name;
            config >> name >> tr.left >> tr.top >> tr.width >> tr.height;
            m_textRects[name] = tr;
        } else if (token == "Directions") {
            std::string name;
            config >> name;
            float time;

            // auto pilots have 5 legs
            for (int i{0}; i < 5; ++i) {
                config >> m_enemyConfig[name].dirs[i];
                config >> time;
                m_enemyConfig[name].times[i] = sf::seconds(time);
            }
        }
        config >> token;
    }
    config.close();
}


void Scene_Easy::init(const std::string &configPath) {

}


void Scene_Easy::keepEntitiesInBounds() {
    auto vb = getViewBounds();
    

    for (auto e : m_entityManager.getEntities("enemy")) {
        if (e->hasComponent<CCollision>()) {
            auto& tfm = e->getComponent<CTransform>();
            auto& rComp = e->getComponent<CRectShape>();
            auto& eSprit = e->getComponent<CAnimation>().animation.getSprite();
            
            auto& rSize = rComp.shape.getSize();
            bool forniture = false;
            if (rComp.name == "GangsterCat") {//name = "GangsterCat"
                auto eRect = eSprit.getGlobalBounds();

                for (auto o : m_entityManager.getEntities("obstacle")) {
                    if (o->hasComponent<CTransform>() && o->hasComponent<CCollision>()) {

                        sf::FloatRect oRect = o->getComponent<CRectShape>().shape.getGlobalBounds();
                        auto& oPos = o->getComponent<CTransform>().pos;

                        if (eRect.intersects(oRect)) {
                            //Right;
                            if (collide(oRect.left, eRect.left, eRect.width) &&
                                absoluteValue(tfm.pos.y, oPos.y) < (oRect.height / 2.f + eRect.height / 2.f - 15.f)) {
                                forniture = true;
                                break;
                            }
                            //Left
                            if (collide(eRect.left, oRect.left, oRect.width) &&
                                absoluteValue(tfm.pos.y, oPos.y) < (oRect.height / 2.f + eRect.height / 2.f - 15.f)) {
                                forniture = true;
                                break;
                            }
                        }
                    }
                }
            }

            if (tfm.pos.x < (vb.left + rSize.x / 2.f + 100) || 
                (tfm.pos.x) >(vb.left + vb.width - rSize.x / 2.f - 100) || 
                forniture) {
                tfm.vel.x *= -1;
                if (!rComp.flipped)
                    eSprit.setScale(-1.0f, 1.0f);
                else
                    e->addComponent<CAnimation>(m_game->assets().getAnimation(rComp.name));
                rComp.flipped = !rComp.flipped;
            }
            //if (tfm.pos.y - rSize.y / 2.f < vb.top || (tfm.pos.y + r) >(vb.top + vb.height)) {
            //    tfm.vel.y *= -1;
            //}
        }
    }
}


void Scene_Easy::onEnd() {

}


void Scene_Easy::sMovement(sf::Time dt) {
    playerMovement();

    // move all objects
    for (auto e: m_entityManager.getEntities()) {
        if (e->hasComponent<CTransform>()) {
            auto &tfm = e->getComponent<CTransform>();

            tfm.pos += tfm.vel * dt.asSeconds();
            tfm.rot += tfm.rotVel * dt.asSeconds();
        }
    }

    if ((m_player->getComponent<CTransform>().pos.y - m_player->getComponent<CCollision>().radius) <= 0) {
        m_player->addComponent<CState>().state = "win";
        setPaused(true);
    }
}


void Scene_Easy::playerMovement() {

    // no movement if player is dead
    if (m_player->hasComponent<CState>() && m_player->getComponent<CState>().state == "dead")
        return;
   
    // player movement
    sf::Vector2f pv;
    auto &pInput = m_player->getComponent<CInput>();
    if (pInput.left) pv.x -= 1;
    if (pInput.right) pv.x += 1;
    if (pInput.up) pv.y -= 1;
    if (pInput.down) pv.y += 1;
    
    if (m_stopers["T"] && pInput.up) pv.y = 0;
    if (m_stopers["B"] && pInput.down) pv.y = 0;
    if (m_stopers["L"] && pInput.left) pv.x = 0;
    if (m_stopers["R"] && pInput.right) pv.x = 0;

    
    pv = normalize(pv);
    m_player->getComponent<CTransform>().vel = m_playerSpeed * pv;

}


void Scene_Easy::sCollisions() {
    checkDogCollision();
    checkBarkCollision();
    //checkPickupCollision();
    //checkMissileCollision();
    //checkPickupCollision();
}


void Scene_Easy::checkIfDead(NttPtr e) {

    std::uniform_int_distribution flip(0, 1);

    // check for planes that need to be destroyed
    if (e->hasComponent<CHealth>()) {
        if (e->getComponent<CHealth>().hp <= 0) {
            //e->addComponent<CAnimation>(m_game->assets().getAnimation("explosion"));
            e->getComponent<CTransform>().vel = sf::Vector2f(0.f, 0.f);
            e->addComponent<CState>().state = "dead";
            e->removeComponent<CCollision>();

            /*if (flip(rng) == 0)
                SoundPlayer::getInstance().play("Explosion1", e->getComponent<CTransform>().pos);
            else
                SoundPlayer::getInstance().play("Explosion2", e->getComponent<CTransform>().pos);*/

        }
    }
}

void Scene_Easy::checkPickupCollision() {// check for plane collision

    if (m_player->hasComponent<CCollision>()) {
        auto pPos = m_player->getComponent<CTransform>().pos;
        auto pCr = m_player->getComponent<CCollision>().radius;
        
        for (auto e : m_entityManager.getEntities("pickup")) {
            if (e->hasComponent<CTransform>() && e->hasComponent<CCollision>()) {
                auto ePos = e->getComponent<CTransform>().pos;
                auto eCr = e->getComponent<CCollision>().radius;

                // planes have collided
                if (dist(ePos, pPos) < (eCr + pCr)) {
                    auto& pHP  = m_player->getComponent<CHealth>().hp;
                    auto& pGun = m_player->getComponent<CGun>(); 
                    auto& pM   = m_player->getComponent<CMissiles>().missileCount;

                    auto& eHP = e->getComponent<CHealth>().hp; 
                    auto   eP = e->getComponent<CPickup>().pickup;
                    e->removeComponent<CAnimation>();
                    e->removeComponent<CCollision>();
                   
                    //0 - "healthRefill";
                    //1 - "missileRefill";
                    //2 - "fireRate";
                    //3 - "fireSpread";
                    switch (eP)
                    {
                        case 0:
                            pHP += 25;
                            break;
                        case 1:
                            pM += 2;
                            break;
                        case 2:
                            if (pGun.fireRate < 9)
                                pGun.fireRate += 1;
                            break;
                        case 3:
                            if (pGun.spreadLevel < 3)
                                pGun.spreadLevel += 1;
                            break;
                        default:
                            break;
                    }
                   
                    //SoundPlayer::getInstance().play("CollectPickup", e->getComponent<CTransform>().pos);
                    e->destroy();
                }
            }
        }
    }
}

void Scene_Easy::checkDogCollision() {// check for obstacle collision

    if (m_player->hasComponent<CCollision>()) {
        sf::FloatRect dGBounds = m_player->getComponent<CAnimation>().animation.getSprite().getGlobalBounds();
        auto& dPos = m_player->getComponent<CTransform>().pos;
        sf::FloatRect dRect(dPos.x - dGBounds.width/2.f, dPos.y - dGBounds.height/2.f, dGBounds.width, 85);

        m_stopers["T"] = false; 
        m_stopers["R"] = false;
        m_stopers["L"] = false;
        m_stopers["B"] = false;

        
        sf::FloatRect oRect;
        
        for (auto e: m_entityManager.getEntities("obstacle")) {
            if (e->hasComponent<CTransform>() && e->hasComponent<CCollision>()) {
                
                sf::FloatRect oRect = e->getComponent<CRectShape>().shape.getGlobalBounds();
                auto& oPos = e->getComponent<CTransform>().pos;

                if (dRect.intersects(oRect)) {
                    //Right;
                    if (collide(oRect.left, dRect.left, dRect.width) &&
                        absoluteValue(dPos.y, oPos.y) < (oRect.height /2.f + dRect.height / 2.f - 15.f)) {
                        m_stopers["R"] = true;
                    }
                    if (collide(dRect.left, oRect.left, oRect.width) &&
                        absoluteValue(dPos.y, oPos.y) < (oRect.height / 2.f + dRect.height / 2.f - 15.f)) {
                        m_stopers["L"] = true;
                    }
                    // Bottom
                    if (collide(oRect.top, dRect.top, dRect.height) &&
                        absoluteValue(dPos.x, oPos.x) < (oRect.width / 2.f + dRect.width / 2.f - 15.f)) {
                        m_stopers["B"] = true;
                    }
                    if (collide(dRect.top, oRect.top, oRect.height) &&
                        absoluteValue(dPos.x, oPos.x) < (oRect.width / 2.f + dRect.width / 2.f - 15.f)) {
                        m_stopers["T"] = true;
                    }
                }
            }
        }

        auto pPos = m_player->getComponent<CTransform>().pos;
        auto pCr = m_player->getComponent<CCollision>().radius;

        for (auto e : m_entityManager.getEntities("enemy")) {
            if (e->hasComponent<CTransform>() && e->hasComponent<CCollision>()) {
                auto ePos = e->getComponent<CTransform>().pos;
                auto eCr = e->getComponent<CCollision>().radius;

                // planes have collided
                if (dist(ePos, pPos) < (eCr + pCr)) {
                    auto& pHP = m_player->getComponent<CHealth>().hp;
                    auto& eHP = e->getComponent<CHealth>().hp;

                    // however many HP the plane has left,
                    // that's how much damage it inflicts on other plane
                    int tmpHP = pHP;
                    pHP -= pHP;
                    eHP -= eHP;

                    checkIfDead(e);
                    int hPickup = hasPickup(rng);
                    
                    if (e->getComponent<CState>().state == "dead" && hPickup == 1) {
                        auto& rComp = e->getComponent<CRectShape>();
                        droppingAPickup(e->getComponent<CTransform>().pos, rComp.name);
                        e->destroy();
                    }

                    checkIfDead(m_player);
                    if (m_player->getComponent<CState>().state == "dead") {
                        setPaused(true);
                    }

                }
            }
        }
    }
}

void Scene_Easy::checkBarkCollision() {
    // Player Bullets
    for (auto bullet: m_entityManager.getEntities("roarBlue")) {
        if (bullet->hasComponent<CTransform>() && bullet->hasComponent<CCollision>()) {
            auto bPos = bullet->getComponent<CTransform>().pos;
            auto bCr = bullet->getComponent<CCollision>().radius;

            for (auto e: m_entityManager.getEntities("enemy")) {
                if (e->hasComponent<CTransform>() && e->hasComponent<CCollision>()) {
                    auto ePos = e->getComponent<CTransform>().pos;
                    auto eCr = e->getComponent<CCollision>().radius;

                    if (dist(ePos, bPos) < (eCr + bCr)) {
                        
                        e->getComponent<CHealth>().hp -= 20;
                        bullet->destroy();
                        checkIfDead(e);
                        
                        if (e->getComponent<CState>().state == "dead") {
                            e->destroy();
                        }
                        int hPickup = hasPickup(rng);
                        if (e->getComponent<CState>().state == "dead" && hPickup == 1) {
                            auto& rComp = e->getComponent<CRectShape>();
                            droppingAPickup(e->getComponent<CTransform>().pos, rComp.name);
                        }
                    }
                }
            }
        }
    }


    // Enemy Bullets
    /*if (m_player->hasComponent<CCollision>()) {
        auto pPos = m_player->getComponent<CTransform>().pos;
        auto pCr = m_player->getComponent<CCollision>().radius;

        for (auto bullet: m_entityManager.getEntities("enemyBullet")) {
            if (bullet->hasComponent<CTransform>() && bullet->hasComponent<CCollision>()) {
                auto bPos = bullet->getComponent<CTransform>().pos;
                auto bCr = bullet->getComponent<CCollision>().radius;

                if (dist(pPos, bPos) < (pCr + bCr)) {
                    m_player->getComponent<CHealth>().hp -= 10;
                    bullet->destroy();
                    checkIfDead(m_player);
                    if (m_player->getComponent<CState>().state == "dead") {
                        setPaused(true);
                    }
                }
            }
        }
    }*/
}


void Scene_Easy::checkMissileCollision() {// missiles
    for (auto m: m_entityManager.getEntities("missile")) {
        if (m->hasComponent<CTransform>() && m->hasComponent<CCollision>()) {
            auto mPos = m->getComponent<CTransform>().pos;
            auto mCr = m->getComponent<CCollision>().radius;

            for (auto e: m_entityManager.getEntities("enemy")) {
                if (e->hasComponent<CTransform>() && e->hasComponent<CCollision>()) {
                    auto ePos = e->getComponent<CTransform>().pos;
                    auto eCr = e->getComponent<CCollision>().radius;

                    if (dist(ePos, mPos) < (eCr + mCr)) {
                        e->getComponent<CHealth>().hp = -1;
                        m->destroy();
                        checkIfDead(e);

                        //int hPickup = hasPickup(rng);
                        //if (e->getComponent<CState>().state == "dead" && hPickup == 1) {
                        //    droppingAPickup(e->getComponent<CTransform>().pos);
                        //}
                        
                    }
                }
            }
        }
    }
}


void Scene_Easy::sUpdate(sf::Time dt) {

}


void Scene_Easy::registerActions() {

    //TODO register action FIRE to fire the gun

    registerAction(sf::Keyboard::P,      "PAUSE");
    registerAction(sf::Keyboard::Escape, "BACK");
    registerAction(sf::Keyboard::Q,      "QUIT");

    registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE");
    registerAction(sf::Keyboard::C, "TOGGLE_COLLISION");
    registerAction(sf::Keyboard::G, "TOGGLE_GRID");

    registerAction(sf::Keyboard::A,     "LEFT");
    registerAction(sf::Keyboard::Left,  "LEFT");
    registerAction(sf::Keyboard::D,     "RIGHT");
    registerAction(sf::Keyboard::Right, "RIGHT");
    registerAction(sf::Keyboard::W,     "UP");
    registerAction(sf::Keyboard::Up,    "UP");
    registerAction(sf::Keyboard::S,     "DOWN");
    registerAction(sf::Keyboard::Down,  "DOWN");

    //registerAction(sf::Keyboard::Space, "BARK");
    registerAction(sf::Keyboard::M,     "LAUNCH");
    registerAction(sf::Mouse::Right,     "BARK");
}


void Scene_Easy::spawnPlayer() {
    m_player = m_entityManager.addEntity("player");
    m_player->addComponent<CTransform>(
            m_spawnPlayerPosition,
            sf::Vector2f(0.f, 0.f),
            0, 0);

    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("Dog"));
    m_player->addComponent<CState>("straight");
    m_player->addComponent<CCollision>(30);
    m_player->addComponent<CInput>();
    m_player->addComponent<CMissiles>();
    m_player->addComponent<CHealth>().hp = 100;
    auto &gun = m_player->addComponent<CGun>();

}


void Scene_Easy::drawAABB() {

    for (auto e: m_entityManager.getEntities()) {

        // draw the world
        if (e->getComponent<CCollision>().has) {

            auto &tfm = e->getComponent<CTransform>();
            auto cr = e->getComponent<CCollision>().radius;

            sf::CircleShape cir(cr);
            cir.setOutlineThickness(2);
            cir.setOutlineColor(sf::Color::Red);
            cir.setFillColor(sf::Color::Transparent);

            cir.setPosition(tfm.pos);
            centerOrigin(cir);

            m_game->window().draw(cir);
        }
    }

}


void Scene_Easy::adjustPlayer() {
    auto vb = getViewBounds();

    auto &pos = m_player->getComponent<CTransform>().pos;
    auto cr = m_player->getComponent<CCollision>().radius;
    
    pos.x = std::max(pos.x, vb.left + cr + 100);
    pos.x = std::min(pos.x, vb.left + vb.width - cr - 110);
    pos.y = std::max(pos.y, vb.top + cr);
    pos.y = std::min(pos.y, vb.top + vb.height - cr - 70);
}

void Scene_Easy::adjustScroll(sf::Time& dt) {
    auto vb = getViewBounds();
    auto& pos = m_player->getComponent<CTransform>().pos;
    auto cr = m_player->getComponent<CCollision>().radius;

    if (pos.y - 100 < vb.top)
        m_worldView.move(0.f, m_scrollSpeed * dt.asSeconds() * -1);
    if (pos.y + 100 > vb.top + vb.height - cr && pos.y < m_worldBounds.height - 100 - cr)
        m_worldView.move(0.f, m_scrollSpeed * dt.asSeconds());

    auto y = m_worldView.getCenter().y;
}

void Scene_Easy::checkPlayerState() {// set the player state
    if (m_player->hasComponent<CState>()) {

        auto xVel = m_player->getComponent<CTransform>().vel.x;
        auto yVel = m_player->getComponent<CTransform>().vel.y;
        std::string newState = "stopped";
        if (yVel < -0.2f) newState = "straight";
        if (xVel < -0.2f) newState = "left";
        if (xVel >  0.2f) newState = "right";

        auto &state = m_player->getComponent<CState>().state;
        if (state != "dead") {
            if (newState != state) { // only if the state has changed, change the animation
                state = newState;
                if (state == "stopped")
                    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("Dog"));
                if (state == "straight")
                    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("DogTop"));
                if (state == "left")
                    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("DogLeft"));
                if (state == "right")
                    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("DogRight"));
            }
        }
    }
}


sf::FloatRect Scene_Easy::getViewBounds() {
    sf::FloatRect bounds;
    bounds.left = m_worldView.getCenter().x - m_worldView.getSize().x / 2.f;
    bounds.top = m_worldView.getCenter().y - m_worldView.getSize().y / 2.f;
    bounds.width = m_worldView.getSize().x;
    bounds.height = m_worldView.getSize().y;
    return bounds;
}


void Scene_Easy::update(sf::Time dt) {
    if (m_isPaused)
        return;

    m_entityManager.update();

    adjustScroll(dt);
    adjustPlayer();
    keepEntitiesInBounds();
    checkPlayerState();
    sMovement(dt);
    sCollisions();
    sGunUpdate(dt);
    //spawnEnemies();
   /*sGunUpdate(dt);
    sAnimation(dt);
    sGuideMissiles(dt);
    sAutoPilot(dt);
    spawnEnemies();

    sRemoveEntitiesOutOfGame();*/
}


void Scene_Easy::sDoAction(const Action &action) {

    // On Key Press
    if (action.type() == "START") {

        bool gameOver = m_player->getComponent<CState>().state == "dead" || m_player->getComponent<CState>().state == "win";

        if (gameOver) {
            if (action.name() == "QUIT") { m_game->quitLevel(); }
            return;
        }  

        if (action.name() == "PAUSE") { setPaused(!m_isPaused); }
        else if (action.name() == "QUIT") { m_game->quitLevel(); }
        else if (action.name() == "BACK") { m_game->backLevel(); }

        else if (action.name() == "TOGGLE_TEXTURE") { m_drawTextures = !m_drawTextures; }
        else if (action.name() == "TOGGLE_COLLISION") { m_drawAABB = !m_drawAABB; }
        else if (action.name() == "TOGGLE_GRID") { m_drawGrid = !m_drawGrid; }

            // Player control
        else if (action.name() == "LEFT") { m_player->getComponent<CInput>().left = true; }
        else if (action.name() == "RIGHT") { m_player->getComponent<CInput>().right = true; }
        else if (action.name() == "UP") { m_player->getComponent<CInput>().up = true; }
        else if (action.name() == "DOWN") { m_player->getComponent<CInput>().down = true; }

            // firing weapons
        //else if (action.name() == "BARK") { bark(); }
        else if (action.name() == "LAUNCH") { fireMissile(); }

    }

        // on Key Release
    else if (action.type() == "END") {
        if (action.name() == "LEFT") { 
            m_player->getComponent<CInput>().left = false; 
        }
        else if (action.name() == "RIGHT") { m_player->getComponent<CInput>().right = false; }
        else if (action.name() == "UP") { m_player->getComponent<CInput>().up = false; }
        else if (action.name() == "DOWN") { m_player->getComponent<CInput>().down = false; }
    }

    else if (action.type() == "CLICK") {
        if (action.name() == "BARK") { 
            m_clickPosition = action.pos();
            bark(); 
        }
    }

}


void Scene_Easy::sRender() {

    m_game->window().setView(m_worldView);

    // draw world
    auto bgColor = sf::Color(100, 100, 255);
    if (m_isPaused)
        bgColor = sf::Color(150, 50, 255);

    m_game->window().clear(bgColor);

    // draw bkg first
    for (auto e: m_entityManager.getEntities("bkg")) {
        if (e->getComponent<CSprite>().has) {
            auto &sprite = e->getComponent<CSprite>().sprite;
            m_game->window().draw(sprite);
        }
    }


    for (auto e: m_entityManager.getEntities()) {

        // draw all entities with textures
        if (e->getComponent<CAnimation>().has) {
            auto &tfm = e->getComponent<CTransform>();
            auto &anim = e->getComponent<CAnimation>().animation;
            anim.getSprite().setPosition(tfm.pos);
            anim.getSprite().setRotation(tfm.rot);
            m_game->window().draw(anim.getSprite());

            // draw HP
            static sf::Text text("HP: ", m_game->assets().getFont("Arial"), 15);
            if (e->hasComponent<CHealth>()) {
                int hp = e->getComponent<CHealth>().hp;
                std::string str = "HP: " + std::to_string(hp);
                text.setString(str);
                centerOrigin(text);

                sf::Vector2f offset(0.f, 40.f);
                if (e->getTag() == "enemy")
                    offset *= -1.f;
                text.setPosition(tfm.pos + offset);
                m_game->window().draw(text);
            }

            // draw ammo count if missiles
            if (e->hasComponent<CMissiles>()) {
                int count = e->getComponent<CMissiles>().missileCount;
                std::string str = "M: " + std::to_string(count);
                text.setString(str);
                centerOrigin(text);

                sf::Vector2f offset(0.f, 55.f);
                if (e->getTag() == "enemy")
                    offset *= -1.f;
                text.setPosition(tfm.pos + offset);
                m_game->window().draw(text);
            }
        }
    }

    if (m_isPaused) {
        //MusicPlayer::getInstance().setVolume(0);
        std::string pausedTxt{ "PAUSED" };
        int sizeTxt = 128;
        if (m_player->hasComponent<CState>() && m_player->getComponent<CState>().state == "dead") {
            pausedTxt = "Game over You Lose!";
            sizeTxt = 30;
        } else if (m_player->hasComponent<CState>() && m_player->getComponent<CState>().state == "win") {
            pausedTxt = "GAM OVER YOU WIN!!!!";
            sizeTxt = 26;
        }
        sf::Text paused(pausedTxt, m_game->assets().getFont("Megaman"), sizeTxt);
        centerOrigin(paused);
        auto bounds = getViewBounds();
        paused.setPosition(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        m_game->window().draw(paused);
    }// else {
     //   MusicPlayer::getInstance().setVolume(1);;
    //}

    // draw bounding boxes
    if (m_drawAABB) {
        drawAABB();
    }


}


void Scene_Easy::sAnimation(sf::Time dt) {

    for (auto e: m_entityManager.getEntities()) {

        // draw the world
        if (e->getComponent<CAnimation>().has) {
            auto &anim = e->getComponent<CAnimation>();
            anim.animation.update(dt);
            if (anim.animation.hasEnded())
                e->destroy();
        }
    }
}


void Scene_Easy::bark() {
    m_player->getComponent<CGun>().isFiring = true;
}


void Scene_Easy::createBullet(sf::Vector2f pos, bool isEnemy) {
    float speed = (isEnemy) ? m_barkSpeed : -m_barkSpeed;
    auto vb = getViewBounds();
    
    m_clickPosition.y = vb.top + m_clickPosition.y;

    auto targetDir = normalize(m_clickPosition - pos);
    sf::Vector2f bv;
    bv = m_barkSpeed * normalize(targetDir + bv);

    auto bullet = m_entityManager.addEntity(isEnemy ? "enemyBullet" : "roarBlue");
    bullet->addComponent<CTransform>(pos, bv, bearing(bv));
    bullet->addComponent<CAnimation>(m_game->assets().getAnimation("RoarBlue"));
    bullet->addComponent<CCollision>(3);

    //std::string sfx = (isEnemy) ? "EnemyGunfire" : "AlliedGunfire";
    //SoundPlayer::getInstance().play("AlliedGunfire", pos);
    //SoundPlayer::getInstance().play(sfx, pos);
}


void Scene_Easy::sGunUpdate(sf::Time dt) {
    for (auto e: m_entityManager.getEntities()) {
        if (e->hasComponent<CGun>()) {

            //
            // every time
            //
            auto &gun = e->getComponent<CGun>();
            bool isEnemy = (e->getTag() == "enemy");
            if (isEnemy)
                gun.isFiring = true;

            gun.countdown -= dt;

            //
            // when firing
            //
            if (gun.isFiring && gun.countdown < sf::Time::Zero) {
                gun.isFiring = false;
                gun.countdown = m_fireInterval / (1.f + gun.fireRate);

                auto pos = e->getComponent<CTransform>().pos;
                switch (gun.spreadLevel) {
                    case 1:
                        createBullet(pos, isEnemy);
                        break;

                    case 2:
                        createBullet(pos + sf::Vector2f(-20.f, 0.f), isEnemy);
                        createBullet(pos + sf::Vector2f(20.f, 0.f), isEnemy);
                        break;

                    case 3:
                        createBullet(pos + sf::Vector2f(0.f, -35.f), isEnemy);
                        createBullet(pos + sf::Vector2f(-20.f, 0.f), isEnemy);
                        createBullet(pos + sf::Vector2f(20.f, 0.f), isEnemy);
                        break;

                    default:
                        std::cerr << "Bad spread level firing gun\n";
                        break;
                }

            }
        }
    }
}


void Scene_Easy::fireMissile() {

    if (m_player->hasComponent<CMissiles>()) {
        size_t &ammo = m_player->getComponent<CMissiles>().missileCount;
        if (ammo > 0) {
            ammo -= 1;
            auto pos = m_player->getComponent<CTransform>().pos;

            auto missile = m_entityManager.addEntity("missile");
            missile->addComponent<CTransform>(
                    pos + sf::Vector2f(0.f, -60.f),
                    sf::Vector2f(0.f, -m_missileSpeed));
            missile->addComponent<CAnimation>(m_game->assets().getAnimation("Missile"));
            missile->addComponent<CCollision>(14);

            //std::cout << "origin: " << missile->getComponent<CAnimation>().animation.getSprite().getOrigin()
            //          << "\n";

            //SoundPlayer::getInstance().play("LaunchMissile", pos);
        }
    }
}

void Scene_Easy::droppingAPickup(sf::Vector2f pos, std::string enemyType) {

    
    int pickupType = 0;
    std::string animation{ "" };

    if (enemyType == "GangsterCat" || enemyType == "Dove") {
        std::uniform_int_distribution flipPickupType(0, 1);
        pickupType = flipPickupType(rng);
        switch (pickupType)
        {
            case 0:
                animation = "PowerUp1";
                break;
            case 1:
                animation = "PowerUp2";
                break;
        }
    } else {
        pickupType = 2;
        animation = "PowerUp3";
    }
    

    auto pickup = m_entityManager.addEntity("pickup");

    pickup->addComponent<CTransform>(pos, sf::Vector2f(0.f, 0.f));
    pickup->addComponent<CCollision>(20);
    pickup->addComponent<CPickup>(pickupType);
    pickup->addComponent<CAnimation>(m_game->assets().getAnimation(animation));
  
}


sf::Vector2f Scene_Easy::findClosestEnemy(sf::Vector2f mPos) {
    float closest = std::numeric_limits<float>::max();
    sf::Vector2f posClosest{0.f, 0.f};
    for (auto e: m_entityManager.getEntities("enemy")) {
        if (e->getComponent<CTransform>().has) {
            auto ePos = e->getComponent<CTransform>().pos;
            float distToEnemy = dist(mPos, ePos);
            if (distToEnemy < closest) {
                closest = distToEnemy;
                posClosest = ePos;
            }
        }
    }
    return posClosest;
}


void Scene_Easy::sGuideMissiles(sf::Time dt) {

    const float approachRate = 800.f;
    for (auto m: m_entityManager.getEntities("missile")) {

        if (m->getComponent<CTransform>().has) {
            auto &tfm = m->getComponent<CTransform>();
            auto ePos = findClosestEnemy(tfm.pos);

            auto targetDir = normalize(ePos - tfm.pos);
            tfm.vel = m_missileSpeed * normalize(approachRate * dt.asSeconds() * targetDir + tfm.vel);
            tfm.rot = bearing(tfm.vel) + 90;
        }
    }
}


void Scene_Easy::spawnEnemies(std::string type, float offset, size_t numPlanes) {

    auto bounds = getViewBounds();
    float spacer = bounds.width / (1.0 + numPlanes);
    sf::Vector2f pos(spacer, offset);
    for (int i{0}; i < numPlanes; ++i) {
        if ((bounds.top + m_worldView.getSize().y / 2.f) > 0) {
            spawnEnemy(type, pos);
            pos.x += spacer;
        }
    }

}


void Scene_Easy::spawnEnemy(std::string type, sf::Vector2f pos) {

    auto vel = sf::Vector2f(0.f, m_enemySpeed);
    float rotation = 180.f;

    auto enemyPlane = m_entityManager.addEntity("enemy");
    enemyPlane->addComponent<CTransform>(pos, vel, rotation);
    enemyPlane->addComponent<CAnimation>(m_game->assets().getAnimation(type));
    enemyPlane->addComponent<CCollision>(20);
    enemyPlane->addComponent<CHealth>(100);

    auto& ap = enemyPlane->addComponent<CAutoPilot>();
    ap.bearings = m_enemyConfig[type].dirs;
    ap.lengths = m_enemyConfig[type].times;

    if (type == "Avenger")
        enemyPlane->addComponent<CGun>();
}


void Scene_Easy::spawnEnemies() {
    std::uniform_int_distribution<int> numberOfPlanes(3, 4);
    std::uniform_int_distribution<int> typeOfPlane(0, 1);
    std::exponential_distribution<float> nextSpawnPoint(1.f / 250.f);

    auto bounds = getViewBounds();
    static float next_spawn_point{bounds.top - 100};

    if (next_spawn_point > bounds.top - 100) {
        int num = numberOfPlanes(rng);
        spawnEnemies((typeOfPlane(rng) == 0) ? "Avenger" : "Raptor", next_spawn_point, num);
        next_spawn_point = bounds.top - 200.f - nextSpawnPoint(rng);
    }

}

void Scene_Easy::sAutoPilot(const sf::Time &dt) {// autopilot enemties
    for (auto e: m_entityManager.getEntities("enemy")) {
        if (e->hasComponent<CAutoPilot>()) {
            auto &ai = e->getComponent<CAutoPilot>();
            ai.countdown -= dt;
            if (ai.countdown < sf::Time::Zero)
            {
                ai.countdown = ai.lengths[ai.currentLeg];
                ai.currentLeg = (ai.currentLeg + 1) % ai.legs;

                auto &tfm = e->getComponent<CTransform>();
                tfm.vel = length(tfm.vel) *  uVecFromBearing(90 + ai.bearings[ai.currentLeg]);
            }

        }
    }
}

void Scene_Easy::sRemoveEntitiesOutOfGame() {
    auto battleField = getViewBounds();
    battleField.top -= 300.f;
    battleField.height += 300.f;
    float buffer = 100.f;

    for (auto e : m_entityManager.getEntities()) {
        if (e->hasComponent<CTransform>()) {
            auto pos = e->getComponent<CTransform>().pos;
            if (pos.x < (battleField.left - buffer) ||
                pos.x > (battleField.left + battleField.width + buffer) ||
                pos.y < (battleField.top - buffer) ||
                pos.y > (battleField.top + battleField.height + buffer)) {
                e->destroy();
            }
        }
    }
}

