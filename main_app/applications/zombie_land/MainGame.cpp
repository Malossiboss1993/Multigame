#include "MainGame.h"

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

/*Cap the number of enemies that can appear in one run*/
constexpr std::size_t g_unMaxActiveEnemies = 4;

/*Combat tuning*/
constexpr int g_nShotDamage = 30;
constexpr int g_nGrenadeDamage = 999;
constexpr int g_nZombieAttackDamage = 25;
constexpr float g_fShotRange = 1500.0f;
constexpr float g_fShotVerticalTolerance = 100.0f;
constexpr float g_fGrenadeDamageRadiusX = 260.0f;
constexpr float g_fGrenadeDamageRadiusY = 150.0f;
constexpr float g_fZombieAttackHitRangeX = 150.0f;
constexpr float g_fZombieAttackHitRangeY = 45.0f;

/*Shot blood effects*/
constexpr float g_fBloodGravity = 1400.0f;
constexpr float g_fBloodSplatLifeSeconds = 4.5f;
constexpr float g_fBloodSplatFadeSeconds = 1.2f;

/*How far the soldier must advance after clearing a wave before the next one starts*/
constexpr float g_fNextWaveAdvanceDistance = 400.0f;

ZombieLand::ZombieLand()
    : m_vecEnemies{&m_zombieWoman, &m_wildZombie, &m_zombieMan,
                   &m_zombie1, &m_zombie2, &m_zombie3, &m_zombie4}
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

    /*Every queued shot hits the closest living enemy in the soldier's line of fire*/
    const unsigned int unShotHits = m_soldier.ConsumeShotHits();

    for (unsigned int i = 0; i < unShotHits; ++i)
    {
        DamageEnemyInShotLine();
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
                fDeltaY <= g_fZombieAttackHitRangeY)
            {
                LOG_INFO("Zombie Land - \"{0}\" hit the soldier",
                         pEnemy->GetDisplayName());
                m_soldier.TakeDamage(g_nZombieAttackDamage);
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

    m_unWaveNumber = 1;
    m_bWaitingForNextWave = false;
    m_fWaveClearSoldierX = 0.0f;

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
        DrawGroundBlood();

        for (Enemy *pEnemy : m_vecActiveEnemies)
        {
            pEnemy->Draw(m_fWorldOffsetX);
        }

        m_soldier.Draw(m_fWorldOffsetX);
    }

    /*Draw the grenade projectile and explosion above the foreground layers so
      the effect stays visible when the target stands behind a cactus*/
    m_soldier.DrawGrenadeEffects(m_fWorldOffsetX);

    /*Blood spray sits above the characters so it stays visible on a head hit*/
    DrawAirborneBlood();

    /*Draw Panel, including the Home button used to exit*/
    MainApp::GetInstance().ptrPanel->OnDraw();
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

Enemy *ZombieLand::GetEnemyInShotLine() const
{
    Enemy *pNearestEnemy = nullptr;
    float fNearestDistanceSq = std::numeric_limits<float>::max();
    const bool bFacingRight = m_soldier.IsFacingRight();

    for (Enemy *pEnemy : m_vecActiveEnemies)
    {
        if (pEnemy->IsDead())
        {
            continue;
        }

        const float fDeltaX = pEnemy->GetFeetWorldX() - m_soldier.GetFeetWorldX();
        const float fDeltaY = pEnemy->GetFeetY() - m_soldier.GetFeetY();
        const float fHorizontalDistance = std::fabs(fDeltaX);
        const float fVerticalDistance = std::fabs(fDeltaY);

        if ((bFacingRight && fDeltaX < 0.0f) ||
            (!bFacingRight && fDeltaX > 0.0f))
        {
            continue;
        }

        if (fHorizontalDistance > g_fShotRange ||
            fVerticalDistance > g_fShotVerticalTolerance)
        {
            continue;
        }

        const float fDistanceSq = fDeltaX * fDeltaX + fDeltaY * fDeltaY;

        if (fDistanceSq < fNearestDistanceSq)
        {
            fNearestDistanceSq = fDistanceSq;
            pNearestEnemy = pEnemy;
        }
    }

    return pNearestEnemy;
}

void ZombieLand::DamageEnemyInShotLine()
{
    Enemy *pTarget = GetEnemyInShotLine();

    if (!pTarget)
    {
        return;
    }

    const float fHitWorldX = pTarget->GetFeetWorldX();
    const float fHeadWorldY = pTarget->GetHeadWorldY();
    const float fGroundY = pTarget->GetFeetY();

    LOG_INFO("Zombie Land - Shot hit \"{0}\" for {1} damage",
             pTarget->GetDisplayName(), g_nShotDamage);
    pTarget->TakeDamage(g_nShotDamage);

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
        pEnemy->TakeDamage(g_nGrenadeDamage, true);
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

    /*A short, dense impact splash keeps the hit readable on the head*/
    for (int i = 0; i < 2; ++i)
    {
        BloodParticle particle;
        particle.fWorldX = fWorldX + jitter(rng) * 0.5f;
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
        particle.fWorldX = fWorldX + jitter(rng);
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
        particle.fWorldX = fWorldX + jitter(rng);
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
        const float fScreenX = splat.fWorldX - m_fWorldOffsetX;
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
        const float fScreenX = particle.fWorldX - m_fWorldOffsetX;
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

void ZombieLand::RandomizeEnemies(bool bSpawnAheadOfPlayer)
{
    m_vecActiveEnemies.clear();

    if (m_vecEnemies.empty())
    {
        return;
    }

    static std::mt19937 rng(std::random_device{}());

    /*Pick a random subset so a wave never shows every enemy type at once*/
    std::vector<std::size_t> vecEnemyOrder(m_vecEnemies.size());
    std::iota(vecEnemyOrder.begin(), vecEnemyOrder.end(), 0);
    std::shuffle(vecEnemyOrder.begin(), vecEnemyOrder.end(), rng);

    const std::size_t unActiveEnemyCount =
        std::min(g_unMaxActiveEnemies, m_vecEnemies.size());
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
        constexpr float fOffscreenSpawnDepth = 520.0f;

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
    std::uniform_real_distribution<float> speedDistribution(350.0f, 650.0f);

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
