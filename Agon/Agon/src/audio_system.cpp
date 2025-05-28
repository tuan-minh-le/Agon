#include "audio_system.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>

// WAV file structures for chunk-based parsing
struct RIFFHeader {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size in bytes
    char wave[4];           // "WAVE"
};

struct ChunkHeader {
    char id[4];             // Chunk identifier
    uint32_t size;          // Chunk size
};

struct FmtChunk {
    uint16_t audio_format;  // Audio format (1 = PCM)
    uint16_t channels;      // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Byte rate
    uint16_t block_align;   // Block align
    uint16_t bits_per_sample; // Bits per sample
};

AudioSystem::AudioSystem() 
    : device(nullptr), context(nullptr), initialized(false), master_volume(1.0f),
      listener_position(0,0,0), listener_velocity(0,0,0) {
    listener_orientation[0] = cgp::vec3(0, 0, -1); // forward
    listener_orientation[1] = cgp::vec3(0, 1, 0);  // up
}

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::initialize() {
    if (initialized) {
        return true;
    }
    
    // Open the default audio device
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "AudioSystem: Failed to open audio device" << std::endl;
        return false;
    }
    
    // Create audio context
    context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cerr << "AudioSystem: Failed to create audio context" << std::endl;
        alcCloseDevice(device);
        device = nullptr;
        return false;
    }
    
    // Make context current
    if (!alcMakeContextCurrent(context)) {
        std::cerr << "AudioSystem: Failed to make context current" << std::endl;
        alcDestroyContext(context);
        alcCloseDevice(device);
        context = nullptr;
        device = nullptr;
        return false;
    }
    
    // Set initial listener properties
    update_listener_properties();
    
    initialized = true;
    std::cout << "AudioSystem: Successfully initialized" << std::endl;
    return true;
}

void AudioSystem::shutdown() {
    if (!initialized) {
        return;
    }
    
    // Stop all sounds
    stop_all_sounds();
    
    // Clean up global sources
    for (auto& pair : global_sources) {
        cleanup_source(pair.second);
    }
    global_sources.clear();
    
    // Clean up 3D sources
    for (auto& pair : audio_sources) {
        if (pair.second && pair.second->source_id != 0) {
            cleanup_source(pair.second->source_id);
        }
    }
    audio_sources.clear();
    
    // Clean up audio clips
    for (auto& pair : audio_clips) {
        if (pair.second && pair.second->buffer_id != 0) {
            alDeleteBuffers(1, &pair.second->buffer_id);
        }
    }
    audio_clips.clear();
    
    // Clean up OpenAL context and device
    if (context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        context = nullptr;
    }
    
    if (device) {
        alcCloseDevice(device);
        device = nullptr;
    }
    
    initialized = false;
    std::cout << "AudioSystem: Shutdown complete" << std::endl;
}

bool AudioSystem::load_wav_file(const std::string& filepath, AudioClip* clip) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "AudioSystem: Failed to open WAV file: " << filepath << std::endl;
        return false;
    }
    
    // Read RIFF header
    RIFFHeader riff_header;
    file.read(reinterpret_cast<char*>(&riff_header), sizeof(RIFFHeader));
    
    if (std::strncmp(riff_header.riff, "RIFF", 4) != 0 || 
        std::strncmp(riff_header.wave, "WAVE", 4) != 0) {
        std::cerr << "AudioSystem: Invalid WAV file format: " << filepath << std::endl;
        return false;
    }
    
    // Parse chunks to find fmt and data
    FmtChunk fmt_chunk;
    std::vector<char> audio_data;
    uint32_t data_size = 0;
    bool fmt_found = false;
    bool data_found = false;
    
    while (file.good() && (!fmt_found || !data_found)) {
        ChunkHeader chunk_header;
        file.read(reinterpret_cast<char*>(&chunk_header), sizeof(ChunkHeader));
        
        if (!file.good()) break;
        
        if (std::strncmp(chunk_header.id, "fmt ", 4) == 0) {
            // Read format chunk
            file.read(reinterpret_cast<char*>(&fmt_chunk), sizeof(FmtChunk));
            
            // Skip any extra bytes in the fmt chunk
            if (chunk_header.size > sizeof(FmtChunk)) {
                file.seekg(chunk_header.size - sizeof(FmtChunk), std::ios::cur);
            }
            
            fmt_found = true;
            
            if (fmt_chunk.audio_format != 1) {
                std::cerr << "AudioSystem: Unsupported WAV format (only PCM supported): " << filepath << std::endl;
                return false;
            }
        } else if (std::strncmp(chunk_header.id, "data", 4) == 0) {
            // Read data chunk
            data_size = chunk_header.size;
            audio_data.resize(data_size);
            file.read(audio_data.data(), data_size);
            data_found = true;
        } else {
            // Skip unknown chunks
            file.seekg(chunk_header.size, std::ios::cur);
        }
    }
    
    file.close();
    
    if (!fmt_found || !data_found) {
        std::cerr << "AudioSystem: Required chunks not found in WAV file: " << filepath << std::endl;
        return false;
    }
    
    // Determine OpenAL format
    ALenum format;
    if (fmt_chunk.channels == 1) {
        format = (fmt_chunk.bits_per_sample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
    } else if (fmt_chunk.channels == 2) {
        format = (fmt_chunk.bits_per_sample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
    } else {
        std::cerr << "AudioSystem: Unsupported channel count: " << fmt_chunk.channels << std::endl;
        return false;
    }
    
    // Create OpenAL buffer
    alGenBuffers(1, &clip->buffer_id);
    alBufferData(clip->buffer_id, format, audio_data.data(), data_size, fmt_chunk.sample_rate);
    
    // Check for errors
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "AudioSystem: OpenAL error loading WAV: " << error << std::endl;
        alDeleteBuffers(1, &clip->buffer_id);
        clip->buffer_id = 0;
        return false;
    }
    
    // Calculate duration
    clip->duration = static_cast<float>(data_size) / (fmt_chunk.sample_rate * fmt_chunk.channels * (fmt_chunk.bits_per_sample / 8));
    clip->loaded = true;
    
    std::cout << "AudioSystem: Successfully loaded WAV file: " << filepath << " (duration: " << clip->duration << "s)" << std::endl;
    return true;
}

bool AudioSystem::load_mp3_file(const std::string& filepath, AudioClip* clip) {
    // For now, we'll just return false and suggest converting MP3 to WAV
    // A full MP3 implementation would require additional libraries like mpg123 or similar
    std::cerr << "AudioSystem: MP3 loading not implemented. Please convert " << filepath << " to WAV format." << std::endl;
    return false;
}

bool AudioSystem::load_audio_clip(const std::string& name, const std::string& filepath) {
    if (!initialized) {
        std::cerr << "AudioSystem: Cannot load audio clip - system not initialized" << std::endl;
        return false;
    }
    
    // Check if already loaded
    if (audio_clips.find(name) != audio_clips.end()) {
        std::cout << "AudioSystem: Audio clip '" << name << "' already loaded" << std::endl;
        return true;
    }
    
    auto clip = std::make_unique<AudioClip>();
    
    // Determine file type by extension
    std::string extension = filepath.substr(filepath.find_last_of('.'));
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    bool success = false;
    if (extension == ".wav") {
        success = load_wav_file(filepath, clip.get());
    } else if (extension == ".mp3") {
        success = load_mp3_file(filepath, clip.get());
    } else {
        std::cerr << "AudioSystem: Unsupported audio format: " << extension << std::endl;
        return false;
    }
    
    if (success) {
        audio_clips[name] = std::move(clip);
        return true;
    }
    
    return false;
}

void AudioSystem::unload_audio_clip(const std::string& name) {
    auto it = audio_clips.find(name);
    if (it != audio_clips.end()) {
        if (it->second->buffer_id != 0) {
            alDeleteBuffers(1, &it->second->buffer_id);
        }
        audio_clips.erase(it);
        std::cout << "AudioSystem: Unloaded audio clip: " << name << std::endl;
    }
}

bool AudioSystem::play_sound_2d(const std::string& clip_name, float volume, bool loop) {
    if (!initialized) return false;
    
    auto clip_it = audio_clips.find(clip_name);
    if (clip_it == audio_clips.end() || !clip_it->second->loaded) {
        std::cerr << "AudioSystem: Audio clip not found or not loaded: " << clip_name << std::endl;
        return false;
    }
    
    // Check if we already have a global source for this clip
    auto source_it = global_sources.find(clip_name);
    ALuint source_id;
    
    if (source_it != global_sources.end()) {
        source_id = source_it->second;
        // Stop current playback if any
        alSourceStop(source_id);
    } else {
        // Create new source
        alGenSources(1, &source_id);
        if (alGetError() != AL_NO_ERROR) {
            std::cerr << "AudioSystem: Failed to create audio source for: " << clip_name << std::endl;
            return false;
        }
        global_sources[clip_name] = source_id;
    }
    
    // Configure source
    alSourcei(source_id, AL_BUFFER, clip_it->second->buffer_id);
    alSourcef(source_id, AL_GAIN, volume * master_volume);
    alSourcei(source_id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    alSourcei(source_id, AL_SOURCE_RELATIVE, AL_TRUE); // 2D sound
    alSource3f(source_id, AL_POSITION, 0.0f, 0.0f, 0.0f);
    
    // Play sound
    alSourcePlay(source_id);
    
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "AudioSystem: Error playing 2D sound: " << error << std::endl;
        return false;
    }
    
    return true;
}

bool AudioSystem::play_sound_2d_once(const std::string& clip_name, float volume) {
    return play_sound_2d(clip_name, volume, false);
}

void AudioSystem::stop_sound_2d(const std::string& clip_name) {
    auto it = global_sources.find(clip_name);
    if (it != global_sources.end()) {
        alSourceStop(it->second);
    }
}

bool AudioSystem::is_playing_2d(const std::string& clip_name) {
    auto it = global_sources.find(clip_name);
    if (it != global_sources.end()) {
        ALint state;
        alGetSourcei(it->second, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }
    return false;
}

bool AudioSystem::play_sound_3d(const std::string& source_name, const std::string& clip_name, 
                                const cgp::vec3& position, float volume, bool loop) {
    if (!initialized) return false;
    
    auto clip_it = audio_clips.find(clip_name);
    if (clip_it == audio_clips.end() || !clip_it->second->loaded) {
        std::cerr << "AudioSystem: Audio clip not found or not loaded: " << clip_name << std::endl;
        return false;
    }
    
    // Get or create 3D source
    auto source_it = audio_sources.find(source_name);
    std::unique_ptr<AudioSource>* source_ptr;
    
    if (source_it != audio_sources.end()) {
        source_ptr = &source_it->second;
        // Stop current playback if any
        alSourceStop((*source_ptr)->source_id);
    } else {
        auto new_source = std::make_unique<AudioSource>();
        alGenSources(1, &new_source->source_id);
        if (alGetError() != AL_NO_ERROR) {
            std::cerr << "AudioSystem: Failed to create 3D audio source for: " << source_name << std::endl;
            return false;
        }
        audio_sources[source_name] = std::move(new_source);
        source_ptr = &audio_sources[source_name];
    }
    
    AudioSource* source = source_ptr->get();
    
    // Configure 3D source
    alSourcei(source->source_id, AL_BUFFER, clip_it->second->buffer_id);
    alSourcef(source->source_id, AL_GAIN, volume * master_volume);
    alSourcei(source->source_id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    alSourcei(source->source_id, AL_SOURCE_RELATIVE, AL_FALSE); // 3D sound
    alSource3f(source->source_id, AL_POSITION, position.x, position.y, position.z);
    
    // Set distance attenuation (optional, for realistic 3D audio)
    alSourcef(source->source_id, AL_REFERENCE_DISTANCE, 1.0f);
    alSourcef(source->source_id, AL_MAX_DISTANCE, 50.0f);
    alSourcef(source->source_id, AL_ROLLOFF_FACTOR, 1.0f);
    
    // Update source properties
    source->position = position;
    source->volume = volume;
    source->looping = loop;
    source->playing = true;
    
    // Play sound
    alSourcePlay(source->source_id);
    
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "AudioSystem: Error playing 3D sound: " << error << std::endl;
        source->playing = false;
        return false;
    }
    
    return true;
}

void AudioSystem::stop_sound_3d(const std::string& source_name) {
    auto it = audio_sources.find(source_name);
    if (it != audio_sources.end()) {
        alSourceStop(it->second->source_id);
        it->second->playing = false;
    }
}

void AudioSystem::update_sound_3d_position(const std::string& source_name, const cgp::vec3& position) {
    auto it = audio_sources.find(source_name);
    if (it != audio_sources.end()) {
        it->second->position = position;
        alSource3f(it->second->source_id, AL_POSITION, position.x, position.y, position.z);
    }
}

void AudioSystem::update_sound_3d_volume(const std::string& source_name, float volume) {
    auto it = audio_sources.find(source_name);
    if (it != audio_sources.end()) {
        it->second->volume = volume;
        alSourcef(it->second->source_id, AL_GAIN, volume * master_volume);
    }
}

bool AudioSystem::is_playing_3d(const std::string& source_name) {
    auto it = audio_sources.find(source_name);
    if (it != audio_sources.end()) {
        ALint state;
        alGetSourcei(it->second->source_id, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }
    return false;
}

void AudioSystem::set_listener_position(const cgp::vec3& position) {
    listener_position = position;
    alListener3f(AL_POSITION, position.x, position.y, position.z);
}

void AudioSystem::set_listener_velocity(const cgp::vec3& velocity) {
    listener_velocity = velocity;
    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
}

void AudioSystem::set_listener_orientation(const cgp::vec3& forward, const cgp::vec3& up) {
    listener_orientation[0] = forward;
    listener_orientation[1] = up;
    
    ALfloat orientation[] = {
        forward.x, forward.y, forward.z,
        up.x, up.y, up.z
    };
    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioSystem::set_master_volume(float volume) {
    master_volume = std::max(0.0f, std::min(1.0f, volume));
    alListenerf(AL_GAIN, master_volume);
    
    // Update all source volumes
    for (auto& pair : global_sources) {
        ALfloat current_volume;
        alGetSourcef(pair.second, AL_GAIN, &current_volume);
        alSourcef(pair.second, AL_GAIN, current_volume * master_volume);
    }
    
    for (auto& pair : audio_sources) {
        alSourcef(pair.second->source_id, AL_GAIN, pair.second->volume * master_volume);
    }
}

float AudioSystem::get_master_volume() const {
    return master_volume;
}

void AudioSystem::update() {
    if (!initialized) return;
    
    // Update listener properties
    update_listener_properties();
    
    // Clean up finished non-looping sounds
    for (auto it = global_sources.begin(); it != global_sources.end(); ) {
        ALint state;
        alGetSourcei(it->second, AL_SOURCE_STATE, &state);
        
        if (state == AL_STOPPED) {
            ALint looping;
            alGetSourcei(it->second, AL_LOOPING, &looping);
            
            if (looping == AL_FALSE) {
                // Non-looping sound finished, clean it up
                cleanup_source(it->second);
                it = global_sources.erase(it);
                continue;
            }
        }
        ++it;
    }
    
    // Update 3D source states
    for (auto& pair : audio_sources) {
        ALint state;
        alGetSourcei(pair.second->source_id, AL_SOURCE_STATE, &state);
        pair.second->playing = (state == AL_PLAYING);
    }
}

void AudioSystem::stop_all_sounds() {
    // Stop all global sources
    for (auto& pair : global_sources) {
        alSourceStop(pair.second);
    }
    
    // Stop all 3D sources
    for (auto& pair : audio_sources) {
        alSourceStop(pair.second->source_id);
        pair.second->playing = false;
    }
}

void AudioSystem::cleanup_source(ALuint source_id) {
    if (source_id != 0) {
        alSourceStop(source_id);
        alDeleteSources(1, &source_id);
    }
}

void AudioSystem::update_listener_properties() {
    alListener3f(AL_POSITION, listener_position.x, listener_position.y, listener_position.z);
    alListener3f(AL_VELOCITY, listener_velocity.x, listener_velocity.y, listener_velocity.z);
    
    ALfloat orientation[] = {
        listener_orientation[0].x, listener_orientation[0].y, listener_orientation[0].z, // forward
        listener_orientation[1].x, listener_orientation[1].y, listener_orientation[1].z  // up
    };
    alListenerfv(AL_ORIENTATION, orientation);
}

std::string AudioSystem::get_last_error() const {
    ALenum error = alGetError();
    switch (error) {
        case AL_NO_ERROR: return "No error";
        case AL_INVALID_NAME: return "Invalid name";
        case AL_INVALID_ENUM: return "Invalid enum";
        case AL_INVALID_VALUE: return "Invalid value";
        case AL_INVALID_OPERATION: return "Invalid operation";
        case AL_OUT_OF_MEMORY: return "Out of memory";
        default: return "Unknown error";
    }
}

// FootstepAudioManager implementation
FootstepAudioManager::FootstepAudioManager(AudioSystem* audio_sys) 
    : audio_system(audio_sys), walking_step_interval(0.5f), running_step_interval(0.3f), 
      last_step_time(0.0f), local_footstep_volume(0.7f), remote_footstep_volume(0.5f),
      max_audible_distance(20.0f) {
}

bool FootstepAudioManager::initialize(const std::string& walking_sound_path, const std::string& running_sound_path) {
    if (!audio_system || !audio_system->is_initialized()) {
        std::cerr << "FootstepAudioManager: Audio system not available" << std::endl;
        return false;
    }
    
    // Load footstep sound clips
    bool success = true;
    success &= audio_system->load_audio_clip("walking", walking_sound_path);
    success &= audio_system->load_audio_clip("running", running_sound_path);
    
    if (success) {
        std::cout << "FootstepAudioManager: Successfully initialized" << std::endl;
    } else {
        std::cerr << "FootstepAudioManager: Failed to load footstep sounds" << std::endl;
    }
    
    return success;
}

void FootstepAudioManager::update_local_player_footsteps(bool is_moving, bool is_running, float dt) {
    if (!audio_system || !is_moving) {
        // Stop footstep sounds if not moving
        audio_system->stop_sound_2d("walking");
        audio_system->stop_sound_2d("running");
        last_step_time = 0.0f;
        return;
    }
    
    last_step_time += dt;
    
    float step_interval = is_running ? running_step_interval : walking_step_interval;
    std::string sound_name = is_running ? "running" : "walking";
    std::string other_sound = is_running ? "walking" : "running";
    
    if (last_step_time >= step_interval) {
        // Stop the other sound type to avoid overlapping
        audio_system->stop_sound_2d(other_sound);
        // Play footstep sound
        audio_system->play_sound_2d_once(sound_name, local_footstep_volume);
        last_step_time = 0.0f;
    }
}

void FootstepAudioManager::update_remote_player_footsteps(const std::string& player_id, bool is_moving, 
                                                         bool is_running, const cgp::vec3& position, 
                                                         const cgp::vec3& listener_position, float dt) {
    if (!audio_system) return;
    
    std::string source_name = "footsteps_" + player_id;
    
    if (!is_moving) {
        // Stop footstep sounds if not moving
        audio_system->stop_sound_3d(source_name);
        return;
    }
    
    // Calculate distance for volume attenuation
    float distance = cgp::norm(position - listener_position);
    if (distance > max_audible_distance) {
        // Too far away, stop the sound
        audio_system->stop_sound_3d(source_name);
        return;
    }
    
    // Calculate volume based on distance
    float volume_multiplier = std::max(0.0f, 1.0f - (distance / max_audible_distance));
    float final_volume = remote_footstep_volume * volume_multiplier;
    
    std::string sound_name = is_running ? "running" : "walking";
    
    // Check if we need to start/restart the footstep sound
    if (!audio_system->is_playing_3d(source_name)) {
        // Start playing footsteps with looping
        audio_system->play_sound_3d(source_name, sound_name, position, final_volume, true);
    } else {
        // Update position and volume
        audio_system->update_sound_3d_position(source_name, position);
        audio_system->update_sound_3d_volume(source_name, final_volume);
    }
}

void FootstepAudioManager::stop_player_footsteps(const std::string& player_id) {
    if (!audio_system) return;
    
    std::string source_name = "footsteps_" + player_id;
    audio_system->stop_sound_3d(source_name);
}

void FootstepAudioManager::stop_all_footsteps() {
    if (!audio_system) return;
    
    // Stop local footsteps
    audio_system->stop_sound_2d("local_footsteps");
    
    // Note: We can't easily stop all remote footsteps without tracking them
    // This would require the AudioSystem to provide a way to enumerate sources by pattern
    std::cout << "FootstepAudioManager: Stopped local footsteps (remote footsteps will fade naturally)" << std::endl;
}
