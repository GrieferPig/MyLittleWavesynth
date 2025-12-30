#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // Required for text
#include <iostream>
#include <string>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_cafe.h>
#include <dirent.h>

#include "mlws.h"

#define SAMPLE_RATE 48000

// Global synth state
struct SynthState
{
   i16 wavetable[WAVETABLE_SIZE];
   Voice voices[3];
   u64 sample_counter;
   u64 note_off_sample;
   u64 loop_end_sample;
};

static SynthState g_synth;

void AudioCallback(void *userdata, Uint8 *stream, int len)
{
   i16 *buffer = (i16 *)stream;
   int sample_count = len / sizeof(i16);
   SynthState *synth = (SynthState *)userdata;

   for (int i = 0; i < sample_count; i++)
   {
      // Loop logic
      if (synth->sample_counter >= synth->loop_end_sample)
      {
         synth->sample_counter = 0;
      }

      // Trigger Note On at 0
      if (synth->sample_counter == 0)
      {
         voice_note_on(&synth->voices[0], 220, SAMPLE_RATE); // A3
         voice_note_on(&synth->voices[1], 277, SAMPLE_RATE); // C#4
         voice_note_on(&synth->voices[2], 329, SAMPLE_RATE); // E4
      }

      // Trigger Note Off at 3s
      if (synth->sample_counter == synth->note_off_sample)
      {
         voice_note_off(&synth->voices[0]);
         voice_note_off(&synth->voices[1]);
         voice_note_off(&synth->voices[2]);
      }

      // Process voices
      i32 mix = 0;
      mix += voice_process(&synth->voices[0]);
      mix += voice_process(&synth->voices[1]);
      mix += voice_process(&synth->voices[2]);

      // Normalize and clamp
      mix /= 3;
      if (mix > 32767)
         mix = 32767;
      else if (mix < -32768)
         mix = -32768;

      buffer[i] = (i16)mix;

      synth->sample_counter++;
   }
}

// Helper function to print available audio devices to Console
void PrintAudioDevices()
{
   int count = SDL_GetNumAudioDevices(0); // 0 = Playback devices

   WHBLogPrintf("--- Available Audio Devices (%d) ---", count);

   for (int i = 0; i < count; ++i)
   {
      // Get the name of the device at index 'i'
      const char *name = SDL_GetAudioDeviceName(i, 0);
      WHBLogPrintf("Device %d: %s", i, (name ? name : "Unknown"));
   }
   WHBLogPrintf("-----------------------------------");
}

int main(int argc, char *argv[])
{
   WHBLogCafeInit();
   WHBLogUdpInit();

   // 1. Init SDL (Video + Audio)
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
   {
      WHBLogPrintf("Init Error: %s", SDL_GetError());
      return 1;
   }

   // 2. Init SDL_ttf (Font system)
   if (TTF_Init() == -1)
   {
      WHBLogPrintf("TTF Init Error: %s", TTF_GetError());
      return 1;
   }

   // 3. Print Audio Devices to Console
   PrintAudioDevices();

   // --- SYNTH INIT ---
   static i32 harmonics[10] = {0};
   u32 phases[10] = {0};
   for (int i = 0; i < 10; i++)
   {
      int n = i + 1;
      harmonics[i] = FIXED_ONE / n;
      phases[i] = (n * n) * 0x08000000;
   }
   osc_build_wavetable(g_synth.wavetable, harmonics, phases, 10);

   for (int i = 0; i < 3; ++i)
   {
      voice_init(&g_synth.voices[i], g_synth.wavetable);

      i32 attack = env_ms_to_increment(500, SAMPLE_RATE);
      i32 decay = env_ms_to_increment(1000, SAMPLE_RATE);
      i32 sustain = env_sustain_to_hp(FIXED_ONE / 3);
      i32 release = env_ms_to_increment(300, SAMPLE_RATE);

      env_init(&g_synth.voices[i].env, attack, decay, sustain, release);

      filter_init(&g_synth.voices[i].filter, 500, SAMPLE_RATE);
      g_synth.voices[i].filter.q = FIXED_ONE;
   }

   g_synth.sample_counter = 0;
   g_synth.note_off_sample = SAMPLE_RATE * 3;
   g_synth.loop_end_sample = SAMPLE_RATE * 5;

   // Init Audio
   SDL_AudioSpec want, have;
   SDL_zero(want);
   want.freq = SAMPLE_RATE;
   want.format = AUDIO_S16SYS; // 16-bit signed (need to force system endian)
   want.channels = 1;          // Mono
   want.samples = 4096;        // Buffer size
   want.callback = AudioCallback;
   want.userdata = &g_synth;

   SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
   if (dev == 0)
   {
      WHBLogPrintf("Failed to open audio: %s", SDL_GetError());
   }
   else
   {
      WHBLogPrintf("Audio opened: %d Hz, %d channels, %d samples", have.freq, have.channels, have.samples);
      SDL_PauseAudioDevice(dev, 0); // Start playing
   }

   // 4. Setup Window & Renderer
   SDL_Window *window = SDL_CreateWindow("Synth Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
   SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

   // 5. Load a Font
   TTF_Font *font = TTF_OpenFont("/vol/content/arial.ttf", 24);
   if (!font)
   {
      WHBLogPrintf("Failed to load font! Make sure arial.ttf exists.");
      // Don't exit, just don't render text
   }

   // Color for the text (White)
   SDL_Color textColor = {255, 255, 255, 255};

   bool isRunning = true;
   SDL_Event event;
   int frameCount = 0;

   while (isRunning)
   {
      while (SDL_PollEvent(&event))
      {
         if (event.type == SDL_QUIT)
            isRunning = false;
      }

      // --- RENDER LOOP ---
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
      SDL_RenderClear(renderer);

      if (font)
      {
         // --- PREPARE TEXT ---
         frameCount++;
         std::string debugText = "Synth Playing... Frame: " + std::to_string(frameCount);
         if (g_synth.sample_counter < g_synth.note_off_sample)
         {
            debugText += " (Note ON)";
         }
         else
         {
            debugText += " (Note OFF)";
         }

         SDL_Surface *textSurface = TTF_RenderText_Solid(font, debugText.c_str(), textColor);
         if (textSurface)
         {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect;
            textRect.x = 20;
            textRect.y = 20;
            textRect.w = textSurface->w;
            textRect.h = textSurface->h;
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
         }
      }

      SDL_RenderPresent(renderer);
   }

   // Cleanup
   if (dev != 0)
      SDL_CloseAudioDevice(dev);
   if (font)
      TTF_CloseFont(font);
   SDL_DestroyRenderer(renderer);
   SDL_DestroyWindow(window);
   TTF_Quit();
   SDL_Quit();

   return 0;
}