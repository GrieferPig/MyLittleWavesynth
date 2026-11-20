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

// -----------------------------------------------------------------
// Public unit types for semantic representation

typedef struct { u32 value; } mlws_hz_t;
typedef struct { u32 value; } mlws_ms_t;
typedef struct { u8 value; } mlws_percent_t;      // 0..100
typedef struct { i8 value; } mlws_bipolar_t;      // -100..100
typedef struct { i16 value; } mlws_cents_t;
typedef struct { i8 value; } mlws_semitones_t;

static inline mlws_hz_t Hz(u32 value)
{
    mlws_hz_t out = {value};
    return out;
}

static inline mlws_ms_t Ms(u32 value)
{
    mlws_ms_t out = {value};
    return out;
}

static inline mlws_percent_t Percent(u32 value)
{
    if (value > 100)
        value = 100;

    mlws_percent_t out = {(u8)value};
    return out;
}

static inline mlws_bipolar_t Bipolar(i32 value)
{
    if (value < -100)
        value = -100;
    else if (value > 100)
        value = 100;

    mlws_bipolar_t out = {(i8)value};
    return out;
}

static inline mlws_cents_t Cents(i32 value)
{
    if (value < -32768)
        value = -32768;
    else if (value > 32767)
        value = 32767;

    mlws_cents_t out = {(i16)value};
    return out;
}

static inline mlws_semitones_t Semitones(i32 value)
{
    if (value < -128)
        value = -128;
    else if (value > 127)
        value = 127;

    mlws_semitones_t out = {(i8)value};
    return out;
}

#define FIXED_SHIFT_Q16 16
#define FIXED_SHIFT_Q8 8
#define FIXED_ONE_Q16 (1 << FIXED_SHIFT_Q16)
#define FIXED_ONE_Q8 (1 << FIXED_SHIFT_Q8)
#define PI 205888 // 3.14159 << FIXED_SHIFT_Q16
#define TO_FIXED(a) ((a) << FIXED_SHIFT_Q16)
#define TO_INT(a) ((a) >> FIXED_SHIFT_Q16)

static inline i32 fixed_mul(i32 a, i32 b)
{
    return (i32)((long long)a * b >> FIXED_SHIFT_Q16);
}

// Translate public unit values to common internal fixed-point values.
static inline i32 mlws_percent_to_q16(mlws_percent_t percent)
{
    return ((i32)percent.value * FIXED_ONE_Q16) / 100;
}

static inline i32 mlws_bipolar_to_q16(mlws_bipolar_t amount)
{
    return ((i32)amount.value * FIXED_ONE_Q16) / 100;
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
static inline void osc_set_frequency(Osc *osc, mlws_hz_t frequency, mlws_hz_t sample_rate)
{
    if (sample_rate.value == 0)
    {
        osc->increment = 0;
        return;
    }

    osc->increment = ((u64)frequency.value << 32) / sample_rate.value;
}

static inline void osc_build_wavetable(i16 *target_buf, const i32 *harmonics, const u32 *phases, int count)
{
    if (!target_buf)
        return;

    if (!harmonics || count <= 0)
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++)
            target_buf[i] = 0;
        return;
    }
    // Measure max amplitude for normalization
    i32 max_amp = 1;

    int harmonic_count = count;

    // Limit harmonic count to half of wavetable size to avoid aliasing
    if (harmonic_count > WAVETABLE_SIZE / 2)
        harmonic_count = WAVETABLE_SIZE / 2;

    for (int i = 0; i < WAVETABLE_SIZE; i++)
    {
        i32 sample = 0;
        u32 base_phase = ((u32)i) << 24; // top 8 bits for sin

        // add each harmonic
        for (int h = 0; h < harmonic_count; h++)
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

    scaler = ((i64)32760 << FIXED_SHIFT_Q16) / max_amp;

    // Second pass to apply norm
    // Alternatively, one can cache previous calculation in a u32 array
    // but recomputing is more memory efficient
    // besides, this is only done once at init
    for (int i = 0; i < WAVETABLE_SIZE; i++)
    {
        i32 sample = 0;
        u32 base_phase = ((u32)i) << 24; // top 8 bits for sin

        // add each harmonic
        for (int h = 0; h < harmonic_count; h++)
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
    i32 curr_level; // Q8.24 amplitude, 0..1

    // Stage times are stored as sample counts so decay/release duration does
    // not accidentally depend on sustain/current level.
    u32 attack;
    u32 decay;
    i32 sustain_level;
    u32 release;

    // Interpolation state for the currently running stage.
    i32 stage_start;
    i32 stage_target;
    u32 stage_pos;
    u32 stage_samples;
} Env;

// Env need higher decimal precision for smoothness
// Use Q8.24 format
#define ENV_FIXED_SHIFT_Q16 24
#define ENV_FIXED_ONE_Q16 (1 << ENV_FIXED_SHIFT_Q16)

static inline u32 env_ms_to_samples(mlws_ms_t time, mlws_hz_t sample_rate)
{
    if (time.value == 0 || sample_rate.value == 0)
        return 0;

    u64 total_samples = ((u64)time.value * sample_rate.value + 999) / 1000;
    if (total_samples == 0)
        total_samples = 1;
    if (total_samples > 0xFFFFFFFFULL)
        total_samples = 0xFFFFFFFFULL;

    return (u32)total_samples;
}

static inline i32 env_percent_to_level(mlws_percent_t percent)
{
    return ((i32)percent.value * ENV_FIXED_ONE_Q16) / 100;
}

// Decay/release use an RC-like ease-out curve
static inline i32 env_rc_curve_q16(i32 t)
{
    if (t <= 0)
        return 0;
    if (t >= FIXED_ONE_Q16)
        return FIXED_ONE_Q16;

    i32 remaining = FIXED_ONE_Q16 - t;
    return FIXED_ONE_Q16 - fixed_mul(remaining, remaining);
}

static inline void env_begin_stage(Env *env, EnvState state, i32 target, u32 samples)
{
    env->state = state;
    env->stage_start = env->curr_level;
    env->stage_target = target;
    env->stage_pos = 0;
    env->stage_samples = samples;
}

static inline u32 env_attack_samples_from_level(const Env *env)
{
    if (env->attack == 0 || env->curr_level >= ENV_FIXED_ONE_Q16)
        return 0;
    if (env->curr_level <= 0)
        return env->attack;

    u32 remaining = (u32)(ENV_FIXED_ONE_Q16 - env->curr_level);
    u64 scaled = (u64)env->attack * remaining;
    scaled = (scaled + (u32)ENV_FIXED_ONE_Q16 - 1) / (u32)ENV_FIXED_ONE_Q16;
    if (scaled == 0)
        scaled = 1;
    if (scaled > 0xFFFFFFFFULL)
        scaled = 0xFFFFFFFFULL;
    return (u32)scaled;
}

static inline void env_set_attack(Env *env, mlws_ms_t attack, mlws_hz_t sample_rate)
{
    env->attack = env_ms_to_samples(attack, sample_rate);
}

static inline void env_set_decay(Env *env, mlws_ms_t decay, mlws_hz_t sample_rate)
{
    env->decay = env_ms_to_samples(decay, sample_rate);
}

static inline void env_set_sustain(Env *env, mlws_percent_t sustain)
{
    env->sustain_level = env_percent_to_level(sustain);

    if (env->state == ENV_SUSTAIN)
        env->curr_level = env->sustain_level;
    else if (env->state == ENV_DECAY)
        env_begin_stage(env, ENV_DECAY, env->sustain_level, env->decay);
}

static inline void env_set_release(Env *env, mlws_ms_t release, mlws_hz_t sample_rate)
{
    env->release = env_ms_to_samples(release, sample_rate);
}

static inline void env_init(
    Env *env,
    mlws_ms_t attack,
    mlws_ms_t decay,
    mlws_percent_t sustain,
    mlws_ms_t release,
    mlws_hz_t sample_rate)
{
    env->state = ENV_IDLE;
    env->curr_level = 0;
    env->stage_start = 0;
    env->stage_target = 0;
    env->stage_pos = 0;
    env->stage_samples = 0;

    env_set_attack(env, attack, sample_rate);
    env_set_decay(env, decay, sample_rate);
    env_set_sustain(env, sustain);
    env_set_release(env, release, sample_rate);
}

static inline void env_note_on(Env *env)
{
    env_begin_stage(
        env,
        ENV_ATTACK,
        ENV_FIXED_ONE_Q16,
        env_attack_samples_from_level(env));
}

static inline void env_note_off(Env *env)
{
    env_begin_stage(env, ENV_RELEASE, 0, env->release);
}

static inline void env_advance_stage(Env *env)
{
    if (env->state == ENV_ATTACK)
        env_begin_stage(env, ENV_DECAY, env->sustain_level, env->decay);
    else if (env->state == ENV_DECAY)
    {
        env->state = ENV_SUSTAIN;
        env->curr_level = env->sustain_level;
    }
    else if (env->state == ENV_RELEASE)
    {
        env->state = ENV_IDLE;
        env->curr_level = 0;
    }
}

// Envelope state machine. Output is Q16.16 linear amplitude.
static inline i32 env_process(Env *env)
{
    for (int transitions = 0; transitions < 3; transitions++)
    {
        if (env->state == ENV_IDLE)
        {
            env->curr_level = 0;
            return 0;
        }

        if (env->state == ENV_SUSTAIN)
        {
            env->curr_level = env->sustain_level;
            break;
        }

        if (env->stage_samples == 0)
        {
            env->curr_level = env->stage_target;
            env_advance_stage(env);
            continue;
        }

        if (env->stage_pos < env->stage_samples)
            env->stage_pos++;

        i32 t = (i32)(((u64)env->stage_pos << FIXED_SHIFT_Q16) / env->stage_samples);
        if (t > FIXED_ONE_Q16)
            t = FIXED_ONE_Q16;

        // Linear attack; RC-like decay/release.
        i32 curve = (env->state == ENV_ATTACK) ? t : env_rc_curve_q16(t);
        i64 delta = (i64)env->stage_target - env->stage_start;
        env->curr_level = env->stage_start + (i32)((delta * curve) >> FIXED_SHIFT_Q16);

        if (env->stage_pos >= env->stage_samples)
        {
            env->curr_level = env->stage_target;
            env_advance_stage(env);
        }
        break;
    }

    return (i32)(env->curr_level >> (ENV_FIXED_SHIFT_Q16 - FIXED_SHIFT_Q16));
}

// SVF filter impl
typedef struct
{
    i32 low;
    i32 band;

    i32 cutoff;
    i32 damping; // inverse proportional w/ q
} Filter;

static inline void filter_set_cutoff(Filter *filter, mlws_hz_t cutoff, mlws_hz_t sample_rate)
{
    if (sample_rate.value == 0)
{
        filter->cutoff = 0;
        return;
    }

    // Frequencies above Nyquist are not representable. Without this clamp,
    // values near/above sample_rate wrap the sine phase and can make the
    // cutoff fall back toward zero.
    u32 nyquist = sample_rate.value / 2;
    u32 cutoff_hz = cutoff.value;
    if (cutoff_hz > nyquist)
        cutoff_hz = nyquist;

    // F = 2*sin(PI * freq / sr)
    u32 phase = (u32)(((u64)cutoff_hz << 31) / sample_rate.value);
    filter->cutoff = fixed_sin(phase) * 2;

    // Clamp to ~0.8 in Q16.16 to keep this Chamberlin SVF stable.
    if (filter->cutoff > 52429)
        filter->cutoff = 52429;
    }

static inline void filter_set_resonance(Filter *filter, mlws_percent_t resonance)
{
    // 0% = no resonance,
    // 100% = maximum resonance.
    mlws_percent_t damping_percent = Percent(100 - resonance.value);
    filter->damping = mlws_percent_to_q16(damping_percent);
}

static inline void filter_init(
    Filter *filter,
    mlws_hz_t cutoff,
    mlws_percent_t resonance,
    mlws_hz_t sample_rate)
{
    filter->low = 0;
    filter->band = 0;

    filter_set_cutoff(filter, cutoff, sample_rate);
    filter_set_resonance(filter, resonance);
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
    u32 sample_rate;

    Osc osc;
    Env env;
    Filter filter;
    const i16 *wavetable;
} Voice;

static inline void voice_init(Voice *v, const i16 *wavetable, mlws_hz_t sample_rate)
{
    v->sample_rate = sample_rate.value;
    osc_init(&v->osc);

    env_init(
        &v->env,
        Ms(0),
        Ms(0),
        Percent(100),
        Ms(0),
        sample_rate);

    filter_init(
        &v->filter,
        Hz(5600),
        Percent(50),
        sample_rate);

    v->wavetable = wavetable;
}

static inline void voice_note_on(Voice *v, mlws_hz_t frequency)
{
    osc_set_frequency(&v->osc, frequency, Hz(v->sample_rate));
    env_note_on(&v->env);
}

static inline void voice_note_off(Voice *v)
{
    env_note_off(&v->env);
}

// Voice-level setters reuse the sample rate stored at initialization.
static inline void voice_set_attack(Voice *v, mlws_ms_t attack)
{
    env_set_attack(&v->env, attack, Hz(v->sample_rate));
}

static inline void voice_set_decay(Voice *v, mlws_ms_t decay)
{
    env_set_decay(&v->env, decay, Hz(v->sample_rate));
}

static inline void voice_set_sustain(Voice *v, mlws_percent_t sustain)
{
    env_set_sustain(&v->env, sustain);
}

static inline void voice_set_release(Voice *v, mlws_ms_t release)
{
    env_set_release(&v->env, release, Hz(v->sample_rate));
}

static inline void voice_set_envelope(
    Voice *v,
    mlws_ms_t attack,
    mlws_ms_t decay,
    mlws_percent_t sustain,
    mlws_ms_t release)
{
    env_set_attack(&v->env, attack, Hz(v->sample_rate));
    env_set_decay(&v->env, decay, Hz(v->sample_rate));
    env_set_sustain(&v->env, sustain);
    env_set_release(&v->env, release, Hz(v->sample_rate));
}

static inline void voice_set_cutoff(Voice *v, mlws_hz_t cutoff)
{
    filter_set_cutoff(&v->filter, cutoff, Hz(v->sample_rate));
}

static inline void voice_set_resonance(Voice *v, mlws_percent_t resonance)
{
    filter_set_resonance(&v->filter, resonance);
}

static inline void voice_set_filter(Voice *v, mlws_hz_t cutoff, mlws_percent_t resonance)
{
    filter_set_cutoff(&v->filter, cutoff, Hz(v->sample_rate));
    filter_set_resonance(&v->filter, resonance);
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
        {
            i64 mixed = (i64)out[i] + sample;
            if (mixed > 2147483647LL)
                mixed = 2147483647LL;
            else if (mixed < (-2147483647LL - 1LL))
                mixed = (-2147483647LL - 1LL);
            out[i] = (i32)mixed;
        }
        else
            out[i] = sample;
    }
}

// -----------------------------------------------------------------
// Polyphonic mixing helpers

#ifndef MLWS_MASTER_GAIN_Q16
#define MLWS_MASTER_GAIN_Q16 16384
#endif

static inline i16 mlws_mix_to_i16_i64(i64 sample)
{
    if (sample > 32767)
        return 32767;
    if (sample < -32768)
        return -32768;
        return (i16)sample;
}


static inline i16 mlws_soft_clip_i16(i32 sample)
{
    return mlws_mix_to_i16_i64(sample);
}

// Sum voices without reducing to i32 first. This wide bus is the fixed-point
// equivalent of the floating-point summing bus used by conventional synths.
static inline i64 voice_mix_sum_i64(Voice *voices, int voice_count)
{
    if (!voices || voice_count <= 0)
        return 0;

    i64 sum = 0;
    for (int i = 0; i < voice_count; i++)
        sum += (i64)voice_process(&voices[i]);

    return sum;
}

// Optional mixer object for callers that want a runtime-adjustable but still
// CONSTANT master gain. It never tracks active_voice_count.
typedef struct
{
    i32 gain_q16;
} VoiceMixer;

static inline void voice_mixer_init_gain(VoiceMixer *mixer, i32 gain_q16)
{
    if (!mixer)
        return;

    if (gain_q16 < 0)
        gain_q16 = 0;

    mixer->gain_q16 = gain_q16;
}

static inline void voice_mixer_init(VoiceMixer *mixer, int voice_capacity)
{
    (void)voice_capacity;
    voice_mixer_init_gain(mixer, MLWS_MASTER_GAIN_Q16);
}

static inline void voice_mixer_set_gain(VoiceMixer *mixer, i32 gain_q16)
{
    voice_mixer_init_gain(mixer, gain_q16);
}

static inline i16 voice_mixer_process(VoiceMixer *mixer, Voice *voices, int active_voice_count)
{
    if (!mixer || !voices || active_voice_count <= 0)
        return 0;

    i64 sum = voice_mix_sum_i64(voices, active_voice_count);
    i64 scaled = (sum * (i64)mixer->gain_q16) >> FIXED_SHIFT_Q16;
    return mlws_mix_to_i16_i64(scaled);
}

static inline void voice_mixer_process_block(
    VoiceMixer *mixer,
    Voice *voices,
    int active_voice_count,
    i16 *out,
    int num_samples)
{
    if (!out || num_samples <= 0)
        return;

    for (int i = 0; i < num_samples; i++)
        out[i] = voice_mixer_process(mixer, voices, active_voice_count);
}

static inline i16 voice_mix_process(Voice *voices, int voice_count)
{
    if (!voices || voice_count <= 0)
        return 0;

    i64 sum = voice_mix_sum_i64(voices, voice_count);
    i64 scaled = (sum * (i64)MLWS_MASTER_GAIN_Q16) >> FIXED_SHIFT_Q16;
    return mlws_mix_to_i16_i64(scaled);
}

static inline void voice_mix_process_block(Voice *voices, int voice_count, i16 *out, int num_samples)
{
    if (!out || num_samples <= 0)
        return;

    for (int i = 0; i < num_samples; i++)
        out[i] = voice_mix_process(voices, voice_count);
}
