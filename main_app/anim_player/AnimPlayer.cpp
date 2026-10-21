#include "AnimPlayer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <debug/Logger.h>

constexpr unsigned int g_unTimerProcessAnimPlayer = 1;

bool AnimPlayer::Init(std::string strPathToAnim, float fFps)
{
    /*Check if container already full*/
    if (!m_vecImages.empty() || m_textureSpriteSheet)
    {
        LOG_ERROR("Anim Player - \"{0}\" already initialized !", strPathToAnim);

        return false;
    }

    /*Create filesystem (C++17) and extension search*/
    namespace fs = std::filesystem;
    using iterator = fs::recursive_directory_iterator;
    const std::string strExtention = ".png";

    /*Search in the directory for files, insert in set container due to alphabetical order*/
    std::set<std::string> containerFilesPath;
    if (fs::is_directory(strPathToAnim))
    {
        for (const auto &file : fs::recursive_directory_iterator(strPathToAnim))
        {
            if (file.is_regular_file() && file.path().extension() == strExtention)
            {
                containerFilesPath.insert(file.path().string());
            }
        }
    }

    /*Load all images from current path folder*/
    for (const auto &files : containerFilesPath)
    {
        std::shared_ptr<Texture> textureToInsert = Texture::CreateTexture(files);

        if (!textureToInsert->Load())
        {
            LOG_ERROR("Anim Player - Unable to load texture \"{0}\"", textureToInsert->GetPath());

            return false;
        }

        m_vecImages.emplace_back(textureToInsert);
    }

    m_fFps = fFps;
    m_unCurrentFrameIndex = 0;
    m_strPath = strPathToAnim;
    m_eState = EAnimPlayerStates::eInactive;
    m_ePlayMode = EAnimPlayerPlayMode::ePlayLooped;

    LOG_INFO("AnimPlayer - Initialized player with path: \"{0}\"", m_strPath);

    return true;
}

bool AnimPlayer::InitSurfaces(std::string strPathToAnim, float fFps)
{
    /*Check if container already full*/
    if (!m_vecImages.empty() || m_textureSpriteSheet)
    {
        LOG_ERROR("Anim Player - \"{0}\" already initialized surfaces!", strPathToAnim);

        return false;
    }

    /*Create filesystem (C++17) and extension search*/
    namespace fs = std::filesystem;
    using iterator = fs::recursive_directory_iterator;
    const std::string strExtention = ".png";

    /*Search in the directory for files, insert in set container due to alphabetical order*/
    std::set<std::string> containerFilesPath;
    if (fs::is_directory(strPathToAnim))
    {
        for (const auto &file : fs::recursive_directory_iterator(strPathToAnim))
        {
            if (file.is_regular_file() && file.path().extension() == strExtention)
            {
                containerFilesPath.insert(file.path().string());
            }
        }
    }

    /*Load all images from current path folder*/
    for (const auto &files : containerFilesPath)
    {
        std::shared_ptr<Texture> textureToInsert = Texture::CreateTexture(files);

        if (!textureToInsert->LoadSurface())
        {
            LOG_ERROR("Anim Player - Unable to create surfce \"{0}\"", textureToInsert->GetPath());

            return false;
        }

        m_vecImages.emplace_back(textureToInsert);
    }

    m_fFps = fFps;
    m_unCurrentFrameIndex = 0;
    m_strPath = strPathToAnim;
    m_eState = EAnimPlayerStates::eInactive;
    m_ePlayMode = EAnimPlayerPlayMode::ePlayLooped;

    LOG_INFO("AnimPlayer - Initialized surfaces player with path: \"{0}\"", m_strPath);

    return true;
}

bool AnimPlayer::InitSpriteSheet(std::string strPathToAnim, unsigned int unFrameWidth,
                                 unsigned int unFrameHeight, float fFps)
{
    /*Check if container already full*/
    if (!m_vecImages.empty() || m_textureSpriteSheet)
    {
        LOG_ERROR("Anim Player - \"{0}\" already initialized !", strPathToAnim);
        return false;
    }

    if (unFrameWidth == 0 || unFrameHeight == 0)
    {
        LOG_ERROR("Anim Player - Invalid sprite sheet frame size for \"{0}\" !", strPathToAnim);
        return false;
    }

    if (fFps <= 0.0f)
    {
        LOG_ERROR("Anim Player - Invalid FPS for sprite sheet \"{0}\" !", strPathToAnim);
        return false;
    }

    std::shared_ptr<Texture> textureSpriteSheet = Texture::CreateTexture(strPathToAnim);

    if (!textureSpriteSheet || !textureSpriteSheet->Load())
    {
        LOG_ERROR("Anim Player - Unable to load sprite sheet \"{0}\" !", strPathToAnim);
        return false;
    }

    const unsigned int unTextureWidth = static_cast<unsigned int>(textureSpriteSheet->GetWidth());
    const unsigned int unTextureHeight = static_cast<unsigned int>(textureSpriteSheet->GetHeight());

    if (unTextureWidth % unFrameWidth != 0 || unTextureHeight % unFrameHeight != 0)
    {
        LOG_ERROR("Anim Player - Sprite sheet \"{0}\" is not divisible by frame size {1}x{2} !",
                  strPathToAnim, unFrameWidth, unFrameHeight);
        return false;
    }

    m_unFramesPerRow = unTextureWidth / unFrameWidth;
    const unsigned int unRows = unTextureHeight / unFrameHeight;
    m_unTotalFrames = m_unFramesPerRow * unRows;

    if (m_unFramesPerRow == 0 || m_unTotalFrames == 0)
    {
        LOG_ERROR("Anim Player - Sprite sheet \"{0}\" contains no frames !", strPathToAnim);
        return false;
    }

    m_textureSpriteSheet = textureSpriteSheet;
    m_bSpriteSheet = true;
    m_unFrameWidth = unFrameWidth;
    m_unFrameHeight = unFrameHeight;
    m_fFps = fFps;
    m_unCurrentFrameIndex = 0;
    m_strPath = strPathToAnim;
    m_eState = EAnimPlayerStates::eInactive;
    m_ePlayMode = EAnimPlayerPlayMode::ePlayLooped;

    LOG_INFO("AnimPlayer - Initialized sprite sheet player with path: \"{0}\"", m_strPath);

    return true;
}

bool AnimPlayer::LoadTexturesFromSurfaces()
{
    for(auto& image : m_vecImages)
    {
        if(!image->LoadTextureFromSurface())
        {
            return false;
        }
    }

    return true;
}

bool AnimPlayer::Deinit()
{
    if(m_vecImages.empty() && !m_textureSpriteSheet)
    {
        return true;
    }

    Stop();

    for (auto &texture : m_vecImages)
    {
        texture->DeleteTexture();
    }

    m_vecImages.clear();

    if (m_textureSpriteSheet)
    {
        m_textureSpriteSheet->DeleteTexture();
        m_textureSpriteSheet.reset();
    }

    m_bSpriteSheet = false;
    m_unFrameWidth = 0;
    m_unFrameHeight = 0;
    m_unFramesPerRow = 0;
    m_unTotalFrames = 0;

    LOG_WARN("AnimPlayer - Deinitialized player with path: \"{0}\"", m_strPath);
    m_strPath = "N/A";

    return true;
}

void AnimPlayer::Start(float fFps, EAnimPlayerPlayMode ePlayMode)
{
    if (GetTotalFrames() == 0)
    {
        LOG_ERROR("Anim Player - AnimPlayer has to be initialized first!");

        return;
    }

    if (m_eState == EAnimPlayerStates::ePlaying)
    {
        return;
    }

    m_eState = EAnimPlayerStates::ePlaying;
    m_ePlayMode = ePlayMode;
    m_unCurrentFrameIndex = 0;
    const unsigned int unPeriodToStartTimerProcess = 1000 / fFps;
    MainApp::GetInstance().ptrTimer->StartTimer(this, g_unTimerProcessAnimPlayer, unPeriodToStartTimerProcess);
}

void AnimPlayer::Stop()
{
    m_eState = EAnimPlayerStates::eInactive;
    m_unCurrentFrameIndex = 0;
    MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerProcessAnimPlayer);
}

void AnimPlayer::Draw(float fX, float fY, float fScaleFactor, bool bFlipHorizontal)
{
    if (m_eState == EAnimPlayerStates::eInactive)
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;

    if (m_bSpriteSheet)
    {
        if (!m_textureSpriteSheet || m_unCurrentFrameIndex >= m_unTotalFrames)
        {
            return;
        }

        const unsigned int unFrameX =
            (m_unCurrentFrameIndex % m_unFramesPerRow) * m_unFrameWidth;
        const unsigned int unFrameY =
            (m_unCurrentFrameIndex / m_unFramesPerRow) * m_unFrameHeight;

        rend->DrawPicturePart(m_textureSpriteSheet, fX, fY,
                              unFrameX, unFrameY,
                              m_unFrameWidth, m_unFrameHeight,
                              fScaleFactor, bFlipHorizontal);
        return;
    }

    if (m_unCurrentFrameIndex >= m_vecImages.size())
    {
        return;
    }

    if (fScaleFactor == 1.0f)
    {
        rend->DrawPicture(m_vecImages[m_unCurrentFrameIndex], fX, fY);
    }
    else
    {
        rend->DrawPictureScaled(m_vecImages[m_unCurrentFrameIndex], fX, fY, fScaleFactor);
    }
}

void AnimPlayer::OnTick(unsigned int unID, unsigned int unTimes)
{
    if (unID == g_unTimerProcessAnimPlayer)
    {
        const unsigned int unTotalFrames = GetTotalFrames();

        if (unTotalFrames == 0)
        {
            return;
        }

        /*Call frame callback evey change of frame*/
        if (m_frameCallback && m_eState == EAnimPlayerStates::ePlaying)
        {
            m_frameCallback(m_unCurrentFrameIndex + 1);
        }

        if (++m_unCurrentFrameIndex >= unTotalFrames)
        {
            m_unCurrentFrameIndex = 0;

            switch (m_ePlayMode)
            {

            case EAnimPlayerPlayMode::ePlayOnce:
            {
                /*Call end callback in the end of the cycle, and stop*/
                if (m_endCallback)
                {
                    m_endCallback();
                }

                m_eState = EAnimPlayerStates::eInactive;
                MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerProcessAnimPlayer);
            }
            break;

            case EAnimPlayerPlayMode::ePlayLooped:
            {
                /*Call end callback in the end of the cycle*/
                if (m_endCallback)
                {
                    m_endCallback();
                }
            }
            break;

            default:
                break;
            }
        }
    }
}

unsigned int AnimPlayer::GetTotalFrames() const
{
    if (m_bSpriteSheet)
    {
        return m_unTotalFrames;
    }

    return static_cast<unsigned int>(m_vecImages.size());
}

void AnimPlayer::SetFrameCallback(std::function<void(unsigned int unCurrentFrame)> &frameCallback)
{
    m_frameCallback = frameCallback;
}

void AnimPlayer::SetEndCallback(std::function<void()> &endCallback)
{
    m_endCallback = endCallback;
}