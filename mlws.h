#pragma once

// Common definitions
typedef unsigned long long u64;
typedef signed long long i64;
typedef int i32;
typedef unsigned int u32;
typedef signed short i16;
typedef unsigned short u16;
typedef signed char i8;
typedef unsigned char u8;

#define FIXED_SHIFT 16
#define FIXED_ONE (1 << FIXED_SHIFT)
#define PI 205888 // 3.14159 << FIXED_SHIFT
#define TO_FIXED(a) ((a) << FIXED_SHIFT)
#define TO_INT(a) ((a) >> FIXED_SHIFT)

static inline i32 fixed_mul(i32 a, i32 b)
{
    return (i32)((long long)a * b >> FIXED_SHIFT);
}

#define WAVETABLE_SIZE 256
#define WAVETABLE_MASK (WAVETABLE_SIZE - 1)

static const i16 SINE_LUT[WAVETABLE_SIZE] = {
    0, 804, 1607, 2410, 3211, 4011, 4807, 5601,
    6392, 7179, 7961, 8739, 9511, 10278, 11039, 11792,
    12539, 13278, 14009, 14732, 15446, 16151, 16845, 17530,
    18204, 18867, 19519, 20159, 20787, 21402, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
    27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
    32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285,
    32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
    30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683,
    27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21402, 20787, 20159, 19519, 18867,
    18204, 17530, 16845, 16151, 15446, 14732, 14009, 13278,
    12539, 11792, 11039, 10278, 9511, 8739, 7961, 7179,
    6392, 5601, 4807, 4011, 3211, 2410, 1607, 804,
    0, -805, -1608, -2411, -3212, -4012, -4808, -5602,
    -6393, -7180, -7962, -8740, -9512, -10279, -11040, -11793,
    -12540, -13279, -14010, -14733, -15447, -16152, -16846, -17531,
    -18205, -18868, -19520, -20160, -20788, -21403, -22006, -22595,
    -23171, -23732, -24280, -24812, -25330, -25833, -26320, -26791,
    -27246, -27684, -28106, -28511, -28899, -29269, -29622, -29957,
    -30274, -30572, -30853, -31114, -31357, -31581, -31786, -31972,
    -32138, -32286, -32413, -32522, -32610, -32679, -32729, -32758,
    -32768, -32758, -32729, -32679, -32610, -32522, -32413, -32286,
    -32138, -31972, -31786, -31581, -31357, -31114, -30853, -30572,
    -30274, -29957, -29622, -29269, -28899, -28511, -28106, -27684,
    -27246, -26791, -26320, -25833, -25330, -24812, -24280, -23732,
    -23171, -22595, -22006, -21403, -20788, -20160, -19520, -18868,
    -18205, -17531, -16846, -16152, -15447, -14733, -14010, -13279,
    -12540, -11793, -11040, -10279, -9512, -8740, -7962, -7180,
    -6393, -5602, -4808, -4012, -3212, -2411, -1608, -805};

static inline i32 fixed_sin(u32 phase)
{
    u32 index = phase >> 24;       // top 8 bits for indexing
    u32 frac = phase & 0x00FFFFFF; // lower 24 bits for frac
    // lerp time!
    i16 p1 = SINE_LUT[index & WAVETABLE_MASK];
    i16 p2 = SINE_LUT[(index + 1) & WAVETABLE_MASK]; // wrap around
    i32 delta = (i32)p2 - (i32)p1;
    i32 interpolated = p1 + (i32)(((i64)delta * frac) >> 24);
    // Although 1 is 65536 in Q16, wavetable is i16, so max is 32767
    // return as is (effectively 0.5 - 0.5 range)
    return interpolated;
}

// Common end

// -----------------------------------------------------------------

// Osc state
typedef struct
{
    u32 phase;
    u32 increment;
} Osc;

static inline void osc_init(Osc *osc)
{
    osc->phase = 0;
    osc->increment = 0;
}

// Use i32 overflow to wrap phase
static inline void osc_set_frequency(Osc *osc, u32 frequency, u32 sample_rate)
{
    osc->increment = ((u64)frequency << 32) / sample_rate;
}

void osc_build_wavetable(i16 *target_buf, const i32 *harmonics, const u32 *phases, int count)
{
    // Measure max amplitude for normalization
    i32 max_amp = 1;

    for (int i = 0; i < WAVETABLE_SIZE; i++)
    {
        i32 sample = 0;
        u32 base_phase = ((u32)i) << 24; // top 8 bits for sin

        // add each harmonic
        for (int h = 0; h < count; h++)
        {
            if (h >= 128)
            {
                break; // over Nyquist freq
            }
            i32 amp = harmonics[h];
            if (amp == 0)
                continue; // no amp

            u32 harmonic_num = (u32)(h + 1);
            u32 phase_offset = phases ? phases[h] : 0;
            u32 harmonic_phase = (base_phase * harmonic_num) + phase_offset;
            i32 sine_amp = fixed_sin(harmonic_phase);
            sample += fixed_mul(amp, sine_amp);
        }

        if (sample < 0)
        {
            sample = -sample; // abs
        }
        if (sample > max_amp)
        {
            max_amp = sample;
        }
    }

    i32 scaler;

    scaler = ((i64)32760 << FIXED_SHIFT) / max_amp;

    // Second pass to apply norm
    // Alternatively, one can cache previous calculation in a u32 array
    // but recomputing is more memory efficient
    // besides, this is only done once at init
    for (int i = 0; i < WAVETABLE_SIZE; i++)
    {
        i32 sample = 0;
        u32 base_phase = ((u32)i) << 24; // top 8 bits for sin

        // add each harmonic
        for (int h = 0; h < count; h++)
        {
            i32 amp = harmonics[h];
            if (amp == 0)
                continue; // no amp

            u32 harmonic_num = (u32)(h + 1);
            u32 phase_offset = phases ? phases[h] : 0;
            u32 harmonic_phase = (base_phase * harmonic_num) + phase_offset;
            i32 sine_amp = fixed_sin(harmonic_phase);
            sample += fixed_mul(amp, sine_amp);
        }

        // apply norm
        i32 final_sample = fixed_mul(sample, scaler);

        // Final safety clamp
        if (final_sample > 32767)
            final_sample = 32767;
        else if (final_sample < -32768)
            final_sample = -32768;

        target_buf[i] = (i16)final_sample;
    }
}

static inline i16 osc_process(Osc *osc, const i16 *wavetable)
{
    osc->phase += osc->increment;
    u32 index = osc->phase >> 24;
    // extract frac for lerp
    u32 frac = osc->phase & 0x00FFFFFF;
    // get two samples
    i16 p1 = wavetable[index & WAVETABLE_MASK];
    i16 p2 = wavetable[(index + 1) & WAVETABLE_MASK]; // wrap around
    i32 delta = (i32)p2 - (i32)p1;
    // lerp
    delta = (i32)(((i64)delta * frac) >> 24);
    return p1 + delta;
}

// Envelope states
typedef enum
{
    ENV_IDLE = 0,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvState;

typedef struct
{
    EnvState state;
    i32 curr_level; // Q8.24

    i32 attack;
    i32 decay;
    i32 sustain_level;
    i32 release;
} Env;

// Env need higher decimal precision for smoothness
// Use Q8.24 format
#define ENV_FIXED_SHIFT 24
#define ENV_FIXED_ONE (1 << ENV_FIXED_SHIFT)

// Remember to use env_sustain_to_hp in user code for sustain level conversion
static inline void env_init(Env *env, i32 attack, i32 decay, i32 sustain_level, i32 release)
{
    env->state = ENV_IDLE;
    env->curr_level = 0;
    env->attack = attack;
    env->decay = decay;
    env->sustain_level = sustain_level;
    env->release = release;
}

static inline void env_note_on(Env *env)
{
    env->state = ENV_ATTACK;
}

static inline void env_note_off(Env *env)
{
    env->state = ENV_RELEASE;
}

static inline i32 env_ms_to_increment(u32 ms, u32 sr)
{
    if (ms == 0)
    {
        return ENV_FIXED_ONE; // instant
    }
    u64 target = (u64)ENV_FIXED_ONE;
    u64 total_samples = ((u64)ms * sr) / 1000;
    if (total_samples == 0)
    {
        total_samples = 1;
    }
    // return increment rate until 1.0 in high precision
    return (i32)(target / total_samples);
}

// sustain level is in Q16.16 -> Q8.24
static inline i32 env_sustain_to_hp(i32 sustain)
{
    return ((i32)sustain) << (ENV_FIXED_SHIFT - FIXED_SHIFT);
}

// env state machine impl
static inline i32 env_process(Env *env)
{
    switch (env->state)
    {
    case ENV_IDLE:
        // reset level
        env->curr_level = 0;
        break;
    case ENV_ATTACK:
        env->curr_level += env->attack;
        if (env->curr_level >= ENV_FIXED_ONE)
        {
            // reach max
            env->curr_level = ENV_FIXED_ONE;
            env->state = ENV_DECAY;
        }
        break;
    case ENV_DECAY:
        env->curr_level -= env->decay;
        if (env->curr_level <= env->sustain_level)
        {
            // reach sustain
            env->curr_level = env->sustain_level;
            env->state = ENV_SUSTAIN;
        }
        break;
    case ENV_SUSTAIN:
        // hold level
        env->curr_level = env->sustain_level;
        break;
    case ENV_RELEASE:
        env->curr_level -= env->release;
        if (env->curr_level <= 0)
        {
            // reach zero
            env->curr_level = 0;
            env->state = ENV_IDLE;
        }
        break;
    }

    // Convert to Q16.16 and apply logarithmic curve (x^2) for perceived loudness
    i32 linear = (i32)(env->curr_level >> (ENV_FIXED_SHIFT - FIXED_SHIFT));
    return fixed_mul(linear, linear);
}

// SVF filter impl
typedef struct
{
    i32 low;
    i32 band;

    i32 cutoff;
    i32 damping; // inverse proportional w/ q
} Filter;

static inline void filter_init(Filter *filter, int cutoff_freq, int sr)
{
    filter->low = 0;
    filter->band = 0;

    // calculate cutoff
    // F = 2*sin(PI * freq / sr)
    u32 phase = (u32)(((u64)cutoff_freq << 31) / sr); // phase = freq * 2^31 / sr
    filter->cutoff = fixed_sin(phase) * 2;            // 2 * sin(PI * freq / sr), in Q16
    // Clamping to ~0.8 in Q16.16 to avoid instability
    // This means its max cutoff will be around 5.6k @ 44.1khz
    // 2x sampling can double this limit but requires more CPU
    if (filter->cutoff > 52429)
    {
        filter->cutoff = 52429;
    }
}

// Chamberlin SVF impl
static inline i32 filter_process(Filter *f, i32 input)
{

    i32 high = input - f->low - fixed_mul(f->damping, f->band);

    f->band += fixed_mul(f->cutoff, high);

    f->low += fixed_mul(f->cutoff, f->band);

    return f->low;
}

// Voice = single note with osc, env, filter
typedef struct
{
    Osc osc;
    Env env;
    Filter filter;
    const i16 *wavetable;
} Voice;

static inline void voice_init(Voice *v, const i16 *wavetable)
{
    osc_init(&v->osc);
    env_init(&v->env, 0, 0, FIXED_ONE, 0);
    v->filter.low = 0;
    v->filter.band = 0;
    v->filter.cutoff = FIXED_ONE; // Open
    v->filter.damping = 0.5;
    v->wavetable = wavetable;
}

static inline void voice_note_on(Voice *v, u32 freq, u32 sample_rate)
{
    osc_set_frequency(&v->osc, freq, sample_rate);
    env_note_on(&v->env);
}

static inline void voice_note_off(Voice *v)
{
    env_note_off(&v->env);
}

static inline i32 voice_process(Voice *v)
{
    i32 osc_out = osc_process(&v->osc, v->wavetable);
    i32 env_amp = env_process(&v->env);
    i32 signal = fixed_mul(osc_out, env_amp);
    return filter_process(&v->filter, signal);
}

static inline void voice_process_block(Voice *v, i32 *out, int num_samples, int accumulate)
{
    for (int i = 0; i < num_samples; i++)
    {
        i32 sample = voice_process(v);
        if (accumulate)
            out[i] += sample;
        else
            out[i] = sample;
    }
}
