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

#include <functional>
#include <map>

namespace MusicPlayer {
// Use ESPSoftwareSerial (not the default SoftwareSerial!)
SoftwareSerial mySoftwareSerial(D7, D5); // RX, TX
State currentState = State::STOPPED;

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

class Mp3Notify;
typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;

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
    myDFPlayer.reset();
    Serial.println("DFPlayer Mini online.");
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
void run() {}
} // namespace MusicPlayer