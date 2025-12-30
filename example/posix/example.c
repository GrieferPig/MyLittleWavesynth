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

    static Voice voices[3];
    for (int v = 0; v < 3; v++)
        voice_init(&voices[v], wavetable);

    // Envelope
    i32 attack = env_ms_to_increment(2000, SAMPLE_RATE);
    i32 decay = env_ms_to_increment(1000, SAMPLE_RATE);
    i32 sustain = env_sustain_to_hp(FIXED_ONE / 3);
    i32 release = env_ms_to_increment(300, SAMPLE_RATE);

    for (int v = 0; v < 3; v++)
    {
        env_init(&voices[v].env, attack, decay, sustain, release);
        filter_init(&voices[v].filter, 5000, SAMPLE_RATE);
        voices[v].filter.damping = FIXED_ONE;
    }

    FILE *f = fopen("output.raw", "wb");
    if (!f)
    {
        perror("Failed to open output file");
        return 1;
    }

    int total_samples = SAMPLE_RATE * DURATION_SEC;
    int note_off_sample = SAMPLE_RATE * 3;

    u32 freqs[3] = {220, 277, 329};
    for (int v = 0; v < 3; v++)
        voice_note_on(&voices[v], freqs[v], SAMPLE_RATE); // A3, C#4, E4

#define BLOCK_SIZE 256
    static i32 mix_buffer[BLOCK_SIZE];

    for (int i = 0; i < total_samples; i += BLOCK_SIZE)
    {
        int samples_to_process = BLOCK_SIZE;
        if (i + samples_to_process > total_samples)
            samples_to_process = total_samples - i;

        // clear buffer
        for (int k = 0; k < samples_to_process; k++)
            mix_buffer[k] = 0;

        if (i <= note_off_sample && note_off_sample < i + samples_to_process)
        {
            int first_part = note_off_sample - i;
            if (first_part > 0)
            {
                for (int v = 0; v < 3; v++)
                    voice_process_block(&voices[v], mix_buffer, first_part, v != 0);
            }

            for (int v = 0; v < 3; v++)
                voice_note_off(&voices[v]);

            int second_part = samples_to_process - first_part;
            if (second_part > 0)
            {
                for (int v = 0; v < 3; v++)
                    voice_process_block(&voices[v], mix_buffer + first_part, second_part, v != 0);
            }
        }
        else
        {
            for (int v = 0; v < 3; v++)
                voice_process_block(&voices[v], mix_buffer, samples_to_process, v != 0);
        }

        for (int k = 0; k < samples_to_process; k++)
        {
            i32 sample = mix_buffer[k] / 3; // normalize

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
