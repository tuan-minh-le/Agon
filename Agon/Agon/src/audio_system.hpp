#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <AL/al.h>
#include <AL/alc.h>
#include "cgp/cgp.hpp"

// Forward declarations for file loading
struct AudioClip {
    ALuint buffer_id;
    float duration;
    bool loaded;
    
    AudioClip() : buffer_id(0), duration(0.0f), loaded(false) {}
};

struct AudioSource {
    ALuint source_id;
    cgp::vec3 position;
    float volume;
    bool looping;
    bool playing;
    
    AudioSource() : source_id(0), position(0,0,0), volume(1.0f), looping(false), playing(false) {}
};

class AudioSystem {
private:
    ALCdevice* device;
    ALCcontext* context;
    
    // Audio clips storage
    std::unordered_map<std::string, std::unique_ptr<AudioClip>> audio_clips;
    
    // Audio sources for 3D positional sound
    std::unordered_map<std::string, std::unique_ptr<AudioSource>> audio_sources;
    
    // Global audio sources (non-positional)
    std::unordered_map<std::string, ALuint> global_sources;
    
    cgp::vec3 listener_position;
    cgp::vec3 listener_velocity;
    cgp::vec3 listener_orientation[2]; // forward and up vectors
    
    bool initialized;
    float master_volume;
    
    // Helper methods
    bool load_wav_file(const std::string& filepath, AudioClip* clip);
    bool load_mp3_file(const std::string& filepath, AudioClip* clip);
    void cleanup_source(ALuint source_id);
    void update_listener_properties();

public:
    AudioSystem();
    ~AudioSystem();
    
    // Initialization and cleanup
    bool initialize();
    void shutdown();
    
    // Audio clip management
    bool load_audio_clip(const std::string& name, const std::string& filepath);
    void unload_audio_clip(const std::string& name);
    
    // Global (2D) sound playback
    bool play_sound_2d(const std::string& clip_name, float volume = 1.0f, bool loop = false);
    bool play_sound_2d_once(const std::string& clip_name, float volume = 1.0f);
    void stop_sound_2d(const std::string& clip_name);
    bool is_playing_2d(const std::string& clip_name);
    
    // 3D positional sound playback
    bool play_sound_3d(const std::string& source_name, const std::string& clip_name, 
                       const cgp::vec3& position, float volume = 1.0f, bool loop = false);
    void stop_sound_3d(const std::string& source_name);
    void update_sound_3d_position(const std::string& source_name, const cgp::vec3& position);
    void update_sound_3d_volume(const std::string& source_name, float volume);
    bool is_playing_3d(const std::string& source_name);
    
    // Listener management (for 3D audio)
    void set_listener_position(const cgp::vec3& position);
    void set_listener_velocity(const cgp::vec3& velocity);
    void set_listener_orientation(const cgp::vec3& forward, const cgp::vec3& up);
    
    // Volume control
    void set_master_volume(float volume);
    float get_master_volume() const;
    
    // Update system (call every frame)
    void update();
    
    // Utility methods
    bool is_initialized() const { return initialized; }
    void stop_all_sounds();
    
    // Error handling
    std::string get_last_error() const;
};

// Helper class for managing footstep sounds specifically
class FootstepAudioManager {
private:
    AudioSystem* audio_system;
    
    // Timing for footstep sounds
    float walking_step_interval;
    float running_step_interval;
    float last_step_time;
    
    // Volume settings
    float local_footstep_volume;
    float remote_footstep_volume;
    float max_audible_distance;
    
public:
    FootstepAudioManager(AudioSystem* audio_sys);
    
    // Initialize footstep sounds
    bool initialize(const std::string& walking_sound_path, const std::string& running_sound_path);
    
    // Update footstep sounds for local player
    void update_local_player_footsteps(bool is_moving, bool is_running, float dt);
    
    // Update footstep sounds for remote players
    void update_remote_player_footsteps(const std::string& player_id, bool is_moving, 
                                       bool is_running, const cgp::vec3& position, 
                                       const cgp::vec3& listener_position, float dt);
    
    // Stop footstep sounds for a specific player
    void stop_player_footsteps(const std::string& player_id);
    
    // Stop all footstep sounds
    void stop_all_footsteps();
    
    // Volume settings
    void set_local_volume(float volume) { local_footstep_volume = volume; }
    void set_remote_volume(float volume) { remote_footstep_volume = volume; }
    void set_max_distance(float distance) { max_audible_distance = distance; }
};
