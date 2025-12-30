#include <stdio.h>
#include <stdlib.h>
#include "mlws.h"

#define SAMPLE_RATE 44100
#define DURATION_SEC 5

int main()
{
    static i16 wavetable[WAVETABLE_SIZE] = {0};
    static i32 harmonics[8] = {0};
    static u32 phases[8] = {0};

    for (int i = 0; i < 8; i++)
    {
        int n = i + 1;
        harmonics[i] = FIXED_ONE / n;

        // Shift phase by roughly n^2 * 11 degrees
        // 0x08000000 is approx 1/32 of a circle (~11 degrees)
        phases[i] = (n * n) * 0x08000000;
    }

    osc_build_wavetable(wavetable, harmonics, phases, 8);

    static Voice voice1, voice2, voice3;
    voice_init(&voice1, wavetable);
    voice_init(&voice2, wavetable);
    voice_init(&voice3, wavetable);

    // Envelope
    i32 attack = env_ms_to_increment(2000, SAMPLE_RATE);
    i32 decay = env_ms_to_increment(1000, SAMPLE_RATE);
    i32 sustain = env_sustain_to_hp(FIXED_ONE / 3);
    i32 release = env_ms_to_increment(300, SAMPLE_RATE);

    env_init(&voice1.env, attack, decay, sustain, release);
    env_init(&voice2.env, attack, decay, sustain, release);
    env_init(&voice3.env, attack, decay, sustain, release);

    filter_init(&voice1.filter, 500, SAMPLE_RATE);
    voice1.filter.q = FIXED_ONE;
    filter_init(&voice2.filter, 500, SAMPLE_RATE);
    voice2.filter.q = FIXED_ONE;
    filter_init(&voice3.filter, 500, SAMPLE_RATE);
    voice3.filter.q = FIXED_ONE;

    FILE *f = fopen("output.raw", "wb");
    if (!f)
    {
        perror("Failed to open output file");
        return 1;
    }

    int total_samples = SAMPLE_RATE * DURATION_SEC;
    int note_off_sample = SAMPLE_RATE * 3;

    voice_note_on(&voice1, 220, SAMPLE_RATE); // A3
    voice_note_on(&voice2, 277, SAMPLE_RATE); // C#4
    voice_note_on(&voice3, 329, SAMPLE_RATE); // E4

#define BLOCK_SIZE 256
    static i32 mix_buffer[BLOCK_SIZE];

    for (int i = 0; i < total_samples; i += BLOCK_SIZE)
    {
        int samples_to_process = BLOCK_SIZE;
        if (i + samples_to_process > total_samples)
            samples_to_process = total_samples - i;

        if (i <= note_off_sample && note_off_sample < i + samples_to_process)
        {
            int first_part = note_off_sample - i;
            if (first_part > 0)
            {
                voice_process_block(&voice1, mix_buffer, first_part, 0);
                voice_process_block(&voice2, mix_buffer, first_part, 1);
                voice_process_block(&voice3, mix_buffer, first_part, 1);
            }

            voice_note_off(&voice1);
            voice_note_off(&voice2);
            voice_note_off(&voice3);

            int second_part = samples_to_process - first_part;
            if (second_part > 0)
            {
                voice_process_block(&voice1, mix_buffer + first_part, second_part, 0);
                voice_process_block(&voice2, mix_buffer + first_part, second_part, 1);
                voice_process_block(&voice3, mix_buffer + first_part, second_part, 1);
            }
        }
        else
        {
            voice_process_block(&voice1, mix_buffer, samples_to_process, 0);
            voice_process_block(&voice2, mix_buffer, samples_to_process, 1);
            voice_process_block(&voice3, mix_buffer, samples_to_process, 1);
        }

        for (int k = 0; k < samples_to_process; k++)
        {
            i32 sample = mix_buffer[k];

            // normalize down to prevent clipping
            sample /= 3;

            // Clamp to i16
            if (sample > 32767)
                sample = 32767;
            if (sample < -32768)
                sample = -32768;

            i16 out = (i16)sample;
            fwrite(&out, sizeof(i16), 1, f);
        }
    }

    fclose(f);
    printf("Done. Written to output.raw\n");
    return 0;
}
