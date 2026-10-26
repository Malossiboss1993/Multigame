#include "MainGame.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
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

ZombieLand::ZombieLand()
    : m_vecEnemies{&m_zombieWoman, &m_wildZombie, &m_zombieMan}
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

    /*Keep the original camera rule: only adjust the view while walking left/right*/
    if (m_soldier.HasHorizontalInput())
    {
        UpdateCamera();
    }

    for (Enemy *pEnemy : m_vecEnemies)
    {
        pEnemy->Update(fDeltaTimeSeconds);
    }

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

    RandomizeEnemies();

    for (Enemy *pEnemy : m_vecEnemies)
    {
        pEnemy->Reset();
    }

    LOG_INFO("Zombie Land - Transition to Application succeed");
}

void ZombieLand::OnExit()
{
    m_soldier.Stop();

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
            for (Enemy *pEnemy : m_vecEnemies)
            {
                pEnemy->Draw(m_fWorldOffsetX);
            }

            m_soldier.Draw(m_fWorldOffsetX);
        }

        DrawBackgroundLayer(unLayerIndex, fBackgroundScaleFactor);
    }

    if (unCharacterLayerIndex >= m_vecTexturesBackground.size())
    {
        for (Enemy *pEnemy : m_vecEnemies)
        {
            pEnemy->Draw(m_fWorldOffsetX);
        }

        m_soldier.Draw(m_fWorldOffsetX);
    }

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

void ZombieLand::RandomizeEnemies()
{
    if (m_vecEnemies.empty())
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);
    const float fMargin = 180.0f;
    const float fPlayerScreenX = m_soldier.GetFeetWorldX() - m_fWorldOffsetX;
    const float fMaxScreenX = std::max(fMargin, fScreenWidth - fMargin);
    const float fDesiredMinScreenX = fPlayerScreenX + 250.0f;
    float fMinScreenX = fMargin;

    /*Prefer spawning away from the soldier; fall back to the full screen if needed*/
    if (fDesiredMinScreenX < fMaxScreenX)
    {
        fMinScreenX = std::max(fMargin, fDesiredMinScreenX);
    }

    const float fBandWidth =
        (fMaxScreenX - fMinScreenX) / static_cast<float>(m_vecEnemies.size());

    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> xDistribution(0.0f, 1.0f);
    std::uniform_real_distribution<float> yDistribution(850.0f, 1030.0f);
    std::uniform_real_distribution<float> delayDistribution(1.5f, 3.5f);
    std::uniform_real_distribution<float> speedDistribution(350.0f, 650.0f);

    /*Shuffle the bands so each enemy type gets a different random slot*/
    std::vector<std::size_t> vecBandOrder(m_vecEnemies.size());
    std::iota(vecBandOrder.begin(), vecBandOrder.end(), 0);
    std::shuffle(vecBandOrder.begin(), vecBandOrder.end(), rng);

    for (std::size_t i = 0; i < m_vecEnemies.size(); ++i)
    {
        const float fBandStart =
            fMinScreenX + fBandWidth * static_cast<float>(vecBandOrder[i]);
        const float fScreenX = fBandStart + fBandWidth * xDistribution(rng);
        const float fWorldX = fScreenX + m_fWorldOffsetX;
        const float fFeetY = yDistribution(rng);
        const float fSpawnDelay = delayDistribution(rng);
        const float fRunSpeed = speedDistribution(rng);

        m_vecEnemies[i]->SetSpawnPosition(fWorldX, fFeetY);
        m_vecEnemies[i]->SetSpawnDelay(fSpawnDelay);
        m_vecEnemies[i]->SetRunSpeed(fRunSpeed);

        LOG_INFO("Zombie Land - Enemy {0} spawn: screen X {1}, feet Y {2}, speed {3}, delay {4}",
                 i, fScreenX, fFeetY, fRunSpeed, fSpawnDelay);
    }
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
