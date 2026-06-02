/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            DrawMatrix                                                                                               *
 * @file      MusicPlayer.hpp                                                                                          *
 * @brief     TODO: Add brief description                                                                              *
 * @date      Sun Sep 14 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */

#ifndef DRAWMATRIX_MUSICPLAYER
#define DRAWMATRIX_MUSICPLAYER

#include <cstdint>
#include <Arduino.h>  // For String class

#include <ArduinoJson.h>

namespace MusicPlayer {

enum class MusicTrack {
    MUSIC_NATURE = 1,
    MUSIC_ALARM = 2,
    MUSIC_KIDS_1 = 3,
    MUSIC_KIDS_2 = 4,
};

enum class State {
    STOPPED,
    PLAYING,
    PAUSED
};

enum class PlaybackMode {
    NORMAL = 0,    // Play tracks in order, stop at end
    REPEAT_ALL = 1, // Repeat entire folder
    REPEAT_ONE = 2, // Repeat current track
    SHUFFLE = 3     // Random order
};

enum class EQMode {
    NORMAL = 0,
    POP = 1,
    ROCK = 2,
    JAZZ = 3,
    CLASSIC = 4,
    BASS = 5
};

constexpr uint8_t MAX_VOLUME = 30; // DFPlayer max volume is 30

void init();
void run();
void play(MusicTrack track);
void play_folder_track(uint8_t folder, uint16_t track);
void stop();
void pause();
void prev();
void next();
void start_volume_change();
void stop_volume_change();
void set_volume(uint8_t volume); // volume: 0-30
State get_state();
// ----- Playback Mode & EQ Controls -----
void set_playback_mode(PlaybackMode mode);
PlaybackMode get_playback_mode();
void set_eq_mode(EQMode mode);
EQMode get_eq_mode();
// ----- Utility helpers (non-blocking queries)
uint16_t total_tracks();
uint16_t total_folders();
uint16_t tracks_in_folder(uint8_t folder);
bool sd_online();
uint16_t current_track();
uint8_t get_volume();
// Currently selected (last played) folder (0 if none)
uint8_t current_folder();

// ----- SD Content Management (LittleFS-based) ------
// Load SD content description from LittleFS JSON file
bool load_sd_content();
// Save SD content description to LittleFS JSON file
bool save_sd_content();
// Upload new SD content via JSON string
bool upload_sd_content(const String& jsonContent);
// Get folder count from loaded content
uint8_t get_content_folder_count();
// Get folder info by index (returns folder ID and track count)
bool get_content_folder(uint8_t index, uint8_t &folderId, uint16_t &trackCount, String &folderName);
// Get track info for a folder (returns track filename)
bool get_content_track(uint8_t folderId, uint8_t trackIndex, String &trackName);
// Get all tracks from a folder with their IDs
bool get_folder_tracks(uint8_t folderId, ArduinoJson::JsonArray &tracksArray);
// Check if content is loaded
bool has_content_data();
} // namespace MusicPlayer

#endif /* DRAWMATRIX_MUSICPLAYER */
