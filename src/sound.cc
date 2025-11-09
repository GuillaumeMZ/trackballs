/* sound.cc
   Handles all sound events / background music

   Copyright (C) 2000  Mathias Broxvall
                       Yannick Perret

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sound.h"

#include "settings.h"

#include <SDL2/SDL_mixer.h>

#include <algorithm>  // for std::min/max
#include <filesystem>

namespace fs = std::filesystem;

SDL_AudioSpec audioFormat;

#define MAX_EFFECTS 1024
const char *wavs[MAX_EFFECTS];
Mix_Chunk *effects[MAX_EFFECTS];
int n_effects = 0;

#define MAX_SONGS 1024  // in case someone makes a symlink to his real music directory..
Mix_Music *music[MAX_SONGS];
char *songName[MAX_SONGS];
int n_songs = 0;
int mute = 1;
char musicPreferences[MAX_SONGS];
int hasMusicPreferences = 0;

void soundInit() {
  mute = 0;

  if (Mix_OpenAudio(22050, AUDIO_S16, 2, 4096) < 0) {
    warning(
        "Couldn't open audio: %s\nTry shutting down artsd/esd or run trackballs through "
        "artsdsp/esddsp\n",
        SDL_GetError());
    mute = 1;
    return;
  }

  clearMusicPreferences();

  const auto sfxDir = fs::path(effectiveShareDir) / "sfx";

  if (fs::exists(sfxDir) && fs::is_directory(sfxDir)) {
    for (const auto& item: fs::directory_iterator(sfxDir)) {
      const fs::path& itemPath(item);

      if (itemPath.extension() != ".wav") {
        continue;
      }

      effects[n_effects] = Mix_LoadWAV(fs::absolute(itemPath).c_str());

      if (effects[n_effects]) {
        wavs[n_effects] = strdup(itemPath.filename().c_str());
        n_effects++;
      } else {
        warning("Failed to load '%s'\n", fs::absolute(itemPath).c_str());
      }
    }
  }

  n_songs = 0;

  const auto musicDir = fs::path(effectiveShareDir) / "music";

  if (fs::exists(musicDir) && fs::is_directory(musicDir)) {
    for (const auto& item: fs::directory_iterator(musicDir)) {
      const fs::path& itemPath(item);

      if (itemPath.extension() != ".ogg" && itemPath.extension() != ".mp3") {
        continue;
      }

      music[n_songs] = Mix_LoadMUS(fs::absolute(itemPath).c_str());
      songName[n_songs] = strdup(itemPath.filename().c_str());
      if (!music[n_songs++]) warning("Failed to load '%s'", fs::absolute(itemPath).c_str());
    }
  }
}

int doSpecialSfx = 0;
double musicFade = 1.0;

void soundIdle(Real td) {
  if (mute) return;

  int nomusic = Settings::settings->musicVolume < 1e-3;
  if (nomusic) {
    Mix_PauseMusic();
  } else {
    Mix_ResumeMusic();
  }

  if (doSpecialSfx) {
    musicFade = std::max(0.0, musicFade - 0.3 * td);
    if (musicFade == 0.0) {
      Mix_Volume(1, (int)(128 * Settings::settings->sfxVolume));
      Mix_PlayChannel(1, effects[doSpecialSfx], 0);
      doSpecialSfx = 0;
    }
  } else if (!Mix_Playing(1))
    musicFade = std::min(1.0, musicFade + 0.3 * td);

  if (!nomusic) {
    Mix_VolumeMusic((int)(musicFade * 127.0 * Settings::settings->musicVolume));

    if (!Mix_PlayingMusic() && n_songs) { playNextSong(); }
  }
}

void playNextSong() {
  int i;
  static int oldSong = -1;

  for (int tries = 0; tries < 2; tries++) {
    if (hasMusicPreferences) {
      /* Pick one of the selected songs, with weights */
      int r = random() % hasMusicPreferences;
      for (i = 0; i < n_songs; i++) {
        r -= musicPreferences[i];
        if (r <= 0) break;
      }
      if (i == n_songs) {
        warning("bad weights in music preferences");
        i = 0;
      }
    } else {
      /* Just pick any random song */
      i = random() % n_songs;
    }
    /* This is to make sure we don't play the same song twice,
       unless there realy isn't any alternative */
    if (i != oldSong) {
      oldSong = i;
      break;
    }
  }
  if (music[i])
    Mix_FadeInMusic(music[i], 1, 2000);
  else
    warning("bad song index %d selected", i);
}

void playEffect(int e, float vol) {
  if (mute) return;
  vol = vol * Settings::settings->sfxVolume;
  if (e >= 0 && e < N_EFFECTS && effects[e] && vol > 1e-3) {
    if (strcmp(wavs[e], SFX_LV_COMPLETE) == 0) {
      doSpecialSfx = e;
      musicFade = 1.0;
    } else if (!Mix_Playing(0)) {
      Mix_Volume(0, (int)(vol * 128));
      Mix_PlayChannel(0, effects[e], 0);
    } else if (!Mix_Playing(2)) {
      Mix_Volume(2, (int)(vol * 128));
      Mix_PlayChannel(2, effects[e], 0);
    } else if (!Mix_Playing(3)) {
      Mix_Volume(3, (int)(vol * 128));
      Mix_PlayChannel(3, effects[e], 0);
    } else if (!Mix_Playing(4)) {
      Mix_Volume(4, (int)(vol * 128));
      Mix_PlayChannel(4, effects[e], 0);
    }
  }
}
void playEffect(int e) { playEffect(e, 1.); }

void playEffect(const char *name) {
  if (mute) return;
  for (int e = 0; e < N_EFFECTS; e++)
    if (effects[e] && strcmp(wavs[e], name) == 0) {
      playEffect(e, 1.);
      return;
    }
}
void playEffect(const char *name, float vol) {
  if (mute) return;
  for (int e = 0; e < N_EFFECTS; e++)
    if (effects[e] && strcmp(wavs[e], name) == 0) {
      playEffect(e, vol);
      return;
    }
}

void volumeChanged() {
  if (mute) return;
  Mix_Volume(0, (int)(127.0 * Settings::settings->sfxVolume));
  Mix_VolumeMusic((int)(127.0 * Settings::settings->musicVolume));
}

void clearMusicPreferences() {
  memset(musicPreferences, 0, sizeof(musicPreferences));
  hasMusicPreferences = 0;
}

void setMusicPreference(char *name, int weight) {
  int i;
  for (i = 0; i < n_songs; i++) {
    if (strcmp(songName[i], name) == 0) break;
  }
  if (i == n_songs) {
    warning("setMusicPreference: failed to find song '%s'", name);
    return;
  } else {
    hasMusicPreferences -= musicPreferences[i];
    hasMusicPreferences += weight;
    musicPreferences[i] = weight;
  }
}
