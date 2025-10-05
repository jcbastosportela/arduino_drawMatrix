/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            DrawMatrix                                                                                               *
 * @file      MusicPlayer.cpp                                                                                          *
 * @brief     TODO: Add brief description                                                                              *
 * @date      Sun Sep 14 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */
#include "MusicPlayer.hpp"

// clang-format off
#include <SoftwareSerial.h>         // needs to be before DFMiniMp3.h
#include <DFMiniMp3.h>              // this library seems to be more robust than DFRobotDFPlayerMini
// #include <DFRobotDFPlayerMini.h>
// clang-format on

#include "AsyncTasker.hpp"
#include <ArduinoJson.h>
#include <LittleFS.h>

#include <functional>
#include <map>

namespace MusicPlayer {
// Use ESPSoftwareSerial (not the default SoftwareSerial!)
SoftwareSerial mySoftwareSerial(D7, D5); // RX, TX
State currentState = State::STOPPED;

// SD Content data structure
struct TrackInfo { String name; };
struct FolderInfo {
    uint8_t id;
    String name;
    std::vector<TrackInfo> tracks;
};
static std::vector<FolderInfo> sdContent;
static bool contentLoaded = false;
const char* SD_CONTENT_FILE = "/sd_content.json";

#if 0
DFRobotDFPlayerMini myDFPlayer;
std::map<MusicTrack, std::function<void()>> trackActions = {
    {MusicTrack::MUSIC_NATURE, []() { myDFPlayer.playFolder(1, 1); }},
    {MusicTrack::MUSIC_ALARM, []() { myDFPlayer.playFolder(1, 2); }},
    {MusicTrack::MUSIC_KIDS_1, []() { myDFPlayer.playFolder(2, 1); }},
    {MusicTrack::MUSIC_KIDS_2, []() { myDFPlayer.playFolder(2, 2); }},
    // Add more tracks as needed
};

// --------------------------------------------------------------------------------------
void init() {
    mySoftwareSerial.begin(9600); // DFPlayer
    if (!myDFPlayer.begin(mySoftwareSerial, true, false)) {
        Serial.println("DFPlayer Mini not detected!");
        while (true)
            ;
    }
    Serial.println("DFPlayer Mini online.");
}
#else

// #define DFMP3_USE_NOACK

class Mp3Notify;
#ifdef DFMP3_USE_NOACK
// Some DFPlayer clones require the IncongruousNoAck variant; enable with build flag -DDFMP3_USE_NOACK
typedef DFMiniMp3<SoftwareSerial, Mp3Notify, Mp3ChipIncongruousNoAck> DfMp3;
#else
typedef DFMiniMp3<SoftwareSerial, Mp3Notify, Mp3ChipIncongruousNoAck, 5000> DfMp3;
#endif

class Mp3Notify {
  public:
    static void PrintlnSourceAction(DfMp3_PlaySources source, const char *action) {
        if (source & DfMp3_PlaySources_Sd) {
            Serial.print("SD Card, ");
        }
        if (source & DfMp3_PlaySources_Usb) {
            Serial.print("USB Disk, ");
        }
        if (source & DfMp3_PlaySources_Flash) {
            Serial.print("Flash, ");
        }
        Serial.println(action);
    }
    static void OnError([[maybe_unused]] DfMp3 &mp3, uint16_t errorCode) {
        // see DfMp3_Error for code meaning
        Serial.println();
        Serial.print("Com Error ");
        Serial.println(errorCode);
    }
    static void OnPlayFinished([[maybe_unused]] DfMp3 &mp3, [[maybe_unused]] DfMp3_PlaySources source, uint16_t track) {
        Serial.print("Play finished for #");
        Serial.println(track);
    }
    static void OnPlaySourceOnline([[maybe_unused]] DfMp3 &mp3, DfMp3_PlaySources source) {
        PrintlnSourceAction(source, "online");
    }
    static void OnPlaySourceInserted([[maybe_unused]] DfMp3 &mp3, DfMp3_PlaySources source) {
        PrintlnSourceAction(source, "inserted");
    }
    static void OnPlaySourceRemoved([[maybe_unused]] DfMp3 &mp3, DfMp3_PlaySources source) {
        PrintlnSourceAction(source, "removed");
    }
};

DfMp3 myDFPlayer(mySoftwareSerial);
std::map<MusicTrack, std::function<void()>> trackActions = {
    {MusicTrack::MUSIC_NATURE, []() { myDFPlayer.playFolderTrack(1, 1); }},
    {MusicTrack::MUSIC_ALARM, []() { myDFPlayer.playFolderTrack(1, 2); }},
    {MusicTrack::MUSIC_KIDS_1, []() { myDFPlayer.playFolderTrack(2, 1); }},
    {MusicTrack::MUSIC_KIDS_2, []() { myDFPlayer.playFolderTrack(2, 2); }},
    // Add more tracks as needed
};

// --------------------------------------------------------------------------------------
void init() {
    mySoftwareSerial.begin(9600); // DFPlayer
    myDFPlayer.begin();
    myDFPlayer.reset(); // waits for online notification
    myDFPlayer.setComRetries(3);

    Serial.println("[MusicPlayer] DFPlayer Mini online (post-reset)");

    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("[MusicPlayer] LittleFS mount failed");
    }

    // Try to load SD content from LittleFS first
    if (!load_sd_content()) {
        Serial.println("[MusicPlayer] No content file found, will use DFPlayer queries");
    }
}
#endif // DFRobotDFPlayerMini_cpp

// --------------------------------------------------------------------------------------
void play(MusicTrack track) {
    auto it = trackActions.find(track);
    if (it != trackActions.end()) {
        it->second(); // Call the associated function
        currentState = State::PLAYING;
    } else {
        Serial.println("Track not found!");
    }
}

// --------------------------------------------------------------------------------------
// play by folder and track number (numeric MP3 filename index)
void play_folder_track(uint8_t folder, uint16_t track) {
    // The library supports playFolderTrack (8-bit track) and playFolderTrack16
    if (track <= 0xFF) {
        myDFPlayer.playFolderTrack(folder, static_cast<uint8_t>(track));
    } else {
        myDFPlayer.playFolderTrack16(folder, track);
    }
    currentState = State::PLAYING;
}

// --------------------------------------------------------------------------------------
void stop() {
    myDFPlayer.stop();
    currentState = State::STOPPED;
}

// --------------------------------------------------------------------------------------
void pause() {
    if (currentState != State::PAUSED) {
        myDFPlayer.pause();
        currentState = State::PAUSED;
    } else {
        myDFPlayer.start();
        currentState = State::PLAYING;
    }
}

// --------------------------------------------------------------------------------------
void next() {
    myDFPlayer.nextTrack();
    currentState = State::PLAYING;
}

void prev() {
    myDFPlayer.prevTrack();
    currentState = State::PLAYING;
}

// --------------------------------------------------------------------------------------
State get_state() { return currentState; }

static bool stop_volume_change_flag = false;
static bool increase_volume_flag = false;
// --------------------------------------------------------------------------------------
void start_volume_change() {
    stop_volume_change_flag = false;
    AsyncTasker::schedule(100, [](uint64_t t, uint64_t &d, bool &repeat) {
        Serial.printf("Volume change step %s\n", increase_volume_flag ? "<+" : "<-");
        if (increase_volume_flag) {
            myDFPlayer.increaseVolume();
        } else {
            myDFPlayer.decreaseVolume();
        }
        repeat = !stop_volume_change_flag;
    },
    true);
}

// --------------------------------------------------------------------------------------
void stop_volume_change() {
    stop_volume_change_flag = true;
    increase_volume_flag ^= true;
}

// --------------------------------------------------------------------------------------
void set_volume(uint8_t volume) {
    if (volume > MAX_VOLUME) {
        volume = MAX_VOLUME;
    }
    myDFPlayer.setVolume(volume);
}

// --------------------------------------------------------------------------------------
void run() {
    // Process any notifications / replies from the DFPlayer
    myDFPlayer.loop();
}

// --------------------------------------------------------------------------------------
uint16_t total_tracks() {
    if (contentLoaded) {
        uint16_t total = 0;
        for (const auto& folder : sdContent) {
            total += folder.tracks.size();
        }
        return total;
    }
    // Fallback to DFPlayer query
    return myDFPlayer.getTotalTrackCount(DfMp3_PlaySource_Sd);
}

// --------------------------------------------------------------------------------------
uint16_t total_folders() {
    if (contentLoaded) {
        return sdContent.size();
    }
    // Fallback to DFPlayer query
    return myDFPlayer.getTotalFolderCount();
}

// --------------------------------------------------------------------------------------
uint16_t tracks_in_folder(uint8_t folder) {
    if (contentLoaded) {
        for (const auto& f : sdContent) {
            if (f.id == folder) {
                return f.tracks.size();
            }
        }
        return 0;
    }
    // Fallback to DFPlayer query
    return myDFPlayer.getFolderTrackCount(folder);
}

// --------------------------------------------------------------------------------------
bool sd_online() {
    // ensure notifications processed
    myDFPlayer.loop();
    return myDFPlayer.isOnline();
}

// --------------------------------------------------------------------------------------
uint16_t current_track() {
    return myDFPlayer.getCurrentTrack(DfMp3_PlaySource_Sd);
}

// --------------------------------------------------------------------------------------
uint8_t get_volume() {
    return myDFPlayer.getVolume();
}

// ----- SD Content Management --------------------------------------------------------
bool load_sd_content() {
    if (!LittleFS.exists(SD_CONTENT_FILE)) {
        Serial.println("[MusicPlayer] No SD content file found");
        return false;
    }

    auto file = LittleFS.open(SD_CONTENT_FILE, "r");
    if (!file) {
        Serial.println("[MusicPlayer] Failed to open SD content file");
        return false;
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[MusicPlayer] JSON parse error: %s\n", error.c_str());
        return false;
    }

    sdContent.clear();
    JsonArray folders = doc["folders"];
    for (JsonObject folderObj : folders) {
        FolderInfo folder;
        folder.id = folderObj["id"];
        folder.name = folderObj["name"].as<String>();

        JsonArray tracks = folderObj["tracks"];
        for (JsonObject trackObj : tracks) {
            TrackInfo track;
            track.name = trackObj["name"].as<String>();
            folder.tracks.push_back(track);
        }
        sdContent.push_back(folder);
    }

    contentLoaded = true;
    Serial.printf("[MusicPlayer] Loaded %u folders from SD content file\n", sdContent.size());
    return true;
}

bool save_sd_content() {
    StaticJsonDocument<2048> doc;
    JsonArray folders = doc.createNestedArray("folders");

    for (const auto& folder : sdContent) {
        JsonObject folderObj = folders.createNestedObject();
        folderObj["id"] = folder.id;
        folderObj["name"] = folder.name;

        JsonArray tracks = folderObj.createNestedArray("tracks");
        for (const auto& track : folder.tracks) {
            JsonObject trackObj = tracks.createNestedObject();
            trackObj["name"] = track.name;
        }
    }

    auto file = LittleFS.open(SD_CONTENT_FILE, "w");
    if (!file) {
        Serial.println("[MusicPlayer] Failed to create SD content file");
        return false;
    }

    serializeJson(doc, file);
    file.close();

    Serial.printf("[MusicPlayer] Saved %u folders to SD content file\n", sdContent.size());
    return true;
}

bool upload_sd_content(const String& jsonContent) {
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, jsonContent);

    if (error) {
        Serial.printf("[MusicPlayer] Upload JSON parse error: %s\n", error.c_str());
        return false;
    }

    sdContent.clear();
    JsonArray folders = doc["folders"];
    for (JsonObject folderObj : folders) {
        FolderInfo folder;
        folder.id = folderObj["id"];
        folder.name = folderObj["name"].as<String>();

        JsonArray tracks = folderObj["tracks"];
        for (JsonObject trackObj : tracks) {
            TrackInfo track;
            track.name = trackObj["name"].as<String>();
            folder.tracks.push_back(track);
        }
        sdContent.push_back(folder);
    }

    contentLoaded = true;
    bool saved = save_sd_content();

    if (saved) {
        Serial.printf("[MusicPlayer] Uploaded and saved %u folders\n", sdContent.size());
    }
    return saved;
}

uint8_t get_content_folder_count() {
    return contentLoaded ? sdContent.size() : 0;
}

bool get_content_folder(uint8_t index, uint8_t &folderId, uint16_t &trackCount, String &folderName) {
    if (!contentLoaded || index >= sdContent.size()) return false;

    const auto& folder = sdContent[index];
    folderId = folder.id;
    trackCount = folder.tracks.size();
    folderName = folder.name;
    return true;
}

bool get_content_track(uint8_t folderId, uint8_t trackIndex, String &trackName) {
    if (!contentLoaded) return false;

    for (const auto& folder : sdContent) {
        if (folder.id == folderId && trackIndex < folder.tracks.size()) {
            trackName = folder.tracks[trackIndex].name;
            return true;
        }
    }
    return false;
}

bool has_content_data() {
    return contentLoaded;
}

} // namespace MusicPlayer
