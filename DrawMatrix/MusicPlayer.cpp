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
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
#include <functional>
#include <map>

namespace MusicPlayer {
// Use ESPSoftwareSerial (not the default SoftwareSerial!)
SoftwareSerial mySoftwareSerial(D7, D5); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
State currentState = State::STOPPED;

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

    myDFPlayer.pause();
    currentState = State::PAUSED;
}

// --------------------------------------------------------------------------------------
State get_state() { return currentState; }

} // namespace MusicPlayer