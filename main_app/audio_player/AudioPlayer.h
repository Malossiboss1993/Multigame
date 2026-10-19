#pragma once

#include <filesystem>
#include <string>

#include <irrKlang.h>

class AudioPlayer
{

private:

    /*Directory that contains the running executable. Used to resolve the
      relative paths of the sound files, so that the application finds them
      no matter from which directory it was started. */
    static std::filesystem::path ExecutableDirectory();

    /*Pointer to the irrKlang sound engine. The engine is created in Init() and
      dropped in Deinit(). */
    irrklang::ISoundEngine* m_engine = nullptr;

public:

    bool Init();
    bool Deinit();
    bool IsPlaying(const std::string& strFile);
    void PlaySound(const std::string& strFile, bool bLoop = false);
    void StopAllSounds();
    void SetVolume(float fVolume);

};
