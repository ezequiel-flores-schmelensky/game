//
// Created by David Burchill on 2022-11-09.
//

#include "Scene_Normal.h"
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



Scene_Normal::Scene_Normal(GameEngine *gameEngine, const std::string &configPath)
        : Scene(gameEngine), m_worldView(gameEngine->window().getDefaultView()), m_worldBounds({0, 0}, {0, 0}) {

    loadFromFile(configPath);

    m_spawnPosition = sf::Vector2f(m_worldView.getSize().x / 2.f,
                                   m_worldBounds.height - m_worldView.getSize().y / 2.f);

    m_worldView.setCenter(m_spawnPosition);

    registerActions();
    spawnPlayer();

    MusicPlayer::getInstance().play("gameTheme");
    MusicPlayer::getInstance().setVolume(1);

}


void Scene_Normal::loadFromFile(const std::string &configPath) {
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
        } else if (token == "EnemySpeed") {
            config >> m_enemySpeed;
        } else if (token == "BulletSpeed") {
            config >> m_bulletSpeed;
        } else if (token == "MissileSpeed") {
            config >> m_missileSpeed;
        } else if (token == "FireInterval") {
            float interval;
            config >> interval;
            m_fireInterval = sf::seconds(interval);
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


void Scene_Normal::init(const std::string &configPath) {

}


void Scene_Normal::keepEntitiesInBounds() {

}


void Scene_Normal::onEnd() {

}


void Scene_Normal::sMovement(sf::Time dt) {
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


void Scene_Normal::playerMovement() {

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
    pv = normalize(pv);
    m_player->getComponent<CTransform>().vel = m_playerSpeed * pv;

}


void Scene_Normal::sCollisions() {
    checkMissileCollision();
    checkBulletCollision();
    checkPlaneCollision();
    checkPickupCollision();
}


void Scene_Normal::checkIfDead(NttPtr e) {

    std::uniform_int_distribution flip(0, 1);

    // check for planes that need to be destroyed
    if (e->hasComponent<CHealth>()) {
        if (e->getComponent<CHealth>().hp <= 0) {
            e->addComponent<CAnimation>(m_game->assets().getAnimation("explosion"));
            e->getComponent<CTransform>().vel = sf::Vector2f(0.f, 0.f);
            e->addComponent<CState>().state = "dead";
            e->removeComponent<CCollision>();

            if (flip(rng) == 0)
                SoundPlayer::getInstance().play("Explosion1", e->getComponent<CTransform>().pos);
            else
                SoundPlayer::getInstance().play("Explosion2", e->getComponent<CTransform>().pos);

        }
    }
}

void Scene_Normal::checkPickupCollision() {// check for plane collision

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
                   
                    SoundPlayer::getInstance().play("CollectPickup", e->getComponent<CTransform>().pos);
                    e->destroy();
                }
            }
        }
    }
}

void Scene_Normal::checkPlaneCollision() {// check for plane collision

    if (m_player->hasComponent<CCollision>()) {
        auto pPos = m_player->getComponent<CTransform>().pos;
        auto pCr = m_player->getComponent<CCollision>().radius;

        for (auto e: m_entityManager.getEntities("enemy")) {
            if (e->hasComponent<CTransform>() && e->hasComponent<CCollision>()) {
                auto ePos = e->getComponent<CTransform>().pos;
                auto eCr = e->getComponent<CCollision>().radius;

                // planes have collided
                if (dist(ePos, pPos) < (eCr + pCr)) {
                    auto &pHP = m_player->getComponent<CHealth>().hp;
                    auto &eHP = e->getComponent<CHealth>().hp;

                    // however many HP the plane has left,
                    // that's how much damage it inflicts on other plane
                    int tmpHP = pHP;
                    pHP -= eHP;
                    eHP -= tmpHP;

                    checkIfDead(e);
                    int hPickup = hasPickup(rng);
                    if (e->getComponent<CState>().state == "dead" && hPickup == 1) {
                        droppingAPickup(e->getComponent<CTransform>().pos);
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


void Scene_Normal::checkBulletCollision() {
    // Player Bullets
    for (auto bullet: m_entityManager.getEntities("playerBullet")) {
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
                        int hPickup = hasPickup(rng);
                        if (e->getComponent<CState>().state == "dead" && hPickup == 1) {
                            droppingAPickup(e->getComponent<CTransform>().pos);
                        }
                    }
                }
            }
        }
    }


    // Enemy Bullets
    if (m_player->hasComponent<CCollision>()) {
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
    }
}


void Scene_Normal::checkMissileCollision() {// missiles
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

                        int hPickup = hasPickup(rng);
                        if (e->getComponent<CState>().state == "dead" && hPickup == 1) {
                            droppingAPickup(e->getComponent<CTransform>().pos);
                        }
                        
                    }
                }
            }
        }
    }
}


void Scene_Normal::sUpdate(sf::Time dt) {

}


void Scene_Normal::registerActions() {

    //TODO register action FIRE to fire the gun

    registerAction(sf::Keyboard::P, "PAUSE");
    registerAction(sf::Keyboard::Escape, "BACK");
    registerAction(sf::Keyboard::Q, "QUIT");

    registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE");
    registerAction(sf::Keyboard::C, "TOGGLE_COLLISION");
    registerAction(sf::Keyboard::G, "TOGGLE_GRID");

    registerAction(sf::Keyboard::A, "LEFT");
    registerAction(sf::Keyboard::Left, "LEFT");
    registerAction(sf::Keyboard::D, "RIGHT");
    registerAction(sf::Keyboard::Right, "RIGHT");
    registerAction(sf::Keyboard::W, "UP");
    registerAction(sf::Keyboard::Up, "UP");
    registerAction(sf::Keyboard::S, "DOWN");
    registerAction(sf::Keyboard::Down, "DOWN");

    registerAction(sf::Keyboard::Space, "FIRE");
    registerAction(sf::Keyboard::M, "LAUNCH");
}


void Scene_Normal::spawnPlayer() {
    m_player = m_entityManager.addEntity("player");
    m_player->addComponent<CTransform>(
            m_spawnPosition,
            sf::Vector2f(0.f, 0.f),
            0, 0);

    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("EagleStr"));
    m_player->addComponent<CState>("straight");
    m_player->addComponent<CCollision>(20);
    m_player->addComponent<CInput>();
    m_player->addComponent<CMissiles>();
    m_player->addComponent<CHealth>().hp = 100;
    auto &gun = m_player->addComponent<CGun>();

}


void Scene_Normal::drawAABB() {

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


void Scene_Normal::adjustPlayer() {
    auto vb = getViewBounds();

    auto &pos = m_player->getComponent<CTransform>().pos;
    auto cr = m_player->getComponent<CCollision>().radius;

    pos.x = std::max(pos.x, vb.left + cr);
    pos.x = std::min(pos.x, vb.left + vb.width - cr);
    pos.y = std::max(pos.y, vb.top + cr);
    pos.y = std::min(pos.y, vb.top + vb.height - cr);
}


void Scene_Normal::checkPlayerState() {// set the player state
    if (m_player->hasComponent<CState>()) {

        auto xVel = m_player->getComponent<CTransform>().vel.x;
        std::string newState = "straight";
        if (xVel < -0.2f) newState = "left";
        if (xVel > 0.2f) newState = "right";

        auto &state = m_player->getComponent<CState>().state;
        if (state != "dead") {
            if (newState != state) { // only if the state has changed, change the animation
                state = newState;
                if (state == "straight")
                    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("EagleStr"));
                if (state == "left")
                    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("EagleLft"));
                if (state == "right")
                    m_player->addComponent<CAnimation>(m_game->assets().getAnimation("EagleRgt"));
            }
        }
    }
}


sf::FloatRect Scene_Normal::getViewBounds() {
    sf::FloatRect bounds;
    bounds.left = m_worldView.getCenter().x - m_worldView.getSize().x / 2.f;
    bounds.top = m_worldView.getCenter().y - m_worldView.getSize().y / 2.f;
    bounds.width = m_worldView.getSize().x;
    bounds.height = m_worldView.getSize().y;
    return bounds;
}


void Scene_Normal::update(sf::Time dt) {
    if (m_isPaused)
        return;

    m_entityManager.update();
    m_worldView.move(0.f, m_scrollSpeed * dt.asSeconds() * -1);
    auto y = m_worldView.getCenter().y;

    adjustPlayer();
    checkPlayerState();
    sMovement(dt);
    sCollisions();
    sGunUpdate(dt);
    sAnimation(dt);
    sGuideMissiles(dt);
    sAutoPilot(dt);
    spawnEnemies();

    sRemoveEntitiesOutOfGame();
}


void Scene_Normal::sDoAction(const Action &action) {

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
        else if (action.name() == "FIRE") { fireBullet(); }
        else if (action.name() == "LAUNCH") { fireMissile(); }

    }

        // on Key Release
    else if (action.type() == "END") {
        if (action.name() == "LEFT") { m_player->getComponent<CInput>().left = false; }
        else if (action.name() == "RIGHT") { m_player->getComponent<CInput>().right = false; }
        else if (action.name() == "UP") { m_player->getComponent<CInput>().up = false; }
        else if (action.name() == "DOWN") { m_player->getComponent<CInput>().down = false; }
    }

}


void Scene_Normal::sRender() {

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
        MusicPlayer::getInstance().setVolume(0);
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
    } else {
        MusicPlayer::getInstance().setVolume(1);;
    }

    // draw bounding boxes
    if (m_drawAABB) {
        drawAABB();
    }


}


void Scene_Normal::sAnimation(sf::Time dt) {

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


void Scene_Normal::fireBullet() {
    m_player->getComponent<CGun>().isFiring = true;
}


void Scene_Normal::createBullet(sf::Vector2f pos, bool isEnemy) {
    float speed = (isEnemy) ? m_bulletSpeed : -m_bulletSpeed;
    std::string sfx = (isEnemy) ? "EnemyGunfire" : "AlliedGunfire";

    auto bullet = m_entityManager.addEntity(isEnemy ? "enemyBullet" : "playerBullet");
    bullet->addComponent<CTransform>(pos, sf::Vector2f(0.f, speed));
    bullet->addComponent<CAnimation>(m_game->assets().getAnimation("Bullet"));
    bullet->addComponent<CCollision>(3);

    //SoundPlayer::getInstance().play("AlliedGunfire", pos);
    SoundPlayer::getInstance().play(sfx, pos);
}


void Scene_Normal::sGunUpdate(sf::Time dt) {
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
                        createBullet(pos + sf::Vector2f(0.f, isEnemy ? 35.f : -35.f), isEnemy);
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


void Scene_Normal::fireMissile() {

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

            SoundPlayer::getInstance().play("LaunchMissile", pos);
        }
    }
}

void Scene_Normal::droppingAPickup(sf::Vector2f pos) {

    std::uniform_int_distribution flipPickupType(0, 3);
    int pickupType = flipPickupType(rng);
    std::string animation{ "" };
    switch (pickupType)
    {
        case 0:
            animation = "HealthRefill";
            break;
        case 1:
            animation = "MissileRefill";
            break;
        case 2:
            animation = "FireRate";
            break;
        case 3:
            animation = "FireSpread";
            break;
    }

    auto pickup = m_entityManager.addEntity("pickup");

    pickup->addComponent<CTransform>(pos, sf::Vector2f(0.f, 0.f));
    pickup->addComponent<CCollision>(20);
    pickup->addComponent<CPickup>(pickupType);
    pickup->addComponent<CAnimation>(m_game->assets().getAnimation(animation));
  
}


sf::Vector2f Scene_Normal::findClosestEnemy(sf::Vector2f mPos) {
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


void Scene_Normal::sGuideMissiles(sf::Time dt) {

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


void Scene_Normal::spawnEnemies(std::string type, float offset, size_t numPlanes) {

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


void Scene_Normal::spawnEnemy(std::string type, sf::Vector2f pos) {

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


void Scene_Normal::spawnEnemies() {
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


void Scene_Normal::sAutoPilot(const sf::Time &dt) {// autopilot enemties
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

void Scene_Normal::sRemoveEntitiesOutOfGame() {
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
