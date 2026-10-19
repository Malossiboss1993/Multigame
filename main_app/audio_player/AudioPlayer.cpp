#include "AudioPlayer.h"

#include <debug/Logger.h>

#include <irrKlang.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include <array>

namespace {

// Returns the directory that contains the running executable.
std::filesystem::path ExecutableDirectory()
{
#if defined(_WIN32)
    char path[MAX_PATH];
    const DWORD size = sizeof(path);
    const DWORD written = GetModuleFileName(nullptr, path, size);
    return std::filesystem::path(path).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 1024;
    std::array<char, 1024> buffer{};
    if (_NSGetExecutablePath(buffer.data(), &size) != 0 || size >= buffer.size())
    {
        return {};
    }
    return std::filesystem::path(buffer.data()).parent_path();
#else
    // Linux and other Unix-like operating systems.
    const std::filesystem::path executablePath = "/proc/self/exe";
    std::error_code ec;
    if (!std::filesystem::is_regular_file(executablePath, ec))
    {
        return {};
    }
    return std::filesystem::path(std::filesystem::read_symlink(executablePath, ec))
        .parent_path();
#endif
}

// Resolves a path of a sound file against the directory of the executable,
// because all paths used by the application are relative to it.
std::filesystem::path ResolveSoundPath(const std::string& strFile)
{
    const std::filesystem::path path(strFile);
    if (path.is_absolute())
    {
        return path;
    }
    return ExecutableDirectory() / path;
}

} // end namespace

bool AudioPlayer::Init()
{
    m_engine = irrklang::createIrrKlangDevice(
        irrklang::ESOD_AUTO_DETECT,
        irrklang::ESEO_DEFAULT_OPTIONS,
        0,
        IRR_KLANG_VERSION);
    if (m_engine == nullptr)
    {
        LOG_ERROR("AudioPlayer - Unable to start the sound engine !");
        return false;
    }
    m_engine->setSoundVolume(1.0f);

    return true;
}

bool AudioPlayer::Deinit()
{
    // The sound engine is the root object of irrKlang and deletes all sounds
    // and sources when it is dropped.
    if (m_engine != nullptr)
    {
        m_engine->drop();
        m_engine = nullptr;
    }

    LOG_WARN("AudioPlayer - Deinitialized ...");
    return true;
}

bool AudioPlayer::IsPlaying(const std::string& strFile)
{
    if (m_engine == nullptr)
    {
        return false;
    }
    return m_engine->isCurrentlyPlaying(strFile.c_str());
}

void AudioPlayer::PlaySound(const std::string& strFile, bool bLoop)
{
    if (m_engine == nullptr)
    {
        LOG_ERROR("AudioPlayer - The sound engine is not initialized !");
        return;
    }

    const std::filesystem::path soundPath = ResolveSoundPath(strFile);

    // Play the sound file. The 'track' parameter makes irrKlang return an
    // interface for the sound, so that its state can be inspected later on.
    irrklang::ISound* sound = m_engine->play2D(
        soundPath.string().c_str(),
        bLoop,
        false,
        true);
    if (sound == nullptr)
    {
        LOG_ERROR("AudioPlayer - Unable to play sound file: " + strFile);
        return;
    }

    // The sound stays tracked by the engine until it has finished playing
    // (or until stopAllSounds() is called), so no further handling is needed.
}

void AudioPlayer::StopAllSounds()
{
    if (m_engine == nullptr)
    {
        LOG_ERROR("AudioPlayer - The sound engine is not initialized !");
        return;
    }

    m_engine->stopAllSounds();
}

void AudioPlayer::SetVolume(float fVolume)
{
    if (m_engine == nullptr)
    {
        LOG_ERROR("AudioPlayer - The sound engine is not initialized !");
        return;
    }

    // The volume is expected in the range of 0.0f (silent) to 1.0f (full).
    m_engine->setSoundVolume(fVolume);
}
