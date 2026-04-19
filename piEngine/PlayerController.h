#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include "AudioSystem.h"
#include "Core/Log.h"
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace Pillar {

// ── Internal state ────────────────────────────────────────────────────────────
static ma_engine                           s_Engine;
static bool                                s_Initialized = false;
static std::unordered_map<uint32_t, ma_sound*> s_Sounds;
static std::mutex                          s_Mutex;
static std::atomic<uint32_t>              s_NextHandle{1};

void AudioSystem::Init() {
    ma_engine_config cfg = ma_engine_config_init();
    ma_result r = ma_engine_init(&cfg, &s_Engine);
    if (r != MA_SUCCESS) {
        PL_ERROR("AudioSystem: failed to initialize miniaudio engine ({})", (int)r);
        return;
    }
    s_Initialized = true;
    PL_INFO("AudioSystem initialized (miniaudio).");
}

void AudioSystem::Shutdown() {
    if (!s_Initialized) return;
    StopAll();
    for (auto& [h, snd] : s_Sounds) {
        ma_sound_uninit(snd);
        delete snd;
    }
    s_Sounds.clear();
    ma_engine_uninit(&s_Engine);
    s_Initialized = false;
}

uint32_t AudioSystem::Play(const std::string& path, float volume, bool loop, float pitch) {
    if (!s_Initialized) return 0;
    auto* snd = new ma_sound();
    ma_uint32 flags = MA_SOUND_FLAG_ASYNC;
    ma_result r = ma_sound_init_from_file(&s_Engine, path.c_str(), flags, nullptr, nullptr, snd);
    if (r != MA_SUCCESS) {
        PL_WARN("AudioSystem: cannot load '{}' ({})", path, (int)r);
        delete snd;
        return 0;
    }
    ma_sound_set_volume(snd, volume);
    ma_sound_set_pitch(snd, pitch);
    ma_sound_set_looping(snd, loop ? MA_TRUE : MA_FALSE);
    ma_sound_start(snd);

    uint32_t handle = s_NextHandle++;
    std::lock_guard<std::mutex> lk(s_Mutex);
    s_Sounds[handle] = snd;
    return handle;
}

void AudioSystem::Stop(uint32_t handle) {
    std::lock_guard<std::mutex> lk(s_Mutex);
    auto it = s_Sounds.find(handle);
    if (it == s_Sounds.end()) return;
    ma_sound_stop(it->second);
    ma_sound_uninit(it->second);
    delete it->second;
    s_Sounds.erase(it);
}

void AudioSystem::Pause(uint32_t handle) {
    std::lock_guard<std::mutex> lk(s_Mutex);
    auto it = s_Sounds.find(handle);
    if (it != s_Sounds.end()) ma_sound_stop(it->second);
}

void AudioSystem::Resume(uint32_t handle) {
    std::lock_guard<std::mutex> lk(s_Mutex);
    auto it = s_Sounds.find(handle);
    if (it != s_Sounds.end()) ma_sound_start(it->second);
}

void AudioSystem::SetVolume(uint32_t handle, float volume) {
    std::lock_guard<std::mutex> lk(s_Mutex);
    auto it = s_Sounds.find(handle);
    if (it != s_Sounds.end()) ma_sound_set_volume(it->second, volume);
}

void AudioSystem::SetMasterVolume(float volume) {
    if (s_Initialized) ma_engine_set_volume(&s_Engine, volume);
}

bool AudioSystem::IsPlaying(uint32_t handle) {
    std::lock_guard<std::mutex> lk(s_Mutex);
    auto it = s_Sounds.find(handle);
    if (it == s_Sounds.end()) return false;
    return ma_sound_is_playing(it->second) == MA_TRUE;
}

void AudioSystem::StopAll() {
    std::lock_guard<std::mutex> lk(s_Mutex);
    for (auto& [h, snd] : s_Sounds) {
        ma_sound_stop(snd);
        ma_sound_uninit(snd);
        delete snd;
    }
    s_Sounds.clear();
}

} // namespace Pillar
