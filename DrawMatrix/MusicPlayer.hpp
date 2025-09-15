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
    PAUSED,
};

void init();
void play(MusicTrack track);
void stop();
void pause();
State get_state();
} // namespace MusicPlayer

#endif /* DRAWMATRIX_MUSICPLAYER */
