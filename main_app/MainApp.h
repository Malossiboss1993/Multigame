#pragma once
#include <memory>
#include <chrono>

namespace Debug
{
    class Logger;
}
class AppSelect;
class AudioPlayer;
class Renderer;
class TimerMgr;
class Panel;

/*MainApp class, used for control the flow of the app.*/
class MainApp
{

private:

    bool m_bShowImguiHelp = true;
    bool m_bFullscreenKeyPressed = false;

    /*Delta Time measure*/
    std::chrono::time_point<std::chrono::system_clock> m_frameBeginTime;
    std::chrono::time_point<std::chrono::system_clock> m_frameEndTime;
    float m_fDeltaTimeInMicroSeconds = 0.0f;
    float m_fDeltaTimeInMilliSeconds = 0.0f;

    MainApp();
    ~MainApp();

    void InitImgui();

public:

    static MainApp& GetInstance()
    {
        static MainApp mainApp;
        return mainApp;
    }

    /** Delete copyt-constructor, move-constructor and assignment operators */
    MainApp(MainApp const&) = delete;
    void operator = (MainApp const&) = delete;
    MainApp(MainApp &&) = delete;
    void operator = (MainApp &&) = delete;

    /*Unique pointers global objects to main classes*/
    /* The Renderer must be the first of these declarations: the OpenGL context
     * is terminated in its destructor, so every other component that uses
     * OpenGL (Panel textures, AppSelect's particle systems, ...) has to be
     * destroyed before it. */
    std::unique_ptr<Renderer> ptrRend;
    std::unique_ptr<Debug::Logger> ptrLogger;
    std::unique_ptr<AppSelect> ptrAppSelect;
    std::unique_ptr<AudioPlayer> ptrAudioPlayer;
    std::unique_ptr<TimerMgr>ptrTimer;
    std::unique_ptr<Panel>ptrPanel;

    bool Init();
    void Run();
    bool Deinit();

    const float& GetDeltaTime();
    const float& GetDeltaTimeInMicroSeconds();
    const unsigned int GeTimeStampInMicroSeconds();

};
