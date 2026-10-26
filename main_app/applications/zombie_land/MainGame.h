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
    std::vector<Enemy *> m_vecEnemies;

    /*Horizontal world scroll, used for the parallax backgrounds*/
    float m_fWorldOffsetX = 0.0f;

    void UpdateCamera();
    void RandomizeEnemies();
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
