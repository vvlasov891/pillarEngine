#pragma once
#include <string>
#include <cstdint>

namespace Pillar {

class AudioSystem {
public:
    static void Init();
    static void Shutdown();

    // Play a sound file (wav, mp3, ogg, flac, opus)
    // Returns a handle for Stop/Pause
    static uint32_t Play(const std::string& path, float volume = 1.0f,
                         bool loop = false, float pitch = 1.0f);
    static void     Stop(uint32_t handle);
    static void     Pause(uint32_t handle);
    static void     Resume(uint32_t handle);
    static void     SetVolume(uint32_t handle, float volume);
    static void     SetMasterVolume(float volume);
    static bool     IsPlaying(uint32_t handle);
    static void     StopAll();
};

} // namespace Pillar
