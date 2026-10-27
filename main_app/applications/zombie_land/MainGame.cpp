#include "MainGame.h"

#include "Parallax.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <random>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <main_app/panel/Panel.h>
#include <debug/Logger.h>

/*The camera starts scrolling forward once the soldier reaches the middle of the screen*/
constexpr float g_fForwardCameraAnchorScreenRatio = 0.5f;

/*When walking back, the soldier moves across the screen first and the camera
  starts following once he reaches the left edge of the playable area*/
constexpr float g_fBackwardCameraAnchorScreenRatio = 1.0f / 25.0f;

/*Parallax factors for layers 1 to 5: far layers move slower, near layers move faster*/
constexpr float g_arrBackgroundParallaxFactors[] =
{
    0.2f,
    0.4f,
    0.6f,
    0.8f,
    1.0f
};
constexpr std::size_t g_unBackgroundParallaxFactorsCount =
    sizeof(g_arrBackgroundParallaxFactors) / sizeof(g_arrBackgroundParallaxFactors[0]);

/*Draw the characters behind the foreground cactus layer*/
constexpr std::size_t g_unCharacterBackgroundLayerIndex = 4;

/*Ground decorations repeat across this world span so the desert never runs out*/
constexpr float g_fEnvironmentRepeatWidth = 6000.0f;

/*Each wave gets a random number of active enemies within this range*/
constexpr std::size_t g_unMinActiveEnemies = 3;
constexpr std::size_t g_unMaxActiveEnemies = 10;

/*Combat tuning*/
constexpr int g_nShotDamage = 30;
constexpr int g_nGrenadeDamage = 999;
/*Each zombie attack removes one quarter of a heart*/
constexpr float g_fZombieAttackDamage = 0.25f;
constexpr float g_fShotRange = 1500.0f;
constexpr float g_fGrenadeDamageRadiusX = 260.0f;
constexpr float g_fGrenadeDamageRadiusY = 150.0f;
constexpr float g_fZombieAttackHitRangeX = 150.0f;
constexpr float g_fZombieAttackHitRangeY = 45.0f;

/*Shot blood effects*/
constexpr float g_fBloodGravity = 1400.0f;
constexpr float g_fBloodSplatLifeSeconds = 4.5f;
constexpr float g_fBloodSplatFadeSeconds = 1.2f;

/*HUD hearts*/
constexpr float g_fHeartStartX = 34.0f;
constexpr float g_fHeartStartY = 30.0f;
constexpr float g_fHeartSpacing = 10.0f;
constexpr float g_fHeartDrainSpeed = 2.0f;

/*Pause after a heart-empty death, then a short protected window after respawn*/
constexpr float g_fRespawnDelaySeconds = 1.2f;
constexpr float g_fRespawnInvulnerabilitySeconds = 1.5f;

/*How far the soldier must advance after clearing a wave before the next one starts*/
constexpr float g_fNextWaveAdvanceDistance = 400.0f;

ZombieLand::ZombieLand()
    : m_vecEnemies{&m_zombieWoman, &m_wildZombie, &m_zombieMan,
                   &m_zombie1, &m_zombie2, &m_zombie3, &m_zombie4,
                   &m_zombieWoman2, &m_zombieMan2, &m_zombie1Extra}
{
}

bool ZombieLand::Init()
{
    /*Load background layers in drawing order: 1 (back) to 5 (front)*/
    const std::vector<std::string> vecBackgroundPaths =
    {
        "../src/resources/zombie_land/background_1/1.png",
        "../src/resources/zombie_land/background_1/2.png",
        "../src/resources/zombie_land/background_1/3.png",
        "../src/resources/zombie_land/background_1/4.png",
        "../src/resources/zombie_land/background_1/5.png"
    };

    m_vecTexturesBackground.reserve(vecBackgroundPaths.size());

    for (const auto &strPath : vecBackgroundPaths)
    {
        auto textureBackground = Texture::CreateTexture(strPath);

        if (!textureBackground || !textureBackground->Load())
        {
            LOG_ERROR("Zombie Land - Unable to load background layer \"{0}\" !", strPath);
            return false;
        }

        /*The background layers are tiled horizontally for the parallax scroll*/
        textureBackground->SetWrapModeS(GL_REPEAT);

        m_vecTexturesBackground.emplace_back(textureBackground);
    }

    m_textureBloodParticle =
        Texture::CreateTexture("../src/resources/zombie_land/blood_particle.png");

    if (!m_textureBloodParticle || !m_textureBloodParticle->Load())
    {
        LOG_ERROR("Zombie Land - Unable to load blood particle !");
        return false;
    }

    m_textureHeartFull =
        Texture::CreateTexture("../src/resources/zombie_land/heart_full.png");
    m_textureHeartEmpty =
        Texture::CreateTexture("../src/resources/zombie_land/heart_empty.png");

    if (!m_textureHeartFull || !m_textureHeartFull->Load() ||
        !m_textureHeartEmpty || !m_textureHeartEmpty->Load())
    {
        LOG_ERROR("Zombie Land - Unable to load heart textures !");
        return false;
    }

    auto loadEnvironmentTexture = [this](EEnvironmentAsset eAsset,
                                         const std::string &strPath) -> bool
    {
        std::shared_ptr<Texture> &texture =
            m_arrEnvironmentTextures[static_cast<std::size_t>(eAsset)];
        texture = Texture::CreateTexture(strPath);
        return texture && texture->Load();
    };

    if (!loadEnvironmentTexture(EEnvironmentAsset::eDeadTree1,
                                "../src/resources/zombie_land/environment/dead_tree_1.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eDeadTree2,
                                "../src/resources/zombie_land/environment/dead_tree_2.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eBush1,
                                "../src/resources/zombie_land/environment/bush_1.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eBush2,
                                "../src/resources/zombie_land/environment/bush_2.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eGrass1,
                                "../src/resources/zombie_land/environment/grass_1.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eGrass2,
                                "../src/resources/zombie_land/environment/grass_2.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eGrass3,
                                "../src/resources/zombie_land/environment/grass_3.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eSkeletonRibcage,
                                "../src/resources/zombie_land/environment/skeleton_ribcage.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eSkeletonSkull,
                                "../src/resources/zombie_land/environment/skeleton_skull.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eSkeletonSpine,
                                "../src/resources/zombie_land/environment/skeleton_spine.png") ||
        !loadEnvironmentTexture(EEnvironmentAsset::eSkeletonHand,
                                "../src/resources/zombie_land/environment/skeleton_hand.png"))
    {
        LOG_ERROR("Zombie Land - Unable to load environment decoration textures !");
        return false;
    }

    CreateEnvironmentDecorations();

    if (!m_soldier.Init())
    {
        LOG_ERROR("Zombie Land - Unable to initialize player !");
        return false;
    }

    for (Enemy *pEnemy : m_vecEnemies)
    {
        if (!pEnemy->Init())
        {
            LOG_ERROR("Zombie Land - Unable to initialize enemy !");
            return false;
        }

        pEnemy->SetTarget(&m_soldier);
    }

    LOG_INFO("Zombie Land - Initialized ...");
    return true;
}

bool ZombieLand::Deinit()
{
    ClearBloodEffects();

    if (m_textureBloodParticle)
    {
        m_textureBloodParticle->DeleteTexture();
        m_textureBloodParticle.reset();
    }

    if (m_textureHeartFull)
    {
        m_textureHeartFull->DeleteTexture();
        m_textureHeartFull.reset();
    }

    if (m_textureHeartEmpty)
    {
        m_textureHeartEmpty->DeleteTexture();
        m_textureHeartEmpty.reset();
    }

    for (std::shared_ptr<Texture> &texture : m_arrEnvironmentTextures)
    {
        if (texture)
        {
            texture->DeleteTexture();
            texture.reset();
        }
    }

    m_vecGroundDecorations.clear();
    m_vecForegroundDecorations.clear();

    m_soldier.Deinit();

    for (Enemy *pEnemy : m_vecEnemies)
    {
        pEnemy->Deinit();
    }

    LOG_WARN("Zombie Land - Deinitialized ...");
    return true;
}

bool ZombieLand::HandleEvent()
{
    const float fDeltaTimeSeconds = MainApp::GetInstance().GetDeltaTime() / 1000.0f;

    m_soldier.Update(fDeltaTimeSeconds);

    /*Keep the soldier turned toward the closest target while firing, even when
      it is behind him or at a different ground depth*/
    if (m_soldier.IsShooting())
    {
        Enemy *pAimTarget = GetNearestShootableEnemy();

        if (pAimTarget)
        {
            m_soldier.FaceTowardsWorldX(pAimTarget->GetFeetWorldX());
        }
    }

    /*Every queued shot hits the nearest living enemy in range*/
    const unsigned int unShotHits = m_soldier.ConsumeShotHits();

    for (unsigned int i = 0; i < unShotHits; ++i)
    {
        DamageNearestEnemy();
    }

    /*Keep the original camera rule: only adjust the view while walking left/right*/
    if (m_soldier.HasHorizontalInput())
    {
        UpdateCamera();
    }

    float fExplosionWorldX = 0.0f;
    float fExplosionFeetY = 0.0f;

    if (m_soldier.ConsumeExplosionImpact(fExplosionWorldX, fExplosionFeetY))
    {
        DamageEnemiesInExplosion(fExplosionWorldX, fExplosionFeetY);
    }

    if (!m_soldier.IsDead())
    {
        for (Enemy *pEnemy : m_vecActiveEnemies)
        {
            pEnemy->Update(fDeltaTimeSeconds);

            if (!pEnemy->ConsumeAttackHit())
            {
                continue;
            }

            const float fDeltaX =
                std::fabs(pEnemy->GetFeetWorldX() - m_soldier.GetFeetWorldX());
            const float fDeltaY =
                std::fabs(pEnemy->GetFeetY() - m_soldier.GetFeetY());

            if (fDeltaX <= g_fZombieAttackHitRangeX &&
                fDeltaY <= g_fZombieAttackHitRangeY &&
                m_fRespawnInvulnerabilitySeconds <= 0.0f)
            {
                LOG_INFO("Zombie Land - \"{0}\" hit the soldier",
                         pEnemy->GetDisplayName());
                m_soldier.TakeDamage(g_fZombieAttackDamage);
            }

            if (m_soldier.IsDead())
            {
                break;
            }
        }
    }
    else
    {
        /*Let existing corpses finish fading even after the soldier dies*/
        for (Enemy *pEnemy : m_vecActiveEnemies)
        {
            if (pEnemy->IsDead())
            {
                pEnemy->Update(fDeltaTimeSeconds);
            }
        }
    }

    UpdateBloodEffects(fDeltaTimeSeconds);
    UpdateHealthHearts(fDeltaTimeSeconds);
    UpdateSoldierRespawn(fDeltaTimeSeconds);

    if (!m_soldier.IsDead() && m_soldier.ConsumeGrenadeThrowRequest())
    {
        Enemy *pTarget = GetNearestEnemy();

        if (pTarget)
        {
            m_soldier.ThrowGrenadeAt(pTarget->GetFeetWorldX(), pTarget->GetFeetY());
            LOG_INFO("Zombie Land - Grenade target selected at world X {0}, feet Y {1}",
                     pTarget->GetFeetWorldX(), pTarget->GetFeetY());
        }
        else
        {
            LOG_INFO("Zombie Land - No enemy available for grenade target");
        }
    }

    UpdateWaveState();

    /*The panel remains active so the Home button can return to App Select*/
    return MainApp::GetInstance().ptrPanel->HandleEvent();
}

const std::string &ZombieLand::GetAppName()
{
    return m_strAppName;
}

void ZombieLand::OnEnter()
{
    /*Zombie Land only needs the Home button and the volume knob*/
    MainApp::GetInstance().ptrPanel->HideFields();

    m_soldier.Reset();
    ClearBloodEffects();
    ResetHealthHearts();

    m_unWaveNumber = 1;
    m_bWaitingForNextWave = false;
    m_fWaveClearSoldierX = 0.0f;
    m_bRespawnPending = false;
    m_fRespawnTimer = 0.0f;
    m_fRespawnInvulnerabilitySeconds = 0.0f;
    m_bGameOver = false;

    RandomizeEnemies(false);

    for (Enemy *pEnemy : m_vecActiveEnemies)
    {
        pEnemy->Reset();
    }

    LOG_INFO("Zombie Land - Wave {0} spawned", m_unWaveNumber);
    LOG_INFO("Zombie Land - Transition to Application succeed");
}

void ZombieLand::OnExit()
{
    m_soldier.Stop();
    ClearBloodEffects();

    for (Enemy *pEnemy : m_vecEnemies)
    {
        pEnemy->Stop();
    }

    MainApp::GetInstance().ptrPanel->ShowFields();
    LOG_INFO("Zombie Land - Exit from Application");
}

void ZombieLand::OnDraw()
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    const float fBackgroundScaleFactor =
        m_vecTexturesBackground.empty() ?
        1.0f :
        static_cast<float>(rend->SCREEN_WIDTH) /
            static_cast<float>(m_vecTexturesBackground.front()->GetWidth());

    const std::size_t unCharacterLayerIndex =
        m_vecTexturesBackground.size() > g_unCharacterBackgroundLayerIndex ?
        g_unCharacterBackgroundLayerIndex :
        m_vecTexturesBackground.size();

    /*Draw background layers from back to front, inserting the characters
      behind the foreground cacti*/
    for (std::size_t unLayerIndex = 0; unLayerIndex < m_vecTexturesBackground.size(); ++unLayerIndex)
    {
        if (unLayerIndex == unCharacterLayerIndex)
        {
            DrawEnvironmentDecorations(false);
            DrawGroundBlood();

            for (Enemy *pEnemy : m_vecActiveEnemies)
            {
                pEnemy->Draw(m_fWorldOffsetX);
            }

            m_soldier.Draw(m_fWorldOffsetX);
        }

        DrawBackgroundLayer(unLayerIndex, fBackgroundScaleFactor);
    }

    if (unCharacterLayerIndex >= m_vecTexturesBackground.size())
    {
        DrawEnvironmentDecorations(false);
        DrawGroundBlood();

        for (Enemy *pEnemy : m_vecActiveEnemies)
        {
            pEnemy->Draw(m_fWorldOffsetX);
        }

        m_soldier.Draw(m_fWorldOffsetX);
    }

    DrawEnvironmentDecorations(true);

    /*Draw the grenade projectile and explosion above the foreground layers so
      the effect stays visible when the target stands behind a cactus*/
    m_soldier.DrawGrenadeEffects(m_fWorldOffsetX);

    /*Blood spray sits above the characters so it stays visible on a head hit*/
    DrawAirborneBlood();

    /*Draw Panel, including the Home button used to exit*/
    MainApp::GetInstance().ptrPanel->OnDraw();

    /*HUD hearts are drawn last so the panel cannot cover them*/
    DrawHealthHearts();
}

void ZombieLand::UpdateCamera()
{
    const auto &rend = MainApp::GetInstance().ptrRend;
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);
    const float fForwardCameraAnchorX =
        fScreenWidth * g_fForwardCameraAnchorScreenRatio;
    const float fBackwardCameraAnchorX =
        fScreenWidth * g_fBackwardCameraAnchorScreenRatio;
    const float fSoldierScreenX = m_soldier.GetFeetWorldX() - m_fWorldOffsetX;

    if (fSoldierScreenX > fForwardCameraAnchorX)
    {
        m_fWorldOffsetX += fSoldierScreenX - fForwardCameraAnchorX;
    }
    else if (fSoldierScreenX < fBackwardCameraAnchorX)
    {
        m_fWorldOffsetX += fSoldierScreenX - fBackwardCameraAnchorX;
    }

    m_fWorldOffsetX = std::max(0.0f, m_fWorldOffsetX);

    const float fHalfSoldierWidth = m_soldier.GetHalfWidth();
    const float fMaxFeetX = fScreenWidth - fHalfSoldierWidth;
    const float fMinFeetX = std::min(fBackwardCameraAnchorX, fMaxFeetX);
    const float fClampedScreenX = std::clamp(
        m_soldier.GetFeetWorldX() - m_fWorldOffsetX, fMinFeetX, fMaxFeetX);

    m_soldier.SetFeetWorldX(fClampedScreenX + m_fWorldOffsetX);
}

Enemy *ZombieLand::GetNearestEnemy() const
{
    Enemy *pNearestEnemy = nullptr;
    float fNearestDistanceSq = std::numeric_limits<float>::max();

    for (Enemy *pEnemy : m_vecActiveEnemies)
    {
        if (pEnemy->IsDead())
        {
            continue;
        }

        const float fDeltaX = pEnemy->GetFeetWorldX() - m_soldier.GetFeetWorldX();
        const float fDeltaY = pEnemy->GetFeetY() - m_soldier.GetFeetY();
        const float fDistanceSq = fDeltaX * fDeltaX + fDeltaY * fDeltaY;

        if (fDistanceSq < fNearestDistanceSq)
        {
            fNearestDistanceSq = fDistanceSq;
            pNearestEnemy = pEnemy;
        }
    }

    return pNearestEnemy;
}

Enemy *ZombieLand::GetNearestShootableEnemy() const
{
    Enemy *pNearestEnemy = nullptr;
    float fNearestDistanceSq = std::numeric_limits<float>::max();
    const float fShotRangeSq = g_fShotRange * g_fShotRange;

    for (Enemy *pEnemy : m_vecActiveEnemies)
    {
        if (pEnemy->IsDead())
        {
            continue;
        }

        const float fDeltaX = pEnemy->GetFeetWorldX() - m_soldier.GetFeetWorldX();
        const float fDeltaY = pEnemy->GetFeetY() - m_soldier.GetFeetY();
        const float fDistanceSq = fDeltaX * fDeltaX + fDeltaY * fDeltaY;

        if (fDistanceSq > fShotRangeSq)
        {
            continue;
        }

        if (fDistanceSq < fNearestDistanceSq)
        {
            fNearestDistanceSq = fDistanceSq;
            pNearestEnemy = pEnemy;
        }
    }

    return pNearestEnemy;
}

void ZombieLand::DamageNearestEnemy()
{
    Enemy *pTarget = GetNearestShootableEnemy();

    if (!pTarget)
    {
        return;
    }

    m_soldier.FaceTowardsWorldX(pTarget->GetFeetWorldX());

    const float fHitWorldX = pTarget->GetFeetWorldX();
    const float fHeadWorldY = pTarget->GetHeadWorldY();
    const float fGroundY = pTarget->GetFeetY();

    LOG_INFO("Zombie Land - Shot hit \"{0}\" for {1} damage",
             pTarget->GetDisplayName(), g_nShotDamage);
    pTarget->TakeDamage(g_nShotDamage, false, m_fWorldOffsetX);

    SpawnBloodAt(fHitWorldX, fHeadWorldY, fGroundY);
}

void ZombieLand::DamageEnemiesInExplosion(float fWorldX, float fFeetY)
{
    unsigned int unHitCount = 0;

    for (Enemy *pEnemy : m_vecActiveEnemies)
    {
        if (pEnemy->IsDead())
        {
            continue;
        }

        const float fDeltaX = pEnemy->GetFeetWorldX() - fWorldX;
        const float fDeltaY = pEnemy->GetFeetY() - fFeetY;

        if (std::fabs(fDeltaX) > g_fGrenadeDamageRadiusX ||
            std::fabs(fDeltaY) > g_fGrenadeDamageRadiusY)
        {
            continue;
        }

        LOG_INFO("Zombie Land - Grenade hit \"{0}\" for {1} damage",
                 pEnemy->GetDisplayName(), g_nGrenadeDamage);
        pEnemy->TakeDamage(g_nGrenadeDamage, true, m_fWorldOffsetX);
        ++unHitCount;
    }

    LOG_INFO("Zombie Land - Grenade damaged {0} enemies", unHitCount);
}

void ZombieLand::SpawnBloodAt(float fWorldX, float fHeadWorldY, float fGroundY)
{
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> jitter(-6.0f, 6.0f);
    std::uniform_real_distribution<float> horizontalVelocity(-115.0f, 115.0f);
    std::uniform_real_distribution<float> upwardVelocity(-320.0f, -90.0f);
    std::uniform_real_distribution<float> sizeDistribution(0.14f, 0.27f);
    std::uniform_real_distribution<float> lifeDistribution(0.34f, 0.62f);

    /*Blood starts at the target's current screen position, then scrolls at the
      ground depth of the hit once the camera moves*/
    const float fAnchoredWorldX =
        fWorldX +
        m_fWorldOffsetX * (GetGroundParallaxFactor(fGroundY) - 1.0f);

    /*A short, dense impact splash keeps the hit readable on the head*/
    for (int i = 0; i < 2; ++i)
    {
        BloodParticle particle;
        particle.fWorldX = fAnchoredWorldX + jitter(rng) * 0.5f;
        particle.fWorldY = fHeadWorldY + jitter(rng) * 0.5f;
        particle.fVelocityX = horizontalVelocity(rng) * 0.2f;
        particle.fVelocityY = upwardVelocity(rng) * 0.15f;
        particle.fGroundY = fGroundY;
        particle.fLifeSeconds = 0.12f + static_cast<float>(i) * 0.04f;
        particle.fMaxLifeSeconds = particle.fLifeSeconds;
        particle.fSize = 0.34f - static_cast<float>(i) * 0.05f;
        m_vecBloodParticles.push_back(particle);
    }

    /*Small red spray around the head*/
    for (int i = 0; i < 13; ++i)
    {
        BloodParticle particle;
        particle.fWorldX = fAnchoredWorldX + jitter(rng);
        particle.fWorldY = fHeadWorldY + jitter(rng);
        particle.fVelocityX = horizontalVelocity(rng);
        particle.fVelocityY = upwardVelocity(rng);
        particle.fGroundY = fGroundY;
        particle.fLifeSeconds = lifeDistribution(rng);
        particle.fMaxLifeSeconds = particle.fLifeSeconds;
        particle.fSize = sizeDistribution(rng);
        m_vecBloodParticles.push_back(particle);
    }

    /*A few drops fall to the ground and leave small stains*/
    std::uniform_real_distribution<float> dropHorizontalVelocity(-80.0f, 80.0f);
    std::uniform_real_distribution<float> dropVerticalVelocity(-20.0f, 100.0f);
    std::uniform_real_distribution<float> dropSizeDistribution(0.14f, 0.22f);

    for (int i = 0; i < 5; ++i)
    {
        BloodParticle particle;
        particle.fWorldX = fAnchoredWorldX + jitter(rng);
        particle.fWorldY = fHeadWorldY + jitter(rng) * 0.5f;
        particle.fVelocityX = dropHorizontalVelocity(rng);
        particle.fVelocityY = dropVerticalVelocity(rng);
        particle.fGroundY = fGroundY;
        particle.fLifeSeconds = 1.8f;
        particle.fMaxLifeSeconds = 1.8f;
        particle.fSize = dropSizeDistribution(rng);
        particle.bLeavesSplat = true;
        m_vecBloodParticles.push_back(particle);
    }
}

void ZombieLand::UpdateBloodEffects(float fDeltaTimeSeconds)
{
    for (auto &particle : m_vecBloodParticles)
    {
        particle.fLifeSeconds -= fDeltaTimeSeconds;
        particle.fVelocityY += g_fBloodGravity * fDeltaTimeSeconds;
        particle.fWorldX += particle.fVelocityX * fDeltaTimeSeconds;
        particle.fWorldY += particle.fVelocityY * fDeltaTimeSeconds;

        if (particle.bLeavesSplat && particle.fWorldY >= particle.fGroundY)
        {
            BloodSplat splat;
            splat.fWorldX = particle.fWorldX;
            splat.fGroundY = particle.fGroundY;
            splat.fSize = std::max(0.18f, particle.fSize * 1.3f);
            splat.fLifeSeconds = g_fBloodSplatLifeSeconds;
            m_vecBloodSplats.push_back(splat);
            particle.fLifeSeconds = 0.0f;
        }
    }

    m_vecBloodParticles.erase(
        std::remove_if(m_vecBloodParticles.begin(), m_vecBloodParticles.end(),
                       [](const BloodParticle &particle)
                       {
                           return particle.fLifeSeconds <= 0.0f;
                       }),
        m_vecBloodParticles.end());

    for (auto &splat : m_vecBloodSplats)
    {
        splat.fLifeSeconds -= fDeltaTimeSeconds;
    }

    m_vecBloodSplats.erase(
        std::remove_if(m_vecBloodSplats.begin(), m_vecBloodSplats.end(),
                       [](const BloodSplat &splat)
                       {
                           return splat.fLifeSeconds <= 0.0f;
                       }),
        m_vecBloodSplats.end());
}

void ZombieLand::DrawGroundBlood()
{
    if (!m_textureBloodParticle || m_vecBloodSplats.empty())
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    rend->UseShaderPicture();

    const float fTextureWidth =
        static_cast<float>(m_textureBloodParticle->GetWidth());
    const float fTextureHeight =
        static_cast<float>(m_textureBloodParticle->GetHeight());
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);

    for (const auto &splat : m_vecBloodSplats)
    {
        const float fScreenX =
            splat.fWorldX -
            m_fWorldOffsetX * GetGroundParallaxFactor(splat.fGroundY);
        const float fScale = splat.fSize;
        const float fWidth = fTextureWidth * fScale;
        const float fHeight = fTextureHeight * fScale;

        if (fScreenX + fWidth < 0.0f || fScreenX - fWidth > fScreenWidth)
        {
            continue;
        }

        const float fAlpha =
            std::clamp(splat.fLifeSeconds / g_fBloodSplatFadeSeconds,
                       0.0f, 1.0f);

        rend->SetColor(0.62f, 0.012f, 0.012f, 0.9f * fAlpha);
        rend->DrawPictureScaled(m_textureBloodParticle,
                                fScreenX - fWidth * 0.5f,
                                splat.fGroundY - fHeight * 0.5f,
                                fScale);

        /*A couple of tiny satellite drops make the stain look splattered*/
        const float fSideScale = fScale * 0.45f;
        const float fSideWidth = fTextureWidth * fSideScale;
        const float fSideHeight = fTextureHeight * fSideScale;

        rend->SetColor(0.62f, 0.012f, 0.012f, 0.68f * fAlpha);
        rend->DrawPictureScaled(m_textureBloodParticle,
                                fScreenX + fWidth * 0.55f - fSideWidth * 0.5f,
                                splat.fGroundY - fSideHeight * 0.5f,
                                fSideScale);
        rend->DrawPictureScaled(m_textureBloodParticle,
                                fScreenX - fWidth * 0.65f - fSideWidth * 0.5f,
                                splat.fGroundY - fHeight * 0.15f,
                                fSideScale);
    }

    rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void ZombieLand::DrawAirborneBlood()
{
    if (!m_textureBloodParticle || m_vecBloodParticles.empty())
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    rend->UseShaderPicture();

    const float fTextureWidth =
        static_cast<float>(m_textureBloodParticle->GetWidth());
    const float fTextureHeight =
        static_cast<float>(m_textureBloodParticle->GetHeight());
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);

    for (const auto &particle : m_vecBloodParticles)
    {
        const float fScreenX =
            particle.fWorldX -
            m_fWorldOffsetX * GetGroundParallaxFactor(particle.fGroundY);
        const float fScale = particle.fSize;
        const float fWidth = fTextureWidth * fScale;
        const float fHeight = fTextureHeight * fScale;

        if (fScreenX + fWidth < 0.0f || fScreenX - fWidth > fScreenWidth)
        {
            continue;
        }

        const float fAlpha =
            std::clamp(particle.fLifeSeconds / particle.fMaxLifeSeconds,
                       0.0f, 1.0f);

        rend->SetColor(0.82f, 0.015f, 0.015f, fAlpha);
        rend->DrawPictureScaled(m_textureBloodParticle,
                                fScreenX - fWidth * 0.5f,
                                particle.fWorldY - fHeight * 0.5f,
                                fScale);
    }

    rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void ZombieLand::ClearBloodEffects()
{
    m_vecBloodParticles.clear();
    m_vecBloodSplats.clear();
}

void ZombieLand::CreateEnvironmentDecorations()
{
    m_vecGroundDecorations.clear();
    m_vecForegroundDecorations.clear();

    /*A fixed seed keeps the desert set dressing stable between runs*/
    std::mt19937 rng(0x5A17C0DEu);
    auto randomBetween = [&rng](float fMin, float fMax)
    {
        return std::uniform_real_distribution<float>(fMin, fMax)(rng);
    };
    auto wrapWorldX = [](float fWorldX)
    {
        fWorldX = std::fmod(fWorldX, g_fEnvironmentRepeatWidth);
        return fWorldX < 0.0f ? fWorldX + g_fEnvironmentRepeatWidth : fWorldX;
    };
    auto addDecoration = [this](std::vector<EnvironmentDecoration> &vecDecorations,
                                EEnvironmentAsset eAsset,
                                float fWorldX, float fGroundY, float fScale)
    {
        vecDecorations.push_back(
            {m_arrEnvironmentTextures[static_cast<std::size_t>(eAsset)],
             fWorldX, fGroundY, fScale});
    };

    /*A few twisted trees frame the route*/
    const std::array<float, 6> arrTreeXs =
    {
        300.0f, 1390.0f, 2530.0f, 3710.0f, 4790.0f, 5660.0f
    };
    for (std::size_t i = 0; i < arrTreeXs.size(); ++i)
    {
        addDecoration(m_vecGroundDecorations,
                      (i % 2 == 0) ?
                          EEnvironmentAsset::eDeadTree1 :
                          EEnvironmentAsset::eDeadTree2,
                      wrapWorldX(arrTreeXs[i] + randomBetween(-80.0f, 80.0f)),
                      randomBetween(905.0f, 985.0f),
                      randomBetween(2.9f, 3.35f));
    }

    /*Bones are rarer landmarks, with the hand clawing out of the dirt*/
    const std::array<float, 7> arrSkeletonXs =
    {
        710.0f, 1180.0f, 2010.0f, 3040.0f, 3980.0f, 4860.0f, 5620.0f
    };
    const std::array<EEnvironmentAsset, 4> arrSkeletonAssets =
    {
        EEnvironmentAsset::eSkeletonRibcage,
        EEnvironmentAsset::eSkeletonSkull,
        EEnvironmentAsset::eSkeletonSpine,
        EEnvironmentAsset::eSkeletonHand
    };
    for (std::size_t i = 0; i < arrSkeletonXs.size(); ++i)
    {
        addDecoration(m_vecGroundDecorations,
                      arrSkeletonAssets[i % arrSkeletonAssets.size()],
                      wrapWorldX(arrSkeletonXs[i] + randomBetween(-90.0f, 90.0f)),
                      randomBetween(970.0f, 1035.0f),
                      randomBetween(2.7f, 3.3f));
    }

    /*Scrub bushes break up the empty ground*/
    for (int i = 0; i < 14; ++i)
    {
        addDecoration(m_vecGroundDecorations,
                      (i % 2 == 0) ?
                          EEnvironmentAsset::eBush1 :
                          EEnvironmentAsset::eBush2,
                      randomBetween(0.0f, g_fEnvironmentRepeatWidth),
                      randomBetween(940.0f, 1025.0f),
                      randomBetween(2.8f, 3.5f));
    }

    /*Small tufts fill in the open sand*/
    const std::array<EEnvironmentAsset, 3> arrGrassAssets =
    {
        EEnvironmentAsset::eGrass1,
        EEnvironmentAsset::eGrass2,
        EEnvironmentAsset::eGrass3
    };
    for (int i = 0; i < 34; ++i)
    {
        addDecoration(m_vecGroundDecorations,
                      arrGrassAssets[i % arrGrassAssets.size()],
                      randomBetween(0.0f, g_fEnvironmentRepeatWidth),
                      randomBetween(950.0f, 1055.0f),
                      randomBetween(2.5f, 3.6f));
    }

    /*Taller foreground grass crosses in front of the soldier's feet*/
    for (int i = 0; i < 12; ++i)
    {
        addDecoration(m_vecForegroundDecorations,
                      arrGrassAssets[(i + 1) % arrGrassAssets.size()],
                      randomBetween(0.0f, g_fEnvironmentRepeatWidth),
                      randomBetween(1035.0f, 1072.0f),
                      randomBetween(3.0f, 4.0f));
    }

    const auto sortByDepth = [](const EnvironmentDecoration &left,
                                const EnvironmentDecoration &right)
    {
        return left.fGroundY < right.fGroundY;
    };
    std::sort(m_vecGroundDecorations.begin(), m_vecGroundDecorations.end(),
              sortByDepth);
    std::sort(m_vecForegroundDecorations.begin(),
              m_vecForegroundDecorations.end(), sortByDepth);
}

void ZombieLand::DrawEnvironmentDecorations(bool bForeground)
{
    const std::vector<EnvironmentDecoration> &vecDecorations =
        bForeground ? m_vecForegroundDecorations : m_vecGroundDecorations;

    if (vecDecorations.empty())
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);

    for (const EnvironmentDecoration &decoration : vecDecorations)
    {
        if (!decoration.texture)
        {
            continue;
        }

        const float fWidth =
            static_cast<float>(decoration.texture->GetWidth()) * decoration.fScale;
        const float fHeight =
            static_cast<float>(decoration.texture->GetHeight()) * decoration.fScale;
        const float fScrollFactor =
            GetGroundParallaxFactor(decoration.fGroundY);
        float fScroll = std::fmod(
            m_fWorldOffsetX * fScrollFactor, g_fEnvironmentRepeatWidth);

        if (fScroll < 0.0f)
        {
            fScroll += g_fEnvironmentRepeatWidth;
        }

        float fScreenX = decoration.fWorldX - fScroll;

        /*Wrap decorations from the end of the pattern around to the other side*/
        if (fScreenX <= -fWidth)
        {
            fScreenX += g_fEnvironmentRepeatWidth;
        }
        else if (fScreenX >= fScreenWidth)
        {
            fScreenX -= g_fEnvironmentRepeatWidth;
        }

        if (fScreenX + fWidth < 0.0f || fScreenX > fScreenWidth)
        {
            continue;
        }

        rend->DrawPictureScaled(decoration.texture,
                                fScreenX,
                                decoration.fGroundY - fHeight,
                                decoration.fScale);
    }
}

void ZombieLand::UpdateSoldierRespawn(float fDeltaTimeSeconds)
{
    if (!m_soldier.IsDead())
    {
        m_fRespawnInvulnerabilitySeconds =
            std::max(0.0f, m_fRespawnInvulnerabilitySeconds - fDeltaTimeSeconds);
        return;
    }

    /*The final heart ends the run instead of bringing the soldier back*/
    if (m_soldier.GetHealth() <= 0.0f)
    {
        if (!m_bGameOver)
        {
            m_bGameOver = true;
            LOG_INFO("Zombie Land - All hearts are empty, game over");
        }

        return;
    }

    if (!m_bRespawnPending)
    {
        m_bRespawnPending = true;
        m_fRespawnTimer = g_fRespawnDelaySeconds;
        LOG_INFO("Zombie Land - Heart emptied, waiting to respawn the soldier");
        return;
    }

    m_fRespawnTimer -= fDeltaTimeSeconds;

    if (m_fRespawnTimer > 0.0f)
    {
        return;
    }

    m_soldier.Respawn();
    m_bRespawnPending = false;
    m_fRespawnInvulnerabilitySeconds = g_fRespawnInvulnerabilitySeconds;

    LOG_INFO("Zombie Land - Soldier respawned, health {0}/{1}",
             m_soldier.GetHealth(), m_soldier.GetMaxHealth());
}

float ZombieLand::GetHeartTargetFill(std::size_t unHeartIndex) const
{
    const float fMaxHealth = static_cast<float>(g_unSoldierHeartCount);
    const float fHealth =
        std::clamp(m_soldier.GetHealth(), 0.0f, fMaxHealth);
    /*The rightmost heart is the active one, matching the existing HUD order*/
    return std::clamp(
        fHealth - static_cast<float>(unHeartIndex), 0.0f, 1.0f);
}

void ZombieLand::UpdateHealthHearts(float fDeltaTimeSeconds)
{
    const float fMaxChange = g_fHeartDrainSpeed * fDeltaTimeSeconds;

    for (std::size_t i = 0; i < g_unSoldierHeartCount; ++i)
    {
        const float fTargetFill = GetHeartTargetFill(i);
        float &fFill = m_arrHeartFill[i];

        if (fFill < fTargetFill)
        {
            fFill = std::min(fTargetFill, fFill + fMaxChange);
        }
        else
        {
            fFill = std::max(fTargetFill, fFill - fMaxChange);
        }
    }
}

void ZombieLand::DrawHealthHearts()
{
    if (!m_textureHeartFull || !m_textureHeartEmpty)
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    rend->UseShaderPicture();

    const float fHeartWidth =
        static_cast<float>(m_textureHeartFull->GetWidth());
    const float fHeartHeight =
        static_cast<float>(m_textureHeartFull->GetHeight());
    const unsigned int unHeartWidth =
        static_cast<unsigned int>(m_textureHeartFull->GetWidth());
    const unsigned int unHeartHeight =
        static_cast<unsigned int>(m_textureHeartFull->GetHeight());
    const float fHeartStep = fHeartWidth + g_fHeartSpacing;

    for (std::size_t i = 0; i < g_unSoldierHeartCount; ++i)
    {
        const float fX = g_fHeartStartX + fHeartStep * static_cast<float>(i);

        /*The empty frame shows the whole outline; the red frame is clipped
          vertically so the heart drains from the top down*/
        rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
        rend->DrawPictureScaled(m_textureHeartEmpty, fX, g_fHeartStartY, 1.0f);

        if (m_arrHeartFill[i] > 0.001f)
        {
            const int nFillHeight = static_cast<int>(
                std::lround(fHeartHeight * m_arrHeartFill[i]));
            const unsigned int unFillHeight = static_cast<unsigned int>(
                std::clamp(nFillHeight, 1, static_cast<int>(fHeartHeight)));
            const unsigned int unSourceY = unHeartHeight - unFillHeight;
            const float fFillY = g_fHeartStartY + fHeartHeight -
                                 static_cast<float>(unFillHeight);

            rend->DrawPicturePart(m_textureHeartFull, fX, fFillY,
                                  0, unSourceY, unHeartWidth, unFillHeight, 1.0f);
        }
    }

    rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void ZombieLand::ResetHealthHearts()
{
    for (std::size_t i = 0; i < g_unSoldierHeartCount; ++i)
    {
        m_arrHeartFill[i] = GetHeartTargetFill(i);
    }
}

void ZombieLand::RandomizeEnemies(bool bSpawnAheadOfPlayer)
{
    m_vecActiveEnemies.clear();

    if (m_vecEnemies.empty())
    {
        return;
    }

    static std::mt19937 rng(std::random_device{}());

    /*Choose a random wave size, clamped to the loaded enemy instances*/
    const std::size_t unMinActiveEnemyCount =
        std::min(g_unMinActiveEnemies, m_vecEnemies.size());
    const std::size_t unMaxActiveEnemyCount =
        std::min(g_unMaxActiveEnemies, m_vecEnemies.size());
    std::uniform_int_distribution<std::size_t> activeEnemyCountDistribution(
        unMinActiveEnemyCount, unMaxActiveEnemyCount);
    const std::size_t unActiveEnemyCount =
        activeEnemyCountDistribution(rng);

    LOG_INFO("Zombie Land - Wave {0} active enemy count: {1}",
             m_unWaveNumber, unActiveEnemyCount);

    /*Pick the enemy instances that will appear in this wave*/
    std::vector<std::size_t> vecEnemyOrder(m_vecEnemies.size());
    std::iota(vecEnemyOrder.begin(), vecEnemyOrder.end(), 0);
    std::shuffle(vecEnemyOrder.begin(), vecEnemyOrder.end(), rng);
    m_vecActiveEnemies.reserve(unActiveEnemyCount);

    for (std::size_t i = 0; i < unActiveEnemyCount; ++i)
    {
        m_vecActiveEnemies.push_back(m_vecEnemies[vecEnemyOrder[i]]);
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);
    const float fMargin = 180.0f;
    const float fPlayerScreenX = m_soldier.GetFeetWorldX() - m_fWorldOffsetX;
    const float fInitialMaxScreenX = std::max(fMargin, fScreenWidth - fMargin);
    float fMinScreenX = fMargin;
    float fMaxScreenX = fInitialMaxScreenX;

    if (bSpawnAheadOfPlayer)
    {
        /*New waves start fully past the right edge so they are not visible at
          the spawn point. The margin also keeps the widest sprite off-screen*/
        constexpr float fOffscreenSpawnMargin = 260.0f;
        /*Keep roughly even spacing for larger waves*/
        const float fOffscreenSpawnDepth =
            std::max(520.0f,
                     static_cast<float>(unActiveEnemyCount) * 90.0f);

        fMinScreenX = fScreenWidth + fOffscreenSpawnMargin;
        fMaxScreenX = fMinScreenX + fOffscreenSpawnDepth;
    }
    else
    {
        /*Prefer spawning away from the soldier; fall back to the full screen if needed*/
        const float fDesiredMinScreenX = fPlayerScreenX + 250.0f;

        if (fDesiredMinScreenX < fMaxScreenX)
        {
            fMinScreenX = std::max(fMargin, fDesiredMinScreenX);
        }
    }

    const float fBandWidth =
        (fMaxScreenX - fMinScreenX) / static_cast<float>(unActiveEnemyCount);

    std::uniform_real_distribution<float> xDistribution(0.0f, 1.0f);
    std::uniform_real_distribution<float> yDistribution(850.0f, 1030.0f);
    const float fMinSpawnDelay = bSpawnAheadOfPlayer ? 0.75f : 1.5f;
    const float fMaxSpawnDelay = bSpawnAheadOfPlayer ? 2.0f : 3.5f;
    std::uniform_real_distribution<float> delayDistribution(fMinSpawnDelay, fMaxSpawnDelay);
    std::uniform_real_distribution<float> speedDistribution(50.0f, 450.0f);

    for (std::size_t i = 0; i < unActiveEnemyCount; ++i)
    {
        Enemy *pEnemy = m_vecActiveEnemies[i];
        const float fBandStart =
            fMinScreenX + fBandWidth * static_cast<float>(i);
        const float fScreenX = fBandStart + fBandWidth * xDistribution(rng);
        const float fWorldX = fScreenX + m_fWorldOffsetX;
        const float fFeetY = yDistribution(rng);
        const float fSpawnDelay = delayDistribution(rng);
        const float fRunSpeed = speedDistribution(rng);

        pEnemy->SetSpawnPosition(fWorldX, fFeetY);
        pEnemy->SetSpawnDelay(fSpawnDelay);
        pEnemy->SetRunSpeed(fRunSpeed);

        LOG_INFO("Zombie Land - Enemy {0} \"{1}\" spawn: screen X {2}, feet Y {3}, speed {4}, delay {5}",
                 i, pEnemy->GetDisplayName(), fScreenX, fFeetY, fRunSpeed, fSpawnDelay);
    }
}

bool ZombieLand::AreAllActiveEnemiesDead() const
{
    for (const Enemy *pEnemy : m_vecActiveEnemies)
    {
        if (!pEnemy->IsDead())
        {
            return false;
        }
    }

    return !m_vecActiveEnemies.empty();
}

bool ZombieLand::AreAllCorpsesFadedOut() const
{
    for (const Enemy *pEnemy : m_vecActiveEnemies)
    {
        if (!pEnemy->IsCorpseFadedOut())
        {
            return false;
        }
    }

    return !m_vecActiveEnemies.empty();
}

void ZombieLand::UpdateWaveState()
{
    if (m_soldier.IsDead() || m_vecActiveEnemies.empty())
    {
        return;
    }

    if (!m_bWaitingForNextWave)
    {
        if (!AreAllActiveEnemiesDead())
        {
            return;
        }

        m_bWaitingForNextWave = true;
        m_fWaveClearSoldierX = m_soldier.GetFeetWorldX();
        LOG_INFO("Zombie Land - Wave {0} cleared, advance forward for the next wave",
                 m_unWaveNumber);
        return;
    }

    const bool bAdvancedForward =
        m_soldier.GetFeetWorldX() >=
        m_fWaveClearSoldierX + g_fNextWaveAdvanceDistance;

    /*Wait for the old corpses to fade before reusing their enemy objects*/
    if (!bAdvancedForward || !AreAllCorpsesFadedOut())
    {
        return;
    }

    ++m_unWaveNumber;
    RandomizeEnemies(true);

    for (Enemy *pEnemy : m_vecActiveEnemies)
    {
        pEnemy->Reset();
    }

    m_bWaitingForNextWave = false;
    m_fWaveClearSoldierX = 0.0f;

    LOG_INFO("Zombie Land - Wave {0} spawned", m_unWaveNumber);
}

void ZombieLand::DrawBackgroundLayer(std::size_t unLayerIndex, float fScaleFactor)
{
    if (unLayerIndex >= m_vecTexturesBackground.size() ||
        unLayerIndex >= g_unBackgroundParallaxFactorsCount)
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    const auto &textureBackground = m_vecTexturesBackground[unLayerIndex];
    const float fLayerWidth = static_cast<float>(textureBackground->GetWidth()) * fScaleFactor;

    if (fLayerWidth <= 0.0f)
    {
        return;
    }

    const float fLayerScroll =
        m_fWorldOffsetX * g_arrBackgroundParallaxFactors[unLayerIndex];
    float fScrollOffset = std::fmod(fLayerScroll, fLayerWidth);

    if (fScrollOffset < 0.0f)
    {
        fScrollOffset += fLayerWidth;
    }

    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);
    const float fStartX = -fScrollOffset;

    /*A background texture is one screen wide, so repeat it to cover the view*/
    for (float fX = fStartX; fX < fScreenWidth; fX += fLayerWidth)
    {
        rend->DrawPictureScaled(textureBackground, fX, 0.0f, fScaleFactor);
    }
}

void ZombieLand::OnTick(unsigned int unID, unsigned int unTimes)
{
}
