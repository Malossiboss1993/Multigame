#pragma once

#include <string>
#include <vector>
#include <functional>

#include <main_app/texture/Texture.h>
#include <main_app/timer/TimerMgr.h>

enum class EAnimPlayerStates
{
    eInactive,
    ePlaying,
    ePaused,
    eTotalAnimPlayerStates
};

enum class EAnimPlayerPlayMode
{
    ePlayOnce,
    ePlayLooped,
    eTotalAnimPlayerPlayModes
};

class AnimPlayer : public ITimer
{

private:

    /*FPS*/
    float m_fFps = 0.0f;

    /*Current frame index of drawn animation*/
    unsigned int m_unCurrentFrameIndex = 0;

    /*Path to anim*/
    std::string m_strPath = "N/A";

    /*AnimPlayer state*/
    EAnimPlayerStates m_eState = EAnimPlayerStates::eInactive;

    /*AnimPlayer play mode*/
    EAnimPlayerPlayMode m_ePlayMode = EAnimPlayerPlayMode::ePlayLooped;

    /*Container holding all the images needed to play the animation*/
    std::vector<std::shared_ptr<Texture>>m_vecImages;

    /*Sprite sheet mode*/
    bool m_bSpriteSheet = false;
    std::shared_ptr<Texture> m_textureSpriteSheet;
    unsigned int m_unFrameWidth = 0;
    unsigned int m_unFrameHeight = 0;
    unsigned int m_unFramesPerRow = 0;
    unsigned int m_unTotalFrames = 0;

    /*Method called every change of frame*/
    std::function<void(unsigned int unCurrentFrame)>m_frameCallback;

    /*Method called in the end of the animation*/
    std::function<void()>m_endCallback;

    unsigned int GetTotalFrames() const;

public:

    bool Init(std::string strPathToAnim, float fFps);
    bool InitSurfaces(std::string strPathToAnim, float fFps);
    /*Single-image sprite sheet: frames are read left-to-right, top-to-bottom*/
    bool InitSpriteSheet(std::string strPathToAnim, unsigned int unFrameWidth,
                         unsigned int unFrameHeight, float fFps);
    bool LoadTexturesFromSurfaces();
    bool Deinit();
    void Start(float fFps = 25.0f, EAnimPlayerPlayMode ePlayMode = EAnimPlayerPlayMode::ePlayLooped);
    void Stop();
    void Draw(float fX, float fY, float fScaleFactor = 1.0f, bool bFlipHorizontal = false);
    void OnTick(unsigned int unID, unsigned int unTimes) final;
    void SetFrameCallback(std::function<void(unsigned int unCurrentFrame)>& frameCallback);
    void SetEndCallback(std::function<void()>& endCallback);
};