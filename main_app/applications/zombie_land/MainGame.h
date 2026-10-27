#pragma once

#include <array>
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

    /*Small soft particle used for blood spray and ground drops*/
    std::shared_ptr<Texture> m_textureBloodParticle;

    /*HUD heart frames and the smoothed fill state of each heart*/
    static constexpr std::size_t g_unSoldierHeartCount = 3;
    std::shared_ptr<Texture> m_textureHeartFull;
    std::shared_ptr<Texture> m_textureHeartEmpty;
    std::array<float, g_unSoldierHeartCount> m_arrHeartFill = {1.0f, 1.0f, 1.0f};

    struct BloodParticle
    {
        float fWorldX = 0.0f;
        float fWorldY = 0.0f;
        float fVelocityX = 0.0f;
        float fVelocityY = 0.0f;
        float fGroundY = 0.0f;
        float fLifeSeconds = 0.0f;
        float fMaxLifeSeconds = 0.0f;
        float fSize = 1.0f;
        bool bLeavesSplat = false;
    };

    struct BloodSplat
    {
        float fWorldX = 0.0f;
        float fGroundY = 0.0f;
        float fSize = 1.0f;
        float fLifeSeconds = 0.0f;
    };

    std::vector<BloodParticle> m_vecBloodParticles;
    std::vector<BloodSplat> m_vecBloodSplats;

    Player m_soldier;
    Enemy m_zombieWoman{"zombie_woman", "Zombie Woman"};
    /*The crawling wild zombie keeps its head much lower than the standing types*/
    Enemy m_wildZombie{"wild_zombie", "Wild Zombie", 96, 96, 38.0f};
    Enemy m_zombieMan{"zombie_man", "Zombie Man"};

    /*The imported zombie_1..4 sprite sheets use 128x128 frames*/
    Enemy m_zombie1{"zombie_1", "Zombie 1", 128, 128};
    Enemy m_zombie2{"zombie_2", "Zombie 2", 128, 128};
    Enemy m_zombie3{"zombie_3", "Zombie 3", 128, 128};
    Enemy m_zombie4{"zombie_4", "Zombie 4", 128, 128};

    /*Extra instances of the existing types let a wave contain up to ten zombies*/
    Enemy m_zombieWoman2{"zombie_woman", "Zombie Woman 2"};
    Enemy m_zombieMan2{"zombie_man", "Zombie Man 2"};
    Enemy m_zombie1Extra{"zombie_1", "Zombie 1 Extra", 128, 128};

    /*All enemy instances are loaded, but only a random subset is active per wave*/
    std::vector<Enemy *> m_vecEnemies;
    std::vector<Enemy *> m_vecActiveEnemies;

    /*Horizontal world scroll, used for the parallax backgrounds*/
    float m_fWorldOffsetX = 0.0f;

    /*Wave progression: the next group spawns after the soldier advances*/
    bool m_bWaitingForNextWave = false;
    float m_fWaveClearSoldierX = 0.0f;
    unsigned int m_unWaveNumber = 1;

    /*A death after the first two hearts brings the soldier back; the last ends the run*/
    bool m_bRespawnPending = false;
    float m_fRespawnTimer = 0.0f;
    float m_fRespawnInvulnerabilitySeconds = 0.0f;
    bool m_bGameOver = false;

    void UpdateCamera();
    void RandomizeEnemies(bool bSpawnAheadOfPlayer);
    void UpdateWaveState();
    bool AreAllActiveEnemiesDead() const;
    bool AreAllCorpsesFadedOut() const;
    Enemy *GetNearestEnemy() const;
    Enemy *GetEnemyInShotLine() const;
    void DamageEnemyInShotLine();
    void DamageEnemiesInExplosion(float fWorldX, float fFeetY);
    void SpawnBloodAt(float fWorldX, float fHeadWorldY, float fGroundY);
    void UpdateBloodEffects(float fDeltaTimeSeconds);
    void DrawGroundBlood();
    void DrawAirborneBlood();
    void ClearBloodEffects();
    void UpdateSoldierRespawn(float fDeltaTimeSeconds);
    float GetHeartTargetFill(std::size_t unHeartIndex) const;
    void UpdateHealthHearts(float fDeltaTimeSeconds);
    void DrawHealthHearts();
    void ResetHealthHearts();
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
