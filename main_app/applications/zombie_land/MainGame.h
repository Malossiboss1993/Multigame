#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <main_app/applications/IApp.h>
#include <main_app/texture/Texture.h>
#include <main_app/timer/TimerMgr.h>

#include "Enemy.h"
#include "Player.h"

class ZombieLand : public IApp, public ITimer
{

private:
    std::string m_strAppName = "Zombie Land";

    /*Textures background layers, drawn from back to front*/
    std::vector<std::shared_ptr<Texture>> m_vecTexturesBackground;

    Player m_soldier;
    Enemy m_zombieWoman{"zombie_woman", "Zombie Woman"};
    Enemy m_wildZombie{"wild_zombie", "Wild Zombie"};
    Enemy m_zombieMan{"zombie_man", "Zombie Man"};

    /*The imported zombie_1..4 sprite sheets use 128x128 frames*/
    Enemy m_zombie1{"zombie_1", "Zombie 1", 128, 128};
    Enemy m_zombie2{"zombie_2", "Zombie 2", 128, 128};
    Enemy m_zombie3{"zombie_3", "Zombie 3", 128, 128};
    Enemy m_zombie4{"zombie_4", "Zombie 4", 128, 128};

    /*All enemy types are loaded, but only a random subset is active per run*/
    std::vector<Enemy *> m_vecEnemies;
    std::vector<Enemy *> m_vecActiveEnemies;

    /*Horizontal world scroll, used for the parallax backgrounds*/
    float m_fWorldOffsetX = 0.0f;

    void UpdateCamera();
    void RandomizeEnemies();
    Enemy *GetNearestEnemy() const;
    void DrawBackgroundLayer(std::size_t unLayerIndex, float fScaleFactor);

public:
    ZombieLand();

    bool Init() override;
    bool Deinit() override;
    bool HandleEvent() override;
    const std::string &GetAppName() override;
    void OnEnter() override;
    void OnExit() override;
    void OnDraw() override;
    void OnTick(unsigned int unID, unsigned int unTimes) final;
};
