#include "AudioManager.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <algorithm>

static const float kPI = 3.14159265f;

// â”€â”€â”€ WAV writer â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

static void writeWavHeader(std::vector<unsigned char>& buf,
                            int sampleRate, int numSamples, int channels = 1) {
    int dataSize = numSamples * channels * 2;
    int fileSize = 44 + dataSize - 8;
    auto W4 = [&](int o, int v)   { memcpy(buf.data()+o, &v, 4); };
    auto W2 = [&](int o, short v) { memcpy(buf.data()+o, &v, 2); };
    auto Ws = [&](int o, const char* s, int n) { memcpy(buf.data()+o, s, n); };
    Ws(0,"RIFF",4); W4(4,fileSize); Ws(8,"WAVE",4);
    Ws(12,"fmt ",4); W4(16,16); W2(20,1); W2(22,(short)channels);
    W4(24,sampleRate); W4(28,sampleRate*channels*2);
    W2(32,(short)(channels*2)); W2(34,16);
    Ws(36,"data",4); W4(40,dataSize);
}

static inline float clamp1(float v) { return v < -1.f ? -1.f : v > 1.f ? 1.f : v; }
static inline float rnd()  { return (float)(rand()%20001-10000)/10000.0f; }

// â”€â”€â”€ SFX synthesis â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

std::vector<unsigned char> AudioManager::createWavBuffer(
        float duration, float freq, bool noise, float pitchSweep,
        float attack, float release) {
    const int SR = 44100;
    const int N  = (int)(SR * duration);
    std::vector<unsigned char> buf(44 + N*2, 0);
    writeWavHeader(buf, SR, N);
    short* s = (short*)(buf.data()+44);

    int attackSamples  = (int)(attack  * SR);
    int releaseSamples = (int)(release * SR);

    for (int i = 0; i < N; ++i) {
        float t   = (float)i / SR;
        float f   = freq + pitchSweep * t;
        float env;
        if (i < attackSamples)
            env = (float)i / attackSamples;
        else if (i > N - releaseSamples)
            env = (float)(N - i) / releaseSamples;
        else
            env = 1.0f;

        float v;
        if (noise) {
            v = rnd() * 0.6f + std::sin(2*kPI*f*t) * 0.4f;
        } else {
            // Sawtooth + sine blend for richer tone
            float ph  = fmodf(f * t, 1.0f);
            float saw = 2.0f * ph - 1.0f;
            v = saw * 0.4f + std::sin(2*kPI*f*t) * 0.6f;
        }
        s[i] = (short)(clamp1(v * env) * 28000.0f);
    }
    return buf;
}

Sound AudioManager::generateTone(float dur, float freq, bool noise,
                                  float pitchSweep, float attack, float release) {
    auto buf = createWavBuffer(dur, freq, noise, pitchSweep, attack, release);
    Wave w   = LoadWaveFromMemory(".wav", buf.data(), (int)buf.size());
    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    return snd;
}

Sound AudioManager::generateComplex(float dur, int SR,
        std::vector<float> freqs, std::vector<float> amps,
        float pitchSweep, bool addNoise, float noiseAmt) {
    int N = (int)(SR * dur);
    std::vector<unsigned char> buf(44 + N*2, 0);
    writeWavHeader(buf, SR, N);
    short* s = (short*)(buf.data()+44);
    for (int i = 0; i < N; ++i) {
        float t   = (float)i / SR;
        float env = 1.0f - (float)i / N;
        // exponential decay envelope
        env = std::exp(-4.0f * (float)i / N);
        float v = 0.0f;
        for (int j = 0; j < (int)freqs.size(); ++j) {
            float f = freqs[j] + pitchSweep * t;
            float ph = fmodf(f * t, 1.0f);
            v += (2.0f * ph - 1.0f) * amps[j] * 0.5f;
            v += std::sin(2*kPI*f*t)  * amps[j] * 0.5f;
        }
        if (addNoise) v += rnd() * noiseAmt;
        s[i] = (short)(clamp1(v * env) * 28000.0f);
    }
    Wave w   = LoadWaveFromMemory(".wav", buf.data(), (int)buf.size());
    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    return snd;
}

// â”€â”€â”€ Per-sample SFX helper â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

static Sound sfxSynth(int SR, int N, const std::vector<float>& samples) {
    std::vector<unsigned char> buf(44 + N*2, 0);
    writeWavHeader(buf, SR, N);
    short* s = (short*)(buf.data()+44);
    for (int i = 0; i < N; ++i)
        s[i] = (short)(clamp1(samples[i]) * 30000.0f);
    Wave w = LoadWaveFromMemory(".wav", buf.data(), (int)buf.size());
    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    return snd;
}

// â”€â”€â”€ Zone music synthesis â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

// LA Ruins: Deep post-apocalyptic dark ambient â€” haunting melody, doom pulse, debris
// ═══════════════════════════════════════════════════════════════════════════
//  DARKNET — MOTOR DE COMPOSICAO (trilha oficial em camadas)
//  Tema principal compartilhado: progressao Am–F–C–G (la menor epico/synthwave)
//  com um motivo melodico reconhecivel que aparece em todas as zonas.
// ═══════════════════════════════════════════════════════════════════════════

static inline float midi2freq(int m) {
    return 440.0f * powf(2.0f, (m - 69) / 12.0f);
}

// Oscilador por forma de onda (fase em [0,1))
static inline float oscw(int wave, float ph) {
    ph -= floorf(ph);
    switch (wave) {
        case 1:  return 2.0f * ph - 1.0f;                 // serra
        case 2:  return ph < 0.5f ? 1.0f : -1.0f;         // quadrada
        case 3:  return 4.0f * fabsf(ph - 0.5f) - 1.0f;   // triangulo
        default: return sinf(2.0f * kPI * ph);            // seno
    }
}

// Estilo de cada zona (variacao do mesmo tema)

// ─────────────────────────────────────────────────────────────────────────────
// CADEIA DE DSP DA TRILHA
// A sintese anterior somava osciladores crus e mandava direto pro WAV com um
// unico tap de eco. Isso soa a chiptune de 1990: sem corpo nas graves, agudo
// aspero e nenhum espaco. Aqui entram as tres coisas que separam "bip de jogo"
// de trilha: FILTRO (tira o serrilhado do dente-de-serra), REVERB (poe a musica
// numa sala) e SATURACAO suave (cola tudo num corpo so).
// ─────────────────────────────────────────────────────────────────────────────

// Filtro passa-baixa de 2 polos (Butterworth simplificado). O corte varia com o
// tempo para dar movimento: e o "abrir e fechar" que faz a trilha respirar.
static void lowpass2(std::vector<float>& x, int SR, float startHz, float endHz) {
    float z1 = 0.0f, z2 = 0.0f;
    int   N  = (int)x.size();
    for (int i = 0; i < N; ++i) {
        float t  = (N > 1) ? (float)i / (float)(N - 1) : 0.0f;
        float fc = startHz + (endHz - startHz) * t;
        if (fc < 60.0f)  fc = 60.0f;
        if (fc > (float)SR * 0.45f) fc = (float)SR * 0.45f;
        float c  = 1.0f / tanf(3.14159265f * fc / (float)SR);
        float a0 = 1.0f / (1.0f + 1.4142f * c + c * c);
        float a1 = 2.0f * a0;
        float b1 = 2.0f * (1.0f - c * c) * a0;
        float b2 = (1.0f - 1.4142f * c + c * c) * a0;
        float in = x[i];
        float out = a0 * in + a1 * z1 + a0 * z2 - b1 * z1 - b2 * z2;
        z2 = z1; z1 = out;
        x[i] = out;
    }
}

// Reverb de Schroeder: 4 combs em paralelo + 2 allpass em serie. Barato e
// suficiente para dar tamanho de sala/caverna conforme `room`.
static void reverbSchroeder(std::vector<float>& x, int SR, float room, float wet) {
    if (wet <= 0.0f) return;
    const int N = (int)x.size();
    std::vector<float> out(N, 0.0f);
    const float combMs[4]  = { 29.7f, 37.1f, 41.1f, 43.7f };
    const float combGain[4] = { 0.76f, 0.74f, 0.72f, 0.70f };
    for (int c = 0; c < 4; ++c) {
        int   d = (int)(combMs[c] * 0.001f * SR * (0.6f + room * 0.9f));
        if (d < 1 || d >= N) continue;
        float g = combGain[c] * (0.55f + room * 0.42f);
        std::vector<float> buf(d, 0.0f);
        int   idx = 0;
        for (int i = 0; i < N; ++i) {
            float y = buf[idx];
            buf[idx] = x[i] + y * g;
            idx = (idx + 1) % d;
            out[i] += y * 0.25f;
        }
    }
    const float apMs[2] = { 5.0f, 1.7f };
    for (int a = 0; a < 2; ++a) {
        int d = (int)(apMs[a] * 0.001f * SR);
        if (d < 1 || d >= N) continue;
        std::vector<float> buf(d, 0.0f);
        int idx = 0;
        const float g = 0.7f;
        for (int i = 0; i < N; ++i) {
            float bufOut = buf[idx];
            float in     = out[i];
            float y      = -g * in + bufOut;
            buf[idx] = in + g * bufOut;
            idx = (idx + 1) % d;
            out[i] = y;
        }
    }
    for (int i = 0; i < N; ++i) x[i] = x[i] * (1.0f - wet * 0.5f) + out[i] * wet;
}

// Compressor de pico simples: segura os transientes do bumbo para a trilha nao
// "bombear" nem estourar quando varias camadas caem no mesmo tempo.
static void softCompress(std::vector<float>& x, float thresh, float ratio) {
    float env = 0.0f;
    for (size_t i = 0; i < x.size(); ++i) {
        float a = fabsf(x[i]);
        env = (a > env) ? (env * 0.30f + a * 0.70f) : (env * 0.9995f);
        if (env > thresh) {
            float over = env - thresh;
            float gain = (thresh + over / ratio) / env;
            x[i] *= gain;
        }
    }
}

struct TrackStyle {
    float bpm        = 92.0f;
    int   transpose  = 0;      // semitons
    float bassAmp    = 0.30f;
    float padAmp     = 0.09f;
    float arpAmp     = 0.10f;
    float leadAmp    = 0.16f;
    int   bassWave   = 1;      // serra
    int   arpWave    = 2;      // quadrada
    int   leadWave   = 0;      // seno
    bool  drums      = true;
    bool  hardDrums  = false;
    bool  arpOn      = true;
    bool  leadOn     = true;
    float drive      = 0.0f;   // distorcao (inferno)
    float echo       = 0.25f;  // mix de eco
    // Novos: e o que tira o som de "chiptune" e poe numa sala.
    float cutoffHz   = 5200.0f; // corte do passa-baixa no fim da cadeia
    float room       = 0.45f;   // tamanho da sala do reverb (0..1)
    float wet        = 0.26f;   // quanto de reverb entra na mistura
    float subAmp     = 0.16f;   // sub-grave senoidal sob o baixo
};

// Adiciona uma nota envelopada no mix (float), com anti-click (ataque/release)
static void addNote(std::vector<float>& mix, int SR, double startT, double durT,
                    float freq, float amp, int wave) {
    int s0 = (int)(startT * SR);
    int ns = (int)(durT * SR);
    if (ns <= 0) return;
    float atk = 0.006f;                 // 6ms
    float rel = (float)durT * 0.45f;
    for (int n = 0; n < ns; ++n) {
        int idx = s0 + n;
        if (idx < 0) continue;
        if (idx >= (int)mix.size()) break;
        float tt = (float)n / SR;
        float env;
        if (tt < atk)                 env = tt / atk;
        else if (tt > (float)durT - rel) env = ((float)durT - tt) / rel;
        else                          env = 1.0f;
        if (env < 0.0f) env = 0.0f;
        // UNISSONO: 3 vozes levemente desafinadas. Uma so sai fina e sintetica;
        // tres batendo entre si produzem o coro que da peso a trilha.
        float ph = freq * tt;
        float v  = oscw(wave, ph)
                 + oscw(wave, ph * 1.0035f) * 0.55f
                 + oscw(wave, ph * 0.9968f) * 0.55f;
        mix[idx] += v * 0.48f * amp * env;
    }
}

static void addKick(std::vector<float>& mix, int SR, double startT, float amp) {
    int s0 = (int)(startT * SR);
    int ns = (int)(0.18f * SR);
    for (int n = 0; n < ns; ++n) {
        int idx = s0 + n; if (idx < 0) continue; if (idx >= (int)mix.size()) break;
        float tt = (float)n / SR;
        float env = expf(-tt * 26.0f);
        float f   = 120.0f * expf(-tt * 30.0f) + 42.0f;   // pitch drop
        mix[idx] += sinf(2.0f * kPI * f * tt) * env * amp;
    }
}

static void addSnare(std::vector<float>& mix, int SR, double startT, float amp) {
    int s0 = (int)(startT * SR);
    int ns = (int)(0.16f * SR);
    float prev = 0.0f;
    for (int n = 0; n < ns; ++n) {
        int idx = s0 + n; if (idx < 0) continue; if (idx >= (int)mix.size()) break;
        float tt = (float)n / SR;
        float env = expf(-tt * 22.0f);
        float nz  = (float)(rand() % 2000 - 1000) / 1000.0f;
        prev = prev * 0.4f + nz * 0.6f;
        float tone = sinf(2.0f * kPI * 185.0f * tt) * 0.5f;
        mix[idx] += (prev * 0.7f + tone) * env * amp;
    }
}

static void addHat(std::vector<float>& mix, int SR, double startT, float amp) {
    int s0 = (int)(startT * SR);
    int ns = (int)(0.04f * SR);
    float prev = 0.0f;
    for (int n = 0; n < ns; ++n) {
        int idx = s0 + n; if (idx < 0) continue; if (idx >= (int)mix.size()) break;
        float tt = (float)n / SR;
        float env = expf(-tt * 120.0f);
        float nz  = (float)(rand() % 2000 - 1000) / 1000.0f;
        prev = nz - prev * 0.2f;          // high-pass cru
        mix[idx] += prev * env * amp;
    }
}

// Compoe a faixa completa (todas as camadas) usando o tema DARKNET.
static std::vector<short> composeTrack(int SR, int N, const TrackStyle& st) {
    std::vector<float> mix(N, 0.0f);

    const float beat = 60.0f / st.bpm;
    const float bar  = beat * 4.0f;
    const int   TR   = st.transpose;

    // Progressao de 4 compassos: Am – F – C – G  (raiz do baixo, MIDI)
    const int bassRoot[4] = { 45, 41, 48, 43 };            // A2 F2 C3 G2
    // Triades (notas de acorde) para pad/arpejo
    const int chord[4][3] = {
        { 57, 60, 64 },   // Am: A3 C4 E4
        { 53, 57, 60 },   // F : F3 A3 C4
        { 55, 60, 64 },   // C : G3 C4 E4
        { 55, 59, 62 },   // G : G3 B3 D4
    };
    // MOTIVO PRINCIPAL — 8 compassos x 4 notas (0 = pausa). La menor.
    const int theme[8][4] = {
        { 76, 72, 69, 72 },   // Am  E5 C5 A4 C5
        { 77, 72, 69, 65 },   // F   F5 C5 A4 F4
        { 76, 72, 67, 72 },   // C   E5 C5 G4 C5
        { 74, 71, 67, 71 },   // G   D5 B4 G4 B4
        { 81, 76, 72, 76 },   // Am  A5 E5 C5 E5  (segunda frase sobe)
        { 77, 74, 72, 69 },   // F   F5 D5 C5 A4
        { 76, 72, 71, 67 },   // C   E5 C5 B4 G4
        { 74, 67, 69,  0 },   // G   D5 G4 A4 (resolve)
    };
    // Baixo: 8 colcheias por compasso (offset sobre a raiz)
    const int bassPat[8] = { 0, 0, 0, 7, 0, 0, 12, 7 };

    double loopBars = 8.0;
    double loopLen  = loopBars * bar;
    double total    = (double)N / SR;

    for (double base = 0.0; base < total; base += loopLen) {
        for (int b = 0; b < 8; ++b) {
            int prog = b % 4;
            double barT = base + b * bar;

            // ── BAIXO (colcheias) ──
            for (int e = 0; e < 8; ++e) {
                int note = bassRoot[prog] + bassPat[e] + TR;
                addNote(mix, SR, barT + e * (beat * 0.5), beat * 0.48,
                        midi2freq(note), st.bassAmp, st.bassWave);
            }

            // ── PADS (acorde sustentado por compasso) ──
            for (int c = 0; c < 3; ++c) {
                addNote(mix, SR, barT, bar * 0.98,
                        midi2freq(chord[prog][c] + TR), st.padAmp, 0);
            }

            // ── ARPEJO (16 semicolcheias subindo) ──
            if (st.arpOn) {
                const int seq[4] = { 0, 1, 2, 1 };
                for (int k = 0; k < 16; ++k) {
                    int ti  = seq[k % 4];
                    int oct = (k >= 8) ? 12 : 0;
                    int note = chord[prog][ti] + 12 + oct + TR;   // oitava acima
                    float a  = st.arpAmp * ((k % 4 == 0) ? 1.0f : 0.7f);
                    addNote(mix, SR, barT + k * (beat * 0.25), beat * 0.22,
                            midi2freq(note), a, st.arpWave);
                }
            }

            // ── TEMA PRINCIPAL (melodia, seminimas) ──
            if (st.leadOn) {
                for (int q = 0; q < 4; ++q) {
                    int note = theme[b][q];
                    if (note == 0) continue;
                    addNote(mix, SR, barT + q * beat, beat * 0.92,
                            midi2freq(note + TR), st.leadAmp, st.leadWave);
                    // brilho de oitava sutil
                    addNote(mix, SR, barT + q * beat, beat * 0.5,
                            midi2freq(note + 12 + TR), st.leadAmp * 0.25f, 0);
                }
            }

            // ── PERCUSSAO ──
            if (st.drums) {
                float kAmp = st.hardDrums ? 0.62f : 0.42f;
                float sAmp = st.hardDrums ? 0.40f : 0.28f;
                float hAmp = st.hardDrums ? 0.16f : 0.11f;
                addKick(mix, SR, barT + 0 * beat, kAmp);
                addKick(mix, SR, barT + 2 * beat, kAmp);
                if (st.hardDrums) addKick(mix, SR, barT + 2.75 * beat, kAmp * 0.7f);
                addSnare(mix, SR, barT + 1 * beat, sAmp);
                addSnare(mix, SR, barT + 3 * beat, sAmp);
                for (int h = 0; h < 8; ++h)
                    addHat(mix, SR, barT + h * (beat * 0.5),
                           hAmp * ((h % 2) ? 1.0f : 0.6f));
            }
        }
    }

    // ── Cadeia final: sub-grave -> compressor -> filtro -> reverb -> saturacao ──
    // A ordem importa: filtrar DEPOIS de comprimir mantem o ataque do bumbo, e o
    // reverb entra depois do filtro para nao devolver o agudo que acabou de sair.
    if (st.subAmp > 0.0f) {
        // sub senoidal seguindo o bumbo: e o que faz a trilha ter fundo em
        // caixas de som de verdade, nao so no fone.
        double barLen = (60.0 / st.bpm) * 4.0;
        for (double t0 = 0.0; t0 < (double)N / SR; t0 += barLen) {
            addNote(mix, SR, t0,               barLen * 0.45, 55.0f, st.subAmp, 0);
            addNote(mix, SR, t0 + barLen * 0.5, barLen * 0.35, 55.0f, st.subAmp * 0.7f, 0);
        }
    }
    softCompress(mix, 0.72f, 3.5f);
    lowpass2(mix, SR, st.cutoffHz * 0.72f, st.cutoffHz);   // abre ao longo da faixa
    reverbSchroeder(mix, SR, st.room, st.wet);

    std::vector<short> s(N, 0);
    for (int i = 0; i < N; ++i) {
        float v = mix[i];
        if (st.drive > 0.0f) v = tanhf(v * (1.0f + st.drive * 3.0f));
        else                 v = tanhf(v * 1.05f);   // saturacao leve = cola
        s[i] = (short)(clamp1(v) * 26000.0f);
    }

    // Eco estereo-falso para profundidade
    if (st.echo > 0.0f) {
        int e1 = (int)(0.21f * SR);
        int e2 = (int)(0.37f * SR);
        for (int i = e1; i < N; ++i) {
            int mixed = (int)s[i] + (int)(s[i - e1] * st.echo);
            if (i >= e2) mixed += (int)(s[i - e2] * st.echo * 0.5f);
            s[i] = (short)std::max(-32767, std::min(32767, mixed));
        }
    }
    return s;
}

std::vector<short> AudioManager::synthLARuins(int SR, int N) {
    // Ruinas de Avalon — synthwave desolado, batida media
    TrackStyle st; st.bpm = 84.0f; st.hardDrums = false;
    // PERFIL ACUSTICO — cidade aberta e morta: brilho medio, sala grande
    st.cutoffHz = 4200.0f; st.room = 0.62f; st.wet = 0.30f; st.subAmp = 0.18f;
    st.bassWave = 1; st.arpWave = 2; st.leadWave = 0; st.echo = 0.28f;
    // LARuins: cidade morta ao ar livre — brilho medio, sala grande
    st.cutoffHz = 4200.0f; st.room = 0.62f; st.wet = 0.30f; st.subAmp = 0.18f;
    return composeTrack(SR, N, st);

    std::vector<short> s(N, 0);
    float bpm  = 68.0f;
    float beat = 60.0f / bpm;

    // Chord progression: Am(55) â†’ G(49) â†’ Em(41.2) â†’ F(43.65) â€” low register
    float roots[4] = {55.0f, 49.0f, 41.2f, 43.65f};
    float chordLen  = beat * 6.0f; // 6 beats per chord change (slower, more tension)

    // Haunting minor pentatonic melody â€” slow moving notes (Am pent: A C D E G)
    float melNotes[8] = {220.0f, 261.6f, 293.7f, 329.6f, 392.0f,
                         349.2f, 293.7f, 261.6f};
    float melPeriod = beat * 16.0f; // full melody = 16 beats

    for (int i = 0; i < N; ++i) {
        float t   = (float)i / SR;
        float v   = 0.0f;
        int   ci  = (int)(t / chordLen) % 4;
        float rt  = roots[ci];

        // Deep sub-drone: pulsing breath of the apocalypse
        float breathMod = 0.85f + 0.15f * std::sin(2*kPI*0.055f*t + 0.8f);
        v += std::sin(2*kPI * rt * t) * 0.38f * breathMod;
        v += std::sin(2*kPI * rt * 0.5f * t) * 0.28f * breathMod;

        // Mid-bass pulse on every 2 beats
        {
            float p2  = fmodf(t, beat * 2.0f);
            float env = std::exp(-p2 * 3.5f);
            float ph  = fmodf(rt * 2.0f * t, 1.0f);
            v += (2.0f*ph-1.0f) * env * 0.18f;
        }

        // Atmospheric string-pad: 4 detuned layers
        float padBreath = 0.08f * (0.4f + 0.6f * std::sin(2*kPI*0.12f*t + 1.2f));
        for (int k = 0; k < 4; ++k) {
            float detune = -1.5f + k * 1.0f; // Hz offset
            float ph = fmodf((rt*2.0f + detune)*t, 1.0f);
            v += (2.0f*ph-1.0f) * padBreath * (k==0||k==3 ? 0.5f : 0.7f);
        }
        // 5th harmonic layer
        v += std::sin(2*kPI * rt * 3.0f * t) * padBreath * 0.55f;

        // Haunting lead melody â€” slow, human, eerily beautiful
        {
            float mp   = fmodf(t, melPeriod);
            int   mi   = (int)(mp / (beat * 2.0f)) % 8;
            float moff = fmodf(mp, beat * 2.0f);
            float mEnv = moff < (beat*0.1f) ? moff/(beat*0.1f) : std::exp(-moff * 1.2f);
            float melF = melNotes[mi];
            v += std::sin(2*kPI * melF * t) * mEnv * 0.14f;
            // slight octave shimmer
            v += std::sin(2*kPI * melF * 2.0f * t) * mEnv * 0.04f;
        }

        // Sparse distant metallic percussion (debris, crumbling concrete)
        {
            // Clang every 7 beats
            float pc7 = fmodf(t, beat * 7.0f);
            float e7  = std::exp(-pc7 * 5.0f);
            v += std::sin(2*kPI * 230.0f * t) * e7 * 0.10f;
            v += rnd() * e7 * 0.07f;
        }
        {
            // Rumble thud every 3.5 beats (asymmetric, unnerving)
            float pt  = fmodf(t, beat * 3.5f);
            float et  = std::exp(-pt * 40.0f);
            float kf  = std::max(20.0f, 62.0f - pt * 85.0f);
            v += std::sin(2*kPI * kf * t) * et * 0.32f;
            v += rnd() * et * 0.08f;
        }

        // Wind and rain â€” layered noise, slow swell
        float windBr = 0.5f + 0.5f * std::sin(2*kPI*0.07f*t + 0.5f);
        float rainBr = 0.3f + 0.7f * std::sin(2*kPI*0.14f*t);
        v += rnd() * 0.022f * windBr;       // wind
        v += rnd() * 0.012f * rainBr;       // rain

        // Eerie high shimmer â€” distant siren echo
        float shimmer = 0.3f + 0.7f * std::sin(2*kPI*0.28f*t);
        float shimF   = 880.0f + 40.0f * std::sin(2*kPI*0.06f*t);
        v += std::sin(2*kPI*shimF*t) * 0.022f * shimmer;

        // Glass/debris tinkle (random sparse high hits)
        {
            float pg = fmodf(t, beat * 11.0f);
            float eg = std::exp(-pg * 18.0f);
            v += std::sin(2*kPI * 1200.0f * t) * eg * 0.05f;
        }

        v = clamp1(v);
        s[i] = (short)(v * 27000.0f);
    }

    // Double echo: 185ms primary + 340ms secondary (cavern reverb feel)
    int echo1 = (int)(0.185f * SR);
    int echo2 = (int)(0.340f * SR);
    for (int i = echo1; i < N; ++i) {
        int mixed = (int)s[i] + (int)(s[i-echo1] * 0.30f);
        if (i >= echo2) mixed += (int)(s[i-echo2] * 0.14f);
        s[i] = (short)std::max(-32767, std::min(32767, mixed));
    }
    return s;
}

// Bunker: Military war march â€” heavy boots, brass fanfare, combat urgency
std::vector<short> AudioManager::synthBunker(int SR, int N) {
    // Bunker NEXUS — marcha de combate, bateria pesada
    { TrackStyle st; st.bpm = 124.0f; st.hardDrums = true; st.drive = 0.10f;
    // PERFIL ACUSTICO — concreto fechado: abafado, grave pesado, pouco espaco
    st.cutoffHz = 3000.0f; st.room = 0.30f; st.wet = 0.22f; st.subAmp = 0.24f;
      st.bassWave = 1; st.arpWave = 2; st.leadWave = 1; st.echo = 0.20f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    float bpm  = 118.0f;
    float beat = 60.0f / bpm;

    // Bass riff: 8-note military march pattern (D minor feel)
    float bassNotes[8] = {73.4f, 87.3f, 73.4f, 65.4f, 82.4f, 87.3f, 77.8f, 73.4f};
    // Brass chord tones per section (changes every 8 beats)
    float brassRoots[4] = {146.8f, 130.8f, 164.8f, 123.5f};

    for (int i = 0; i < N; ++i) {
        float t = (float)i / SR;
        float v = 0.0f;
        int   bi = (int)(t / (beat * 8.0f)) % 4;

        // Driving bass riff â€” punchy sawtooth with gritty tone
        {
            int   ni  = (int)(t / (beat*0.5f)) % 8;
            float nOff= fmodf(t, beat*0.5f);
            float env = std::exp(-nOff * 7.0f) * (0.8f + 0.2f * std::sin(2*kPI*nOff*4));
            float f   = bassNotes[ni];
            float ph  = fmodf(f*t, 1.0f);
            float saw = 2.0f*ph - 1.0f;
            v += saw * env * 0.32f;
            // Octave layer
            v += std::sin(2*kPI*f*2*t) * env * 0.08f;
        }

        // Heavy STOMP kick on 1 and 3 (deep military footfall)
        {
            float p1 = fmodf(t, beat*2.0f);
            float p3 = fmodf(t - beat, beat*2.0f);
            float e1 = std::exp(-p1 * 28.0f);
            float e3 = std::exp(-p3 * 28.0f);
            float kf = 75.0f - p1 * 180.0f;
            v += std::sin(2*kPI * kf * t) * (e1+e3) * 0.38f;
            v += rnd() * (e1+e3) * 0.12f;
        }

        // Military snare on 2 and 4 â€” crisp, metallic
        {
            float ps = fmodf(t - beat, beat*2.0f);
            float es = std::max(0.0f, 1.0f - ps/0.07f);
            float es2= std::max(0.0f, 1.0f - ps/0.20f);
            v += rnd() * es  * 0.28f;
            v += std::sin(2*kPI*250.0f*t) * es2 * 0.10f; // snare body tone
        }

        // Hi-hat: tight 16ths with open accent on beats 1 & 3
        {
            float ph16 = fmodf(t, beat*0.25f);
            float eh   = std::max(0.0f, 1.0f - ph16/0.025f);
            float isAccent = (fmodf(t, beat) < 0.02f) ? 2.5f : 1.0f;
            v += rnd() * eh * 0.065f * isAccent;
        }

        // Brass stab every 4 beats â€” triumphant fanfare
        {
            float pos = fmodf(t, beat*4.0f);
            float env = std::max(0.0f, 0.65f - pos/0.5f);
            float root= brassRoots[bi];
            v += std::sin(2*kPI*root*t)       * env * 0.20f;
            v += std::sin(2*kPI*root*1.25f*t) * env * 0.12f; // major 3rd
            v += std::sin(2*kPI*root*1.5f*t)  * env * 0.10f; // perfect 5th
            v += rnd() * env * 0.04f; // brass edge
        }

        // Rising tension pad underneath
        float padEnv = 0.05f * (0.5f + 0.5f * std::sin(2*kPI*0.2f*t));
        v += std::sin(2*kPI*brassRoots[bi]*0.5f*t) * padEnv;
        v += std::sin(2*kPI*brassRoots[bi]*0.75f*t) * padEnv * 0.7f;

        // Sub bass rumble
        v += std::sin(2*kPI*40.0f*t) * 0.14f;

        v = clamp1(v);
        s[i] = (short)(v * 26500.0f);
    }

    // Short slap-back echo (60ms at 20%) for room feel
    int echo1 = (int)(0.06f * SR);
    for (int i = echo1; i < N; ++i) {
        int mixed = (int)s[i] + (int)(s[i-echo1] * 0.20f);
        s[i] = (short)std::max(-32767, std::min(32767, mixed));
    }
    return s;
}

// Kronos Forge: Brutal industrial metal â€” distorted riff, machine percussion, grinding
std::vector<short> AudioManager::synthFactory(int SR, int N) {
    // Kronos Forge — industrial distorcido
    { TrackStyle st; st.bpm = 134.0f; st.hardDrums = true; st.drive = 0.40f;
    // PERFIL ACUSTICO — forja industrial: metalico e seco, agudo aberto
    st.cutoffHz = 6200.0f; st.room = 0.40f; st.wet = 0.20f; st.subAmp = 0.22f;
      st.bassWave = 1; st.arpWave = 1; st.leadWave = 1; st.echo = 0.18f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    float bpm  = 155.0f;
    float beat = 60.0f / bpm;

    // Power chord riff: E5 power chords with palm-mute feel
    // Pattern: E-E-G-E-A-E-B-E (metal riff)
    float riffNotes[8] = {82.4f, 82.4f, 98.0f, 82.4f, 110.0f, 82.4f, 123.5f, 82.4f};
    float riffLen[8]   = {0.5f,  0.25f, 0.5f,  0.25f, 0.5f,   0.25f, 0.5f,   0.25f};
    float riffAcc[8]   = {1.0f,  0.5f,  1.0f,  0.5f,  1.0f,   0.5f,  1.0f,   0.5f };

    float riffPattern = 0.0f;
    for (int k = 0; k < 8; ++k) riffPattern += riffLen[k] * beat;

    for (int i = 0; i < N; ++i) {
        float t = (float)i / SR;
        float v = 0.0f;

        // Distorted power chord riff â€” heavily clipped sawtooth
        {
            float rp = fmodf(t, riffPattern);
            float acc = 0.0f;
            int ni = 0;
            for (int k = 0; k < 8; ++k) {
                float dur = riffLen[k] * beat;
                if (rp < acc + dur) { ni = k; break; }
                acc += dur;
            }
            float nOff = fmodf(rp - acc, riffLen[ni]*beat);
            float env  = std::exp(-nOff * 5.0f) * riffAcc[ni];
            float f    = riffNotes[ni];
            float ph   = fmodf(f*t, 1.0f);
            float saw  = 2.0f*ph - 1.0f;
            // Extreme distortion (waveshaping)
            float dist = clamp1(saw * 6.0f);
            v += dist * env * 0.25f;
            // Power chord 5th
            float ph5  = fmodf(f*1.5f*t, 1.0f);
            v += clamp1((2.0f*ph5-1.0f) * 5.0f) * env * 0.14f;
        }

        // Double-kick pattern: 1/8 note kicks with strong accent on 1 and 3
        {
            float pos8 = fmodf(t, beat*0.5f);
            float env8 = std::exp(-pos8 * 55.0f);
            bool  isStrong = (fmodf(t, beat*2.0f) < 0.02f || fmodf(t - beat, beat*2.0f) < 0.02f);
            float kMult = isStrong ? 1.6f : 0.9f;
            float kickF = 88.0f - pos8 * 250.0f;
            v += std::sin(2*kPI * kickF * t) * env8 * 0.35f * kMult;
            v += rnd() * env8 * 0.10f * kMult;
        }

        // Metal snare on 2 and 4 â€” extra crunch
        {
            float ps = fmodf(t - beat, beat*2.0f);
            float es = std::max(0.0f, 1.0f - ps/0.06f);
            v += rnd() * es * 0.30f;
            v += std::sin(2*kPI*200.0f*t) * es * 0.12f;
            v += std::sin(2*kPI*350.0f*t) * es * 0.06f;
        }

        // Fast 16th hi-hats with open on beat
        {
            float ph16 = fmodf(t, beat*0.25f);
            float eh   = std::max(0.0f, 1.0f - ph16/0.018f);
            float isOpen = (fmodf(t, beat) < 0.04f) ? 4.0f : 1.0f;
            v += rnd() * eh * 0.06f * isOpen;
        }

        // Industrial metal clang (every 2 beats, random pitch variation)
        {
            float pos = fmodf(t, beat*2.0f);
            float env = std::exp(-pos * 7.0f);
            v += std::sin(2*kPI*440.0f*t) * env * 0.10f;
            v += std::sin(2*kPI*550.0f*t) * env * 0.06f;
            v += rnd() * env * 0.07f;
        }

        // Grinding machine drone underneath
        float grindF = 55.0f + 5.0f*std::sin(2*kPI*0.5f*t);
        v += std::sin(2*kPI*grindF*t) * 0.12f;

        // Machine hiss
        v += rnd() * 0.015f;

        v = clamp1(v);
        s[i] = (short)(v * 26000.0f);
    }

    // Tight room reverb simulation (40ms at 22%)
    int echo1 = (int)(0.04f * SR);
    int echo2 = (int)(0.09f * SR);
    for (int i = echo2; i < N; ++i) {
        int mixed = (int)s[i]
                  + (int)(s[i-echo1] * 0.22f)
                  + (int)(s[i-echo2] * 0.10f);
        s[i] = (short)std::max(-32767, std::min(32767, mixed));
    }
    return s;
}

// Core Facility: Epic boss rush â€” orchestral synth, battle arpeggio, war drums, alarm
std::vector<short> AudioManager::synthCore(int SR, int N) {
    // Nucleo KRONOS — epico e tenso, tema em destaque
    { TrackStyle st; st.bpm = 104.0f; st.hardDrums = true;
    // PERFIL ACUSTICO — nucleo alienigena: cristalino, cauda longa
    st.cutoffHz = 7000.0f; st.room = 0.75f; st.wet = 0.38f; st.subAmp = 0.20f;
      st.bassWave = 1; st.arpWave = 2; st.leadWave = 1;
      st.leadAmp = 0.18f; st.padAmp = 0.11f; st.echo = 0.35f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    float bpm  = 145.0f;
    float beat = 60.0f / bpm;

    // Heroic minor arpeggio (Am pentatonic, 2 octaves)
    float arpNotes[16] = {
        220.0f,261.6f,329.6f,392.0f,440.0f,392.0f,329.6f,261.6f,
        220.0f,196.0f,174.6f,196.0f,220.0f,261.6f,294.0f,329.6f
    };
    // Chord roots that cycle every 8 beats: Am â†’ F â†’ C â†’ G
    float chordRoots[4] = {55.0f, 87.3f, 65.4f, 98.0f};
    float chordLen = beat * 8.0f;

    for (int i = 0; i < N; ++i) {
        float t   = (float)i / SR;
        float v   = 0.0f;
        int   ci  = (int)(t / chordLen) % 4;
        float cr  = chordRoots[ci];

        // Epic arpeggiated lead â€” fast 32nd note runs
        {
            int   ni  = (int)(t / (beat*0.25f)) % 16;
            float nOff= fmodf(t, beat*0.25f);
            float env = std::exp(-nOff * 14.0f);
            float f   = arpNotes[ni];
            v += std::sin(2*kPI*f*t)      * env * 0.18f;
            v += std::sin(2*kPI*f*2.0f*t) * env * 0.06f;
            // Slight detune for chorus shimmer
            v += std::sin(2*kPI*f*1.003f*t) * env * 0.05f;
        }

        // Chord-based sub-bass pulse on every beat
        {
            float pos = fmodf(t, beat);
            float env = std::exp(-pos * 18.0f);
            v += std::sin(2*kPI * cr * t) * env * 0.38f;
            v += rnd() * env * 0.06f;
        }

        // Epic "string ensemble" pad â€” detuned saw layers
        {
            float pEnv = 0.09f * (0.55f + 0.45f * std::sin(2*kPI*0.45f*t));
            for (int k = 0; k < 4; ++k) {
                float df  = -3.0f + k*2.0f;
                float ph  = fmodf((cr*2.0f + df)*t, 1.0f);
                v += (2.0f*ph-1.0f) * pEnv;
                // 5th harmonic layer
                float ph5 = fmodf((cr*3.0f + df)*t, 1.0f);
                v += (2.0f*ph5-1.0f) * pEnv * 0.6f;
            }
        }

        // Thunderous war kick on 1, 2, 3, 4 with accent on 1
        {
            float p = fmodf(t, beat);
            float isDownbeat = (fmodf(t, beat*4.0f) < 0.02f) ? 1.5f : 1.0f;
            float env = std::exp(-p * 32.0f) * isDownbeat;
            float kickF = 90.0f - p*200.0f;
            v += std::sin(2*kPI * kickF * t) * env * 0.36f;
            v += rnd() * env * 0.10f;
        }

        // Powerful snare on beats 2 and 4
        {
            float ps = fmodf(t - beat, beat*2.0f);
            float es = std::max(0.0f, 1.0f - ps/0.065f);
            v += rnd() * es * 0.32f;
            v += std::sin(2*kPI*180.0f*t) * es * 0.14f;
        }

        // Rapid hi-hat 32nds
        {
            float ph32 = fmodf(t, beat*0.125f);
            float eh   = std::max(0.0f, 1.0f - ph32/0.015f);
            v += rnd() * eh * 0.050f;
        }

        // KRONOS alarm sweep (ascending danger signal)
        {
            float sweep = 600.0f + 500.0f * std::sin(2*kPI*0.4f*t);
            v += std::sin(2*kPI*sweep*t) * 0.035f * (0.4f + 0.6f * std::abs(std::sin(2*kPI*2.0f*t)));
        }

        // Electronic glitch hits every 3 beats
        {
            float pos = fmodf(t, beat*3.0f);
            float env = std::exp(-pos * 20.0f);
            v += std::sin(2*kPI*1200.0f*t) * env * 0.06f;
            v += rnd() * env * 0.05f;
        }

        v = clamp1(v);
        s[i] = (short)(v * 26500.0f);
    }

    // Rich reverb: two echoes for epic hall sound
    int echo1 = (int)(0.08f * SR);
    int echo2 = (int)(0.20f * SR);
    for (int i = echo2; i < N; ++i) {
        int mixed = (int)s[i]
                  + (int)(s[i-echo1] * 0.28f)
                  + (int)(s[i-echo2] * 0.14f);
        s[i] = (short)std::max(-32767, std::min(32767, mixed));
    }
    return s;
}

// Main menu theme: epic cinematic â€” KRONOS-style sweeping intro
std::vector<short> AudioManager::synthMenu(int SR, int N) {
    // ★ TEMA PRINCIPAL OFICIAL DO DARKNET ★ — synthwave epico Am–F–C–G
    { TrackStyle st; st.bpm = 92.0f; st.hardDrums = false;
    // PERFIL ACUSTICO — menu: espacoso e limpo
    st.cutoffHz = 5200.0f; st.room = 0.68f; st.wet = 0.34f; st.subAmp = 0.16f;
      st.bassWave = 1; st.arpWave = 2; st.leadWave = 0;
      st.leadAmp = 0.19f; st.arpAmp = 0.11f; st.padAmp = 0.10f; st.echo = 0.30f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    float bpm  = 88.0f;
    float beat = 60.0f / bpm;
    float melody[8] = {146.8f, 164.8f, 146.8f, 130.8f,
                       110.0f, 123.5f, 130.8f, 110.0f};
    for (int i = 0; i < N; ++i) {
        float t = (float)i / SR;
        float v = 0.0f;
        {   // Sub-bass epic pulse on every beat
            float pos = fmodf(t, beat);
            float env = std::exp(-pos * 20.0f);
            v += std::sin(2*kPI*55.0f*t) * env * 0.40f;
            v += rnd() * env * 0.06f;
        }
        {   // Slow bass melody
            int ni = (int)(t / (beat*2.0f)) % 8;
            v += std::sin(2*kPI*(melody[ni]*0.5f)*t) * 0.25f;
        }
        {   // Sweeping string-pad (3 detuned saws)
            float pEnv = 0.12f*(0.5f+0.5f*std::sin(2*kPI*0.08f*t));
            for (int k=0;k<3;++k){
                float ph=fmodf((110.0f-3.0f+k*3.0f)*t,1.0f);
                v+=(2.0f*ph-1.0f)*pEnv;
            }
        }
        {   // Rising brass stab every 4 beats
            float pos=fmodf(t,beat*4.0f);
            float env=std::max(0.0f,0.6f-pos/0.8f);
            int ni=(int)(t/(beat*4.0f))%8;
            float f=melody[ni];
            v+=std::sin(2*kPI*f*t)*env*0.20f;
            v+=std::sin(2*kPI*f*1.5f*t)*env*0.10f;
        }
        {   // Metallic percussion every 2 beats
            float pos=fmodf(t,beat*2.0f);
            float env=std::exp(-pos*14.0f);
            v+=(std::sin(2*kPI*220.0f*t)+rnd()*0.4f)*env*0.16f;
        }
        {   // High shimmer
            float shimF=880.0f+120.0f*std::sin(2*kPI*0.12f*t);
            v+=std::sin(2*kPI*shimF*t)*0.03f*(0.4f+0.6f*std::sin(2*kPI*0.25f*t));
        }
        v+=rnd()*0.015f*(0.3f+0.7f*std::sin(2*kPI*0.06f*t));
        s[i]=(short)(clamp1(v)*27000.0f);
    }
    return s;
}

std::vector<short> AudioManager::synthCemetery(int SR, int N) {
    // Cemiterio — assombrado, lento, sem bateria
    { TrackStyle st; st.bpm = 72.0f; st.drums = false; st.arpOn = false;
    // PERFIL ACUSTICO — cemiterio: escuro, nevoa, reverb longo
    st.cutoffHz = 2600.0f; st.room = 0.80f; st.wet = 0.42f; st.subAmp = 0.14f;
      st.padAmp = 0.13f; st.leadAmp = 0.12f; st.leadWave = 0; st.echo = 0.42f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    float bpm = 52.0f, beat = 60.0f / bpm;
    float bells[4] = {261.6f, 220.0f, 196.0f, 174.6f};
    for (int i = 0; i < N; ++i) {
        float t = (float)i / SR, v = 0.0f;
        { float e = std::exp(-fmodf(t, beat*4)*1.8f);
          int ni = (int)(t/(beat*4))%4;
          float s1 = std::sin(2*kPI*bells[ni]*t);
          float s2 = std::sin(2*kPI*bells[ni]*2.0f*t)*0.4f;
          v += (s1+s2)*e*0.30f; }
        { float dr = std::sin(2*kPI*40.0f*t)*0.2f*(0.5f+0.5f*std::sin(2*kPI*0.07f*t));
          v += dr; }
        { float wind = rnd()*0.08f*(0.3f+0.7f*std::sin(2*kPI*0.04f*t)); v += wind; }
        { float pos=fmodf(t,beat*2); float e2=std::exp(-pos*12.0f);
          v += (std::sin(2*kPI*110.0f*t)+rnd()*0.3f)*e2*0.12f; }
        s[i] = (short)(clamp1(v)*27000.0f);
    }
    return s;
}

std::vector<short> AudioManager::synthCursedFarm(int SR, int N) {
    // Fazenda Maldita — folk-horror inquieto
    { TrackStyle st; st.bpm = 76.0f; st.drums = false; st.arpOn = true;
    // PERFIL ACUSTICO — campo aberto ao anoitecer
    st.cutoffHz = 3600.0f; st.room = 0.50f; st.wet = 0.26f; st.subAmp = 0.16f;
      st.arpAmp = 0.05f; st.leadAmp = 0.11f; st.leadWave = 0; st.echo = 0.36f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    for (int i = 0; i < N; ++i) {
        float t = (float)i / SR, v = 0.0f;
        { float env = 0.3f + 0.2f*std::sin(2*kPI*0.05f*t);
          for (int k=0;k<3;++k){
              float ph=fmodf((58.0f-2.0f+k*2.0f)*t,1.0f);
              v+=(2.0f*ph-1.0f)*env*0.12f;
          }}
        { float crow=std::sin(2*kPI*800.0f*t)*std::exp(-fmodf(t,3.7f)*8.0f)*0.10f; v+=crow; }
        { float rus=rnd()*0.06f*(0.2f+0.8f*std::sin(2*kPI*0.12f*t)); v+=rus; }
        { float buzz=std::sin(2*kPI*55.0f*t+std::sin(2*kPI*2.2f*t)*3.0f)*0.15f
                      *(0.4f+0.6f*std::sin(2*kPI*0.09f*t)); v+=buzz; }
        s[i] = (short)(clamp1(v)*27000.0f);
    }
    return s;
}

std::vector<short> AudioManager::synthGhostCity(int SR, int N) {
    // Cidade Fantasma — eco urbano assombrado
    { TrackStyle st; st.bpm = 80.0f; st.drums = false; st.arpOn = true;
    // PERFIL ACUSTICO — ruas vazias: eco de predio
    st.cutoffHz = 3200.0f; st.room = 0.72f; st.wet = 0.36f; st.subAmp = 0.16f;
      st.arpAmp = 0.06f; st.leadAmp = 0.12f; st.leadWave = 0; st.echo = 0.44f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    float bpm=60.0f, beat=60.0f/bpm;
    float notes[6]={130.8f,146.8f,123.5f,116.5f,110.0f,130.8f};
    for (int i = 0; i < N; ++i) {
        float t=(float)i/SR, v=0.0f;
        { int ni=(int)(t/(beat*2))%6;
          float pEnv=0.15f*(0.4f+0.6f*std::sin(2*kPI*0.06f*t));
          for(int k=0;k<3;++k){
              float dFreq=notes[ni]*(1.0f+(k-1)*0.007f);
              v+=std::sin(2*kPI*dFreq*t)*pEnv;
          }}
        { float wind=rnd()*0.05f*(0.5f+0.5f*std::sin(2*kPI*0.03f*t)); v+=wind; }
        { float pos=fmodf(t,beat*3); float e=std::exp(-pos*6.0f);
          float echo=std::sin(2*kPI*220.0f*t)*e*0.08f; v+=echo; }
        { float bass=std::sin(2*kPI*55.0f*t)*0.18f*(0.3f+0.7f*std::sin(2*kPI*0.08f*t)); v+=bass; }
        s[i]=(short)(clamp1(v)*27000.0f);
    }
    return s;
}

std::vector<short> AudioManager::synthDarkForest(int SR, int N) {
    // Floresta Negra — tensa, nevoa sonora
    { TrackStyle st; st.bpm = 70.0f; st.drums = false; st.arpOn = false;
    // PERFIL ACUSTICO — floresta: folhagem come o agudo
    st.cutoffHz = 2800.0f; st.room = 0.66f; st.wet = 0.34f; st.subAmp = 0.15f;
      st.padAmp = 0.13f; st.leadAmp = 0.10f; st.leadWave = 0; st.echo = 0.42f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    for (int i = 0; i < N; ++i) {
        float t=(float)i/SR, v=0.0f;
        { float dr=std::sin(2*kPI*38.0f*t+std::sin(2*kPI*0.15f*t)*0.8f)*0.25f
                   *(0.6f+0.4f*std::sin(2*kPI*0.05f*t)); v+=dr; }
        { float cr=rnd()*0.04f*std::exp(-fmodf(t,0.23f)*40.0f); v+=cr; }
        { float owl=std::sin(2*kPI*(320.0f+40.0f*std::sin(2*kPI*0.4f*t))*t)
                    *0.07f*std::exp(-fmodf(t,5.3f)*3.0f); v+=owl; }
        { float amb=rnd()*0.03f*(0.4f+0.6f*std::sin(2*kPI*0.07f*t)); v+=amb; }
        { float tremolo=std::sin(2*kPI*82.0f*t)*0.12f
                        *(0.5f+0.5f*std::sin(2*kPI*4.5f*t)); v+=tremolo; }
        s[i]=(short)(clamp1(v)*27000.0f);
    }
    return s;
}

std::vector<short> AudioManager::synthCatacombs(int SR, int N) {
    // Catacumbas — profundo, eco de pedra
    { TrackStyle st; st.bpm = 66.0f; st.drums = false; st.arpOn = false;
    // PERFIL ACUSTICO — caverna de pedra: o espaco mais longo do jogo
    st.cutoffHz = 2200.0f; st.room = 0.88f; st.wet = 0.46f; st.subAmp = 0.18f;
      st.padAmp = 0.14f; st.leadAmp = 0.10f; st.leadWave = 0; st.echo = 0.46f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    float bpm=44.0f, beat=60.0f/bpm;
    for (int i = 0; i < N; ++i) {
        float t=(float)i/SR, v=0.0f;
        { float bass=std::sin(2*kPI*32.0f*t)*0.30f
                     *(0.6f+0.4f*std::sin(2*kPI*0.04f*t)); v+=bass; }
        { float pos=fmodf(t,beat); float e=std::exp(-pos*10.0f);
          v+=(std::sin(2*kPI*90.0f*t)+rnd()*0.3f)*e*0.14f; }
        { float drip=std::sin(2*kPI*1200.0f*t)*std::exp(-fmodf(t,2.1f)*18.0f)*0.06f; v+=drip; }
        { float moan=std::sin(2*kPI*(55.0f+5.0f*std::sin(2*kPI*0.03f*t))*t)
                     *0.10f*(0.3f+0.7f*std::sin(2*kPI*0.06f*t)); v+=moan; }
        { float rev=rnd()*0.02f*(0.2f+0.8f*std::sin(2*kPI*0.09f*t)); v+=rev; }
        s[i]=(short)(clamp1(v)*27000.0f);
    }
    return s;
}

std::vector<short> AudioManager::synthManor(int SR, int N) {
    // Mansao das Sombras — gotico
    { TrackStyle st; st.bpm = 74.0f; st.drums = false; st.arpOn = true;
    // PERFIL ACUSTICO — salao vazio de mansao
    st.cutoffHz = 3000.0f; st.room = 0.70f; st.wet = 0.36f; st.subAmp = 0.15f;
      st.arpAmp = 0.05f; st.leadAmp = 0.12f; st.leadWave = 0; st.echo = 0.42f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    float bpm=56.0f, beat=60.0f/bpm;
    float chord[4]={65.4f, 77.8f, 87.3f, 98.0f};
    for (int i = 0; i < N; ++i) {
        float t=(float)i/SR, v=0.0f;
        { for(int k=0;k<4;++k){
              float ph=fmodf(chord[k]*t,1.0f);
              float saw=(2.0f*ph-1.0f)*0.10f*(0.4f+0.6f*std::sin(2*kPI*0.05f*t));
              v+=saw;
          }}
        { float pos=fmodf(t,beat*2); float e=std::exp(-pos*4.0f);
          v+=std::sin(2*kPI*98.0f*t)*e*0.16f; }
        { float toll=std::sin(2*kPI*220.0f*t)*std::exp(-fmodf(t,beat*8)*0.8f)*0.12f; v+=toll; }
        { v+=rnd()*0.02f*(0.3f+0.7f*std::sin(2*kPI*0.04f*t)); }
        s[i]=(short)(clamp1(v)*27000.0f);
    }
    return s;
}

std::vector<short> AudioManager::synthInferno(int SR, int N) {
    // Zona Inferno — intenso, distorcido, rapido
    { TrackStyle st; st.bpm = 150.0f; st.hardDrums = true; st.drive = 0.60f;
    // PERFIL ACUSTICO — inferno: sujo, grave enorme, medio agressivo
    st.cutoffHz = 5600.0f; st.room = 0.44f; st.wet = 0.24f; st.subAmp = 0.28f;
      st.bassWave = 1; st.arpWave = 1; st.leadWave = 1; st.echo = 0.16f;
      return composeTrack(SR, N, st); }
    std::vector<short> s(N, 0);
    float bpm=140.0f, beat=60.0f/bpm;
    for (int i = 0; i < N; ++i) {
        float t=(float)i/SR, v=0.0f;
        { float bass=std::sin(2*kPI*55.0f*t+std::sin(2*kPI*55.0f*t)*2.5f)
                     *0.30f*(0.7f+0.3f*std::sin(2*kPI*0.1f*t)); v+=bass; }
        { float pos=fmodf(t,beat); float e=std::exp(-pos*22.0f);
          v+=(std::sin(2*kPI*110.0f*t)+rnd()*0.5f)*e*0.22f; }
        { float crackle=rnd()*0.10f*(0.3f+0.7f*std::sin(2*kPI*0.25f*t)); v+=crackle; }
        { float flame=std::sin(2*kPI*(220.0f+rnd()*80.0f)*t)*0.06f
                      *(0.4f+0.6f*std::sin(2*kPI*0.18f*t)); v+=flame; }
        { float dist=std::sin(2*kPI*880.0f*t)*0.05f
                     *(0.2f+0.8f*std::sin(2*kPI*0.07f*t)); v+=dist; }
        s[i]=(short)(clamp1(v)*27000.0f);
    }
    return s;
}

std::vector<unsigned char> AudioManager::buildMusicForZone(ZoneID zone, float dur, int SR) {
    int N = (int)(SR * dur);
    std::vector<unsigned char> buf(44 + N*2, 0);
    writeWavHeader(buf, SR, N);
    std::vector<short> samples;
    switch (zone) {
        case ZoneID::LARuins:       samples = synthLARuins(SR,N);   break;
        case ZoneID::Bunker:        samples = synthBunker(SR,N);    break;
        case ZoneID::KronosForge:   samples = synthFactory(SR,N);   break;
        case ZoneID::KronosNexus:   samples = synthCore(SR,N);      break;
        case ZoneID::Cemetery:      samples = synthCemetery(SR,N);  break;
        case ZoneID::CursedFarm:    samples = synthCursedFarm(SR,N);break;
        case ZoneID::GhostCity:     samples = synthGhostCity(SR,N); break;
        case ZoneID::DarkForest:    samples = synthDarkForest(SR,N);break;
        case ZoneID::Catacombs:     samples = synthCatacombs(SR,N); break;
        case ZoneID::AbandonedManor:samples = synthManor(SR,N);     break;
        case ZoneID::InfernoZone:   samples = synthInferno(SR,N);   break;
        default:                    samples = synthLARuins(SR,N);   break;
    }
    memcpy(buf.data()+44, samples.data(), N*2);
    return buf;
}

// â”€â”€â”€ Init / Shutdown â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void AudioManager::init() {
    InitAudioDevice();
    const int SR = 44100;

    // â”€â”€â”€ LASER â€” FM synthesis, sharp attack, 2200â†’400Hz sweep â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        // Blaster sci-fi punchy: SNAP transiente + sweep de pitch + corpo serra + cauda
        int N = (int)(SR * 0.20f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;

            // 1) Transiente de ataque (click branco bem curto ~3ms) — o "snap"
            float clickEnv = std::exp(-220.0f * pct);
            float click    = rnd() * clickEnv * 0.9f;

            // 2) Corpo: sweep rapido de 1900Hz -> 280Hz (o "pew")
            float freq = 280.0f + 1620.0f * std::exp(-22.0f * pct);
            float bodyEnv = std::exp(-9.0f * pct);
            float ph   = fmodf(freq * t, 1.0f);
            float saw  = (2.0f * ph - 1.0f);                 // serra = grit
            float v    = saw * 0.34f * bodyEnv;
            v += std::sin(2*kPI * freq * t) * 0.40f * bodyEnv;
            // 3) Sub para dar peso
            v += std::sin(2*kPI * (freq*0.5f) * t) * 0.18f * bodyEnv;
            // 4) Brilho de alta no inicio
            v += std::sin(2*kPI * freq * 2.0f * t) * 0.16f * std::exp(-30.0f*pct);

            s[i] = clamp1(click + v);
        }
        sfxLaser = sfxSynth(SR, N, s);
        SetSoundVolume(sfxLaser, 0.80f);
    }

    // â”€â”€â”€ EMP â€” sub-bass boom + electrical crackle spread â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.60f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float boomEnv = std::exp(-9.0f * pct);
            float v = std::sin(2*kPI * 58.0f * t) * 0.50f * boomEnv;
            v += std::sin(2*kPI * 38.0f * t) * 0.32f * boomEnv;
            float crackEnv = pct < 0.04f ? pct/0.04f : std::exp(-4.0f * (pct - 0.04f));
            v += rnd() * crackEnv * 0.48f;
            float zapF = 700.0f + 500.0f * std::sin(2*kPI * 90.0f * t);
            v += std::sin(2*kPI * zapF * t) * crackEnv * 0.18f;
            v += rnd() * 0.08f * (1.0f - pct); // sustained crackle tail
            s[i] = clamp1(v);
        }
        sfxEMP = sfxSynth(SR, N, s);
        SetSoundVolume(sfxEMP, 0.88f);
    }

    // â”€â”€â”€ HIT â€” metallic clank, inharmonic ring, reverb tail â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        // Impacto seco e satisfatorio: SNAP de impacto + ring metalico inarmonico
        int N = (int)(SR * 0.20f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;

            // Transiente de impacto (punch inicial ~2ms)
            float punch = rnd() * std::exp(-300.0f * pct) * 0.95f;
            // Thump grave que cai rapido (peso do golpe)
            float thumpF = std::max(70.0f, 240.0f - t * 900.0f);
            float thump  = std::sin(2*kPI * thumpF * t) * std::exp(-26.0f * pct) * 0.5f;

            float env = std::exp(-13.0f * pct);
            float v = std::sin(2*kPI * 840.0f * t) * 0.30f;
            v += std::sin(2*kPI * 1180.0f * t) * 0.20f;
            v += std::sin(2*kPI * 420.0f * t)  * 0.18f;
            v += rnd() * std::exp(-50.0f * pct) * 0.25f;

            s[i] = clamp1(punch + thump + v * env);
        }
        int delay = (int)(0.05f * SR);
        for (int i = delay; i < N; ++i)
            s[i] = clamp1(s[i] + s[i-delay] * 0.22f);
        sfxHit = sfxSynth(SR, N, s);
        SetSoundVolume(sfxHit, 0.72f);
    }

    // â”€â”€â”€ HIT HEAVY â€” bass thud + metallic scrape + reverb â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.38f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float thudEnv = std::exp(-7.0f * pct);
            float kf  = std::max(20.0f, 130.0f - t * 90.0f);
            float v   = std::sin(2*kPI * kf * t) * 0.55f * thudEnv;
            v += std::sin(2*kPI * 60.0f * t) * 0.30f * thudEnv;
            float scrEnv = std::exp(-5.5f * pct);
            v += rnd() * scrEnv * 0.38f;
            v += std::sin(2*kPI * 620.0f * t) * scrEnv * 0.13f;
            s[i] = clamp1(v);
        }
        int delay = (int)(0.07f * SR);
        for (int i = delay; i < N; ++i)
            s[i] = clamp1(s[i] + s[i-delay] * 0.24f);
        sfxHitHeavy = sfxSynth(SR, N, s);
        SetSoundVolume(sfxHitHeavy, 0.82f);
    }

    // â”€â”€â”€ ALIEN HIT â€” wet organic thud â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.20f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float env = std::exp(-9.0f * pct);
            float v = std::sin(2*kPI * 95.0f * t) * 0.42f;
            v += std::sin(2*kPI * 58.0f * t) * 0.32f;
            v += rnd() * 0.48f * std::exp(-28.0f * pct);
            float org = 180.0f + 280.0f * pct;
            v += std::sin(2*kPI * org * t) * 0.14f;
            s[i] = clamp1(v * env);
        }
        sfxHitAlien = sfxSynth(SR, N, s);
        SetSoundVolume(sfxHitAlien, 0.65f);
    }

    // â”€â”€â”€ EXPLOSION â€” sub punch, mid crunch, hi shrapnel â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.70f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float subEnv = pct < 0.008f ? pct/0.008f : std::exp(-4.2f * (pct - 0.008f));
            float sf  = std::max(18.0f, 58.0f - t * 32.0f);
            float v   = std::sin(2*kPI * sf * t) * 0.58f * subEnv;
            v += std::sin(2*kPI * 32.0f * t) * 0.32f * subEnv;
            float midEnv = std::exp(-6.5f * pct);
            v += rnd() * midEnv * 0.42f;
            float ph = fmodf(200.0f * t, 1.0f);
            v += clamp1((2.0f*ph-1.0f) * 3.0f) * midEnv * 0.14f;
            float hiEnv = pct < 0.025f ? 0.0f : std::exp(-9.5f * (pct - 0.025f));
            v += rnd() * hiEnv * 0.22f;
            s[i] = clamp1(v);
        }
        sfxExplosion = sfxSynth(SR, N, s);
        SetSoundVolume(sfxExplosion, 0.82f);
    }

    // â”€â”€â”€ BIG EXPLOSION â€” massive layered shockwave â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 1.1f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float subEnv = pct < 0.006f ? pct/0.006f : std::exp(-2.8f * (pct - 0.006f));
            float v = std::sin(2*kPI * 36.0f * t) * 0.68f * subEnv;
            v += std::sin(2*kPI * 20.0f * t) * 0.42f * subEnv;
            float midEnv = std::exp(-4.5f * pct);
            v += rnd() * midEnv * 0.58f;
            v += std::sin(2*kPI * 110.0f * t) * midEnv * 0.18f;
            float hiEnv = pct < 0.04f ? 0.0f : std::exp(-6.5f * (pct - 0.04f));
            v += rnd() * hiEnv * 0.32f;
            s[i] = clamp1(v);
        }
        int d1 = (int)(0.09f * SR), d2 = (int)(0.20f * SR);
        for (int i = d2; i < N; ++i)
            s[i] = clamp1(s[i] + s[i-d1]*0.20f + s[i-d2]*0.10f);
        sfxExplosionBig = sfxSynth(SR, N, s);
        SetSoundVolume(sfxExplosionBig, 0.95f);
    }

    // â”€â”€â”€ PICKUP â€” bright ascending chime â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.22f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float env = pct < 0.04f ? pct/0.04f : std::exp(-8.5f * (pct - 0.04f));
            float v = std::sin(2*kPI * 1320.0f * t) * 0.48f;
            v += std::sin(2*kPI * 1980.0f * t) * 0.28f;
            v += std::sin(2*kPI * 2640.0f * t) * 0.16f;
            s[i] = v * env;
        }
        sfxPickup = sfxSynth(SR, N, s);
        SetSoundVolume(sfxPickup, 0.52f);
    }

    // â”€â”€â”€ PICKUP CREDITS â€” 3-note coin jingle â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.28f);
        std::vector<float> s(N);
        float notes[3] = {1047.0f, 1319.0f, 1568.0f};
        int   segLen   = N / 3;
        for (int i = 0; i < N; ++i) {
            float t    = (float)i / SR;
            int   ni   = std::min(2, i / segLen);
            float noff = (float)(i - ni * segLen) / segLen;
            float env  = noff < 0.02f ? noff/0.02f : std::exp(-16.0f * (noff - 0.02f));
            float freq = notes[ni];
            float v = std::sin(2*kPI * freq * t) * 0.58f;
            v += std::sin(2*kPI * freq * 2.0f * t) * 0.22f;
            s[i] = v * env;
        }
        sfxPickupCredits = sfxSynth(SR, N, s);
        SetSoundVolume(sfxPickupCredits, 0.56f);
    }

    // â”€â”€â”€ LEVEL UP â€” C-E-G-C ascending arpeggio â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 1.3f);
        std::vector<float> s(N);
        float arpF[4] = {523.25f, 659.25f, 783.99f, 1046.50f};
        int   segLen  = N / 4;
        for (int i = 0; i < N; ++i) {
            float t    = (float)i / SR;
            int   ni   = std::min(3, i / segLen);
            float noff = (float)(i - ni * segLen) / segLen;
            float env  = noff < 0.04f ? noff/0.04f : std::exp(-3.0f * (noff - 0.04f));
            float freq = arpF[ni];
            float v = std::sin(2*kPI * freq * t) * 0.55f;
            v += std::sin(2*kPI * freq * 2.0f * t) * 0.16f;
            v += std::sin(2*kPI * freq * 1.005f * t) * 0.11f; // chorus
            s[i] = v * env;
        }
        sfxLevelUp = sfxSynth(SR, N, s);
        SetSoundVolume(sfxLevelUp, 0.85f);
    }

    // â”€â”€â”€ SHIELD â€” electric buzz, reverse-style attack â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.52f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float env = pct < 0.12f ? pct/0.12f : std::exp(-3.5f * (pct - 0.12f));
            float freq = 1500.0f + 900.0f * pct;
            float v = std::sin(2*kPI * freq * t) * 0.48f;
            float ph = fmodf(freq * 0.5f * t, 1.0f);
            v += (2.0f*ph-1.0f) * 0.26f;
            v += std::sin(2*kPI * freq * 3.0f * t) * 0.14f;
            v += rnd() * 0.09f * pct;
            s[i] = clamp1(v * env);
        }
        sfxShield = sfxSynth(SR, N, s);
        SetSoundVolume(sfxShield, 0.68f);
    }

    // â”€â”€â”€ PORTAL â€” sweep down then up, alien timbre â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 1.25f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float env = pct < 0.04f ? pct/0.04f : (pct > 0.88f ? (1.0f-pct)/0.12f : 0.97f);
            float freq;
            if (pct < 0.5f) {
                freq = 800.0f * std::exp(-3.2f * pct * 2.0f);
            } else {
                float p2 = (pct - 0.5f) * 2.0f;
                freq = 75.0f + 1150.0f * (1.0f - std::exp(-3.2f * p2));
            }
            float v = std::sin(2*kPI * freq * t) * 0.42f;
            v += std::sin(2*kPI * freq * 1.5f * t) * 0.24f;
            v += std::sin(2*kPI * freq * 0.5f * t) * 0.20f;
            v += rnd() * 0.10f;
            s[i] = clamp1(v * env);
        }
        sfxPortal = sfxSynth(SR, N, s);
        SetSoundVolume(sfxPortal, 0.87f);
    }

    // â”€â”€â”€ FOOTSTEP â€” heavy boot thud, randomized each play â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int variant = rand() % 3;
        float bf[3] = {85.0f, 105.0f, 68.0f};
        int N = (int)(SR * 0.11f);
        std::vector<float> s(N);
        float b = bf[variant];
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float env = std::exp(-28.0f * pct);
            float kf  = std::max(20.0f, b - t * b * 7.0f);
            float v = std::sin(2*kPI * kf * t) * 0.72f;
            v += rnd() * std::exp(-65.0f * pct) * 0.42f;
            s[i] = clamp1(v * env);
        }
        sfxFootstep = sfxSynth(SR, N, s);
        SetSoundVolume(sfxFootstep, 0.22f);
    }

    // â”€â”€â”€ MELEE SWING â€” air whoosh â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.15f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float wEnv = pct < 0.25f ? pct/0.25f : (1.0f - pct)/0.75f;
            float freq = 180.0f + 650.0f * std::sin(kPI * pct);
            float v = rnd() * 0.62f;
            v += std::sin(2*kPI * freq * t) * 0.22f;
            v += std::sin(2*kPI * freq * 1.8f * t) * 0.12f;
            s[i] = v * wEnv;
        }
        sfxMeleeSwing = sfxSynth(SR, N, s);
        SetSoundVolume(sfxMeleeSwing, 0.58f);
    }

    // â”€â”€â”€ MELEE IMPACT â€” heavy crunch + thud composite â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.25f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            // Crunch pesado com SNAP de impacto e thump grave (soco com peso)
            float snap = rnd() * std::exp(-260.0f * pct) * 1.0f;
            float thumpF = std::max(55.0f, 200.0f - t * 700.0f);
            float thump  = std::sin(2*kPI * thumpF * t) * std::exp(-20.0f * pct) * 0.55f;
            float crE = std::exp(-14.0f * pct);
            float v = rnd() * crE * 0.40f;
            v += std::sin(2*kPI * 360.0f * t) * crE * 0.26f;
            v += std::sin(2*kPI * 185.0f * t) * crE * 0.22f;
            float ringFreq = std::max(100.0f, 820.0f - t * 450.0f);
            v += std::sin(2*kPI * ringFreq * t) * std::exp(-7.5f * pct) * 0.20f;
            s[i] = clamp1(snap + thump + v);
        }
        sfxMeleeImpact = sfxSynth(SR, N, s);
        SetSoundVolume(sfxMeleeImpact, 0.95f);
    }

    // â”€â”€â”€ ALIEN SCREAM â€” saw wave + noise + fast vibrato, descending â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.65f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float env = pct < 0.04f ? pct/0.04f : std::exp(-3.2f * (pct - 0.04f));
            float freq = 580.0f * std::exp(-2.4f * pct);
            float vibrato = 1.0f + 0.09f * std::sin(2*kPI * 22.0f * t);
            float ph  = fmodf(freq * vibrato * t, 1.0f);
            float v   = (2.0f*ph - 1.0f) * 0.52f;
            v += rnd() * 0.38f;
            float ph2 = fmodf(freq * 2.0f * vibrato * t, 1.0f);
            v += (2.0f*ph2 - 1.0f) * 0.14f;
            s[i] = clamp1(v * env);
        }
        sfxAlienScream = sfxSynth(SR, N, s);
        SetSoundVolume(sfxAlienScream, 0.82f);
    }

    // â”€â”€â”€ BOSS ROAR â€” chest rumble + sub shockwave, 1.4s â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 1.4f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float subEnv = pct < 0.07f ? pct/0.07f : std::exp(-2.4f * (pct - 0.07f));
            float v = std::sin(2*kPI * 36.0f * t) * 0.62f * subEnv;
            v += std::sin(2*kPI * 55.0f * t) * 0.36f * subEnv;
            v += std::sin(2*kPI * 88.0f * t) * 0.24f * subEnv;
            v += std::sin(2*kPI * 120.0f * t) * 0.14f * subEnv;
            v += rnd() * std::exp(-3.8f * pct) * 0.28f;
            float growlF = 175.0f + 45.0f * std::sin(2*kPI * 9.0f * t);
            v += std::sin(2*kPI * growlF * t) * subEnv * 0.13f;
            s[i] = clamp1(v);
        }
        sfxBossRoar = sfxSynth(SR, N, s);
        SetSoundVolume(sfxBossRoar, 0.96f);
    }

    // â”€â”€â”€ CHARGE UP â€” ascending 200â†’1800Hz electronic hum â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.88f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float t   = (float)i / SR;
            float pct = (float)i / N;
            float env = pct < 0.04f ? pct/0.04f : (pct > 0.92f ? (1.0f-pct)/0.08f : 1.0f);
            float freq = 200.0f * std::exp(std::log(9.0f) * pct);
            float v = std::sin(2*kPI * freq * t) * 0.46f;
            v += std::sin(2*kPI * freq * 2.0f * t) * 0.20f * pct;
            v += std::sin(2*kPI * freq * 3.0f * t) * 0.12f * pct;
            float ph = fmodf(freq * t, 1.0f);
            v += (2.0f*ph - 1.0f) * 0.16f * pct;
            v += rnd() * 0.08f * pct;
            s[i] = clamp1(v * env);
        }
        sfxChargeUp = sfxSynth(SR, N, s);
        SetSoundVolume(sfxChargeUp, 0.72f);
    }

    // â”€â”€â”€ BURST â€” 3 rapid shots â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int N = (int)(SR * 0.20f);
        std::vector<float> s(N);
        int   shotLen = N / 3;
        for (int i = 0; i < N; ++i) {
            float t    = (float)i / SR;
            int   shot = i / shotLen;
            float pct  = (float)(i - shot * shotLen) / shotLen;
            float env  = pct < 0.02f ? pct/0.02f : std::exp(-20.0f * (pct - 0.02f));
            float freq = 1500.0f - shot * 220.0f;
            float v = std::sin(2*kPI * freq * t) * 0.52f;
            v += rnd() * 0.36f;
            s[i] = clamp1(v * env);
        }
        sfxBurst = sfxSynth(SR, N, s);
        SetSoundVolume(sfxBurst, 0.72f);
    }

    // â”€â”€â”€ RICOCHETS â€” 3 variants with pitch drops â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        float ricos[3]    = {1850.0f, 2250.0f, 1620.0f};
        float ricoDrops[3]= {-900.0f, -1100.0f, -700.0f};
        for (int r = 0; r < 3; ++r) {
            int N = (int)(SR * 0.14f);
            std::vector<float> s(N);
            for (int i = 0; i < N; ++i) {
                float t   = (float)i / SR;
                float pct = (float)i / N;
                float env = std::exp(-11.0f * pct);
                float freq = std::max(200.0f, ricos[r] + ricoDrops[r] * pct);
                float v = std::sin(2*kPI * freq * t) * 0.66f;
                v += std::sin(2*kPI * freq * 1.55f * t) * 0.22f;
                s[i] = v * env;
            }
            sfxRicochets[r] = sfxSynth(SR, N, s);
            SetSoundVolume(sfxRicochets[r], 0.46f);
        }
    }

    // â”€â”€ Music â€” generate 4 zone tracks (30s loops) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    // ── New SFX ──────────────────────────────────────────────────────────────
    {   // PLAYER HURT
        int N = (int)(SR * 0.12f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float f=400.0f*std::exp(-4.0f*pct);
            float env=pct<0.02f?pct/0.02f:std::exp(-12.0f*(pct-0.02f));
            s[i]=clamp1((std::sin(2*kPI*f*t)*0.6f+rnd()*0.1f)*env);
        }
        sfxPlayerHurt=sfxSynth(SR,N,s); SetSoundVolume(sfxPlayerHurt,0.70f);
    }
    {   // PLAYER DEATH
        int N = (int)(SR * 1.2f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float f=300.0f*std::exp(-3.5f*pct);
            float env=pct<0.04f?pct/0.04f:std::exp(-2.0f*(pct-0.04f));
            float v=std::sin(2*kPI*f*t)*0.55f+std::sin(2*kPI*f*0.5f*t)*0.3f+rnd()*0.12f*std::exp(-6.0f*pct);
            s[i]=clamp1(v*env);
        }
        sfxPlayerDeath=sfxSynth(SR,N,s); SetSoundVolume(sfxPlayerDeath,0.88f);
    }
    {   // DEATH CRY -- grito de voz humana angustiada ("aaah" com formantes)
        int N = (int)(SR * 1.4f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float vib = 1.0f + 0.06f*std::sin(2*kPI*6.0f*t);       // vibrato
            float f0  = (320.0f - 120.0f*pct) * vib;               // pitch cai (desespero)
            float ph  = fmodf(f0*t, 1.0f);
            float src = (2.0f*ph - 1.0f);                          // fonte glotal (serra)
            float f1  = std::sin(2*kPI*730.0f*t) * 0.5f;           // formante F1 (vogal "ah")
            float f2  = std::sin(2*kPI*1100.0f*t) * 0.3f;          // formante F2
            float voice = src*0.4f + (f1+f2) * (0.4f + 0.3f*src);
            voice += rnd() * 0.10f;                                // respiracao/breathiness
            float env = pct<0.05f ? pct/0.05f : (pct>0.8f ? (1.0f-pct)/0.2f : 1.0f);
            env *= 0.85f + 0.15f*std::sin(2*kPI*9.0f*t);           // tremor de choro
            s[i] = clamp1(voice * env * 0.8f);
        }
        sfxDeathCry=sfxSynth(SR,N,s); SetSoundVolume(sfxDeathCry,0.92f);
    }
    {   // EVOLVE -- sweep 100->900Hz
        int N = (int)(SR * 1.3f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float f=100.0f*std::exp(std::log(9.0f)*pct);
            float mod=std::sin(2*kPI*f*2.1f*t)*(1.5f-pct);
            float env=pct<0.05f?pct/0.05f:(pct>0.88f?(1.0f-pct)/0.12f:1.0f);
            s[i]=clamp1((std::sin(2*kPI*f*t+mod*2.0f)*0.5f+std::sin(2*kPI*f*1.5f*t)*0.2f*pct+rnd()*0.05f*pct)*env);
        }
        sfxEvolve=sfxSynth(SR,N,s); SetSoundVolume(sfxEvolve,0.85f);
    }
    {   // HEAL
        int N = (int)(SR * 0.45f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float env=pct<0.06f?pct/0.06f:std::exp(-4.5f*(pct-0.06f));
            s[i]=(std::sin(2*kPI*600.0f*t)*0.45f+std::sin(2*kPI*900.0f*t)*0.22f+std::sin(2*kPI*1200.0f*t)*0.14f*(1.0f-pct))*env;
        }
        sfxHeal=sfxSynth(SR,N,s); SetSoundVolume(sfxHeal,0.60f);
    }
    {   // ITEM RARE -- 3-chord
        int N = (int)(SR * 0.55f);
        std::vector<float> s(N);
        float nts[3]={523.25f,659.25f,783.99f};
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float env=pct<0.04f?pct/0.04f:std::exp(-3.5f*(pct-0.04f));
            float v=0;
            for(float f:nts){v+=std::sin(2*kPI*f*t)*0.28f+std::sin(2*kPI*f*1.004f*t)*0.08f;}
            s[i]=v*env;
        }
        sfxItemRare=sfxSynth(SR,N,s); SetSoundVolume(sfxItemRare,0.72f);
    }
    {   // ITEM LEGENDARY -- 4-note fanfare
        int N = (int)(SR * 0.9f);
        std::vector<float> s(N);
        float nts[4]={523.25f,659.25f,783.99f,1046.50f};
        int seg=N/4;
        for (int i = 0; i < N; ++i) {
            int ni=std::min(3,i/seg); float pct=(float)(i-ni*seg)/seg; float t=(float)i/SR;
            float env=pct<0.04f?pct/0.04f:std::exp(-2.5f*(pct-0.04f));
            float f=nts[ni];
            s[i]=(std::sin(2*kPI*f*t)*0.55f+std::sin(2*kPI*f*2.0f*t)*0.18f+std::sin(2*kPI*f*0.5f*t)*0.12f+std::sin(2*kPI*f*1.005f*t)*0.10f)*env;
        }
        sfxItemLegendary=sfxSynth(SR,N,s); SetSoundVolume(sfxItemLegendary,0.90f);
    }
    {   // PORTAL CLOSE
        int N = (int)(SR * 0.85f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float env=pct<0.04f?pct/0.04f:(pct>0.85f?(1.0f-pct)/0.15f:1.0f);
            float f=50.0f+350.0f*pct;
            s[i]=clamp1((std::sin(2*kPI*f*t)*0.44f+std::sin(2*kPI*f*1.5f*t)*0.22f+std::sin(2*kPI*f*0.5f*t)*0.18f+rnd()*0.08f*(1.0f-pct))*env);
        }
        sfxPortalClose=sfxSynth(SR,N,s); SetSoundVolume(sfxPortalClose,0.80f);
    }
    {   // SKILL UNLOCK
        int N = (int)(SR * 0.65f);
        std::vector<float> s(N);
        float nts[3]={523.25f,659.25f,1046.50f};
        int seg=N/3;
        for (int i = 0; i < N; ++i) {
            int ni=std::min(2,i/seg); float pct=(float)(i-ni*seg)/seg; float t=(float)i/SR;
            float env=pct<0.03f?pct/0.03f:std::exp(-4.0f*(pct-0.03f));
            s[i]=(std::sin(2*kPI*nts[ni]*t)*0.50f+std::sin(2*kPI*nts[ni]*2.0f*t)*0.15f)*env;
        }
        sfxSkillUnlock=sfxSynth(SR,N,s); SetSoundVolume(sfxSkillUnlock,0.75f);
    }
    {   // ACHIEVEMENT
        int N = (int)(SR * 0.75f);
        std::vector<float> s(N);
        float nts[3]={659.25f,783.99f,1046.50f};
        int seg=N/3;
        for (int i = 0; i < N; ++i) {
            int ni=std::min(2,i/seg); float pct=(float)(i-ni*seg)/seg; float t=(float)i/SR;
            float env=pct<0.03f?pct/0.03f:std::exp(-3.0f*(pct-0.03f));
            float f=nts[ni];
            s[i]=(std::sin(2*kPI*f*t)*0.52f+std::sin(2*kPI*f*1.5f*t)*0.20f+std::sin(2*kPI*f*2.0f*t)*0.10f)*env;
        }
        sfxAchievement=sfxSynth(SR,N,s); SetSoundVolume(sfxAchievement,0.82f);
    }
    {   // LAVA BUBBLE
        int N = (int)(SR * 0.45f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float env=pct<0.1f?pct/0.1f:std::exp(-5.0f*(pct-0.1f));
            s[i]=clamp1(rnd()*0.5f*std::exp(-8.0f*pct)+std::sin(2*kPI*60.0f*t)*0.35f*env+std::sin(2*kPI*90.0f*t)*0.15f*env);
        }
        sfxLavaBubble=sfxSynth(SR,N,s); SetSoundVolume(sfxLavaBubble,0.55f);
    }
    {   // GEYSER ERUPT
        int N = (int)(SR * 0.65f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float env=pct<0.05f?pct/0.05f:std::exp(-3.5f*(pct-0.05f));
            float f=80.0f+50.0f*pct;
            s[i]=clamp1((rnd()*0.55f+std::sin(2*kPI*f*t)*0.28f+std::sin(2*kPI*f*2.0f*t)*0.12f)*env);
        }
        sfxGeyserErupt=sfxSynth(SR,N,s); SetSoundVolume(sfxGeyserErupt,0.78f);
    }
    {   // LAVA DMG
        int N = (int)(SR * 0.10f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float env=std::exp(-15.0f*pct);
            float mod=std::sin(2*kPI*1200.0f*t)*1.5f;
            s[i]=clamp1((std::sin(2*kPI*500.0f*t+mod)*0.45f+rnd()*0.3f*std::exp(-30.0f*pct))*env);
        }
        sfxLavaDmg=sfxSynth(SR,N,s); SetSoundVolume(sfxLavaDmg,0.65f);
    }
    {   // GHOST WAIL
        int N = (int)(SR * 1.1f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float env=pct<0.08f?pct/0.08f:std::exp(-2.5f*(pct-0.08f));
            float f=680.0f*std::exp(-1.5f*pct);
            float vib=1.0f+0.12f*std::sin(2*kPI*7.5f*t);
            float ph=fmodf(f*vib*t,1.0f);
            s[i]=clamp1(((2.0f*ph-1.0f)*0.40f+rnd()*0.12f+std::sin(2*kPI*f*2.0f*vib*t)*0.12f)*env);
        }
        sfxGhostWail=sfxSynth(SR,N,s); SetSoundVolume(sfxGhostWail,0.65f);
    }
    {   // THUNDER
        int N = (int)(SR * 1.8f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float cE=pct<0.005f?pct/0.005f:std::exp(-9.0f*(pct-0.005f));
            float rE=pct<0.01f?pct/0.01f:std::exp(-2.8f*(pct-0.01f));
            s[i]=clamp1(rnd()*cE*0.90f+std::sin(2*kPI*32.0f*t)*rE*0.55f+std::sin(2*kPI*20.0f*t)*rE*0.38f+rnd()*rE*0.20f);
        }
        sfxThunder=sfxSynth(SR,N,s); SetSoundVolume(sfxThunder,0.90f);
    }
    {   // CRIT HIT
        int N = (int)(SR * 0.18f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            s[i]=clamp1((rnd()*0.55f+std::sin(2*kPI*2800.0f*t)*0.28f+std::sin(2*kPI*1400.0f*t)*0.18f)*std::exp(-18.0f*pct));
        }
        sfxCritHit=sfxSynth(SR,N,s); SetSoundVolume(sfxCritHit,0.80f);
    }
    {   // BOSS PHASE
        int N = (int)(SR * 1.5f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float env=pct<0.06f?pct/0.06f:std::exp(-1.8f*(pct-0.06f));
            float f1=38.0f+20.0f*std::sin(2*kPI*0.4f*t);
            float mod=std::sin(2*kPI*0.8f*t)*800.0f;
            s[i]=clamp1((std::sin(2*kPI*f1*t)*0.55f+std::sin(2*kPI*f1*2.0f*t)*0.25f+std::sin(2*kPI*(400.0f+mod)*t)*0.18f+rnd()*0.08f*std::exp(-5.0f*pct))*env);
        }
        sfxBossPhase=sfxSynth(SR,N,s); SetSoundVolume(sfxBossPhase,0.92f);
    }
    {   // PORTAL SUCK
        int N = (int)(SR * 0.60f);
        std::vector<float> s(N);
        for (int i = 0; i < N; ++i) {
            float pct=(float)i/N; float t=(float)i/SR;
            float f=120.0f-80.0f*pct;
            float env=pct<0.1f?pct/0.1f:(pct>0.85f?(1.0f-pct)/0.15f:1.0f);
            s[i]=clamp1((std::sin(2*kPI*f*t)*0.40f+std::sin(2*kPI*f*1.5f*t)*0.18f+rnd()*0.12f*(1.0f-pct))*env);
        }
        sfxPortalSuck=sfxSynth(SR,N,s); SetSoundVolume(sfxPortalSuck,0.50f);
    }

    // Initialize musicLoaded
    for (int i = 0; i < kNumZones; ++i) musicLoaded[i] = false;

    // ── Music -- all 11 zones (30s loops) ────────────────────────────────────
    const float DUR = 30.0f;
    ZoneID allZones[kNumZones] = {
        ZoneID::LARuins, ZoneID::Bunker, ZoneID::KronosForge, ZoneID::KronosNexus,
        ZoneID::Cemetery, ZoneID::CursedFarm, ZoneID::GhostCity, ZoneID::DarkForest,
        ZoneID::Catacombs, ZoneID::AbandonedManor, ZoneID::InfernoZone
    };
    for (int i = 0; i < kNumZones; ++i) {
        musicBuffers[i] = buildMusicForZone(allZones[i], DUR, SR);
        bgMusic[i] = LoadMusicStreamFromMemory(".wav",
                        musicBuffers[i].data(), (int)musicBuffers[i].size());
        if (bgMusic[i].stream.sampleRate > 0) {
            musicLoaded[i] = true;
            SetMusicVolume(bgMusic[i], 0.48f);
        }
    }
    // Menu music (25s loop)
    {
        int menuN = (int)(SR * 25.0f);
        menuMusicBuffer.resize(44 + menuN*2, 0);
        writeWavHeader(menuMusicBuffer, SR, menuN);
        auto menuSamples = synthMenu(SR, menuN);
        memcpy(menuMusicBuffer.data()+44, menuSamples.data(), menuN*2);
        menuMusic = LoadMusicStreamFromMemory(".wav",
                        menuMusicBuffer.data(), (int)menuMusicBuffer.size());
        if (menuMusic.stream.sampleRate > 0) {
            menuMusicLoaded = true;
            SetMusicVolume(menuMusic, 0.58f);
        }
    }
}

void AudioManager::setZone(ZoneID zone) {
    if (zone == currentMusicZone) return;
    int oldIdx = (int)currentMusicZone;
    int newIdx = (int)zone;
    if (musicLoaded[oldIdx]) StopMusicStream(bgMusic[oldIdx]);
    currentMusicZone = zone;
    if (musicLoaded[newIdx]) {
        SetMusicVolume(bgMusic[newIdx], 0.48f);
        PlayMusicStream(bgMusic[newIdx]);
    }
}

void AudioManager::setCombat(bool fighting) {
    if (fighting == inCombat) return;
    inCombat = fighting;
    int idx = (int)currentMusicZone;
    if (musicLoaded[idx])
        SetMusicVolume(bgMusic[idx], fighting ? 0.65f : 0.48f);
}

void AudioManager::shutdown() {
    UnloadSound(sfxLaser);      UnloadSound(sfxEMP);
    UnloadSound(sfxMeleeSwing); UnloadSound(sfxMeleeImpact);
    UnloadSound(sfxHit);        UnloadSound(sfxHitHeavy);
    UnloadSound(sfxHitAlien);   UnloadSound(sfxExplosion);
    UnloadSound(sfxExplosionBig); UnloadSound(sfxPickup);
    UnloadSound(sfxPickupCredits); UnloadSound(sfxLevelUp);
    UnloadSound(sfxShield);     UnloadSound(sfxPortal);
    UnloadSound(sfxPortalClose); UnloadSound(sfxFootstep);
    UnloadSound(sfxAlienScream); UnloadSound(sfxBossRoar);
    UnloadSound(sfxChargeUp);   UnloadSound(sfxBurst);
    UnloadSound(sfxPlayerHurt); UnloadSound(sfxPlayerDeath); UnloadSound(sfxDeathCry);
    UnloadSound(sfxEvolve);     UnloadSound(sfxHeal);
    UnloadSound(sfxItemRare);   UnloadSound(sfxItemLegendary);
    UnloadSound(sfxSkillUnlock); UnloadSound(sfxAchievement);
    UnloadSound(sfxLavaBubble); UnloadSound(sfxGeyserErupt);
    UnloadSound(sfxLavaDmg);    UnloadSound(sfxGhostWail);
    UnloadSound(sfxThunder);    UnloadSound(sfxCritHit);
    UnloadSound(sfxBossPhase);  UnloadSound(sfxPortalSuck);
    for (auto& r : sfxRicochets) UnloadSound(r);
    for (int i = 0; i < kNumZones; ++i)
        if (musicLoaded[i]) UnloadMusicStream(bgMusic[i]);
    if (menuMusicLoaded) UnloadMusicStream(menuMusic);
    CloseAudioDevice();
}

void AudioManager::updateMusic() {
    for (int i = 0; i < kNumZones; ++i)
        if (musicLoaded[i] && i == (int)currentMusicZone)
            UpdateMusicStream(bgMusic[i]);
    if (menuMusicLoaded) UpdateMusicStream(menuMusic);
}

void AudioManager::playMenuMusic() {
    if (menuMusicLoaded) {
        // Stop zone music if any
        for (int i = 0; i < kNumZones; ++i)
            if (musicLoaded[i]) StopMusicStream(bgMusic[i]);
        PlayMusicStream(menuMusic);
    }
}

void AudioManager::stopMenuMusic() {
    if (menuMusicLoaded) StopMusicStream(menuMusic);
}

// ── Controles de mute ────────────────────────────────────────────────────────
void AudioManager::setMusicEnabled(bool b) {
    musicEnabled = b;
    float zv = b ? 0.48f : 0.0f;
    for (int i = 0; i < kNumZones; ++i)
        if (musicLoaded[i]) SetMusicVolume(bgMusic[i], zv);
    if (menuMusicLoaded) SetMusicVolume(menuMusic, b ? 0.58f : 0.0f);
}

void AudioManager::setAllSoundOn(bool b) {
    allSoundOn = b;
    SetMasterVolume(b ? 1.0f : 0.0f);  // master kill switch de TUDO
}

void AudioManager::playDeathCry() const {
    if (!voiceEnabled) return;
    PlaySound(sfxDeathCry);
}

// â”€â”€â”€ Play â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void AudioManager::playLaser()         const { PlaySound(sfxLaser); }
void AudioManager::playEMP()           const { PlaySound(sfxEMP); }
void AudioManager::playHit()           const { PlaySound(sfxHit); }
void AudioManager::playHitHeavy()      const { PlaySound(sfxHitHeavy); }
void AudioManager::playHitAlien()      const { PlaySound(sfxHitAlien); }
void AudioManager::playExplosion()     const { PlaySound(sfxExplosion); }
void AudioManager::playExplosionBig()  const { PlaySound(sfxExplosionBig); }
void AudioManager::playPickup()        const { PlaySound(sfxPickup); }
void AudioManager::playPickupCredits() const { PlaySound(sfxPickupCredits); }
void AudioManager::playLevelUp()       const { PlaySound(sfxLevelUp); }
void AudioManager::playShield()        const { PlaySound(sfxShield); }
void AudioManager::playPortal()        const { PlaySound(sfxPortal); }
void AudioManager::playFootstep()      const { PlaySound(sfxFootstep); }
void AudioManager::playMeleeSwing()    const { PlaySound(sfxMeleeSwing); }
void AudioManager::playMeleeImpact()   const { PlaySound(sfxMeleeImpact); }
void AudioManager::playAlienScream()   const { PlaySound(sfxAlienScream); }
void AudioManager::playBossRoar()      const { PlaySound(sfxBossRoar); }
void AudioManager::playChargeUp()      const { PlaySound(sfxChargeUp); }
void AudioManager::playBurst()         const { PlaySound(sfxBurst); }
void AudioManager::playRicochet()      const { PlaySound(sfxRicochets[rand() % 3]); }

// ── New contextual play methods ───────────────────────────────────────────────

void AudioManager::playMeleeHit(bool isCrit)    const { isCrit ? PlaySound(sfxCritHit) : PlaySound(sfxMeleeImpact); }
void AudioManager::playPlasmaShot()             const { PlaySound(sfxLaser); }
void AudioManager::playShotgun()                const { PlaySound(sfxExplosion); }
void AudioManager::playExplosion(bool large)    const { large ? PlaySound(sfxExplosionBig) : PlaySound(sfxExplosion); }
void AudioManager::playEnemyHit()               const { PlaySound(sfxHit); }
void AudioManager::playEnemyDeath(bool isBoss)  const { isBoss ? PlaySound(sfxBossRoar) : PlaySound(sfxAlienScream); }
void AudioManager::playPlayerHurt()             const { if (voiceEnabled) PlaySound(sfxPlayerHurt); }
void AudioManager::playEvolve()                 const { PlaySound(sfxEvolve); }
void AudioManager::playHeal()                   const { PlaySound(sfxHeal); }
void AudioManager::playItemPickup(int rarity)   const {
    if (rarity >= 5)      PlaySound(sfxItemLegendary);
    else if (rarity >= 3) PlaySound(sfxItemRare);
    else                  PlaySound(sfxPickup);
}
void AudioManager::playPortalOpen()             const { PlaySound(sfxPortal); }
void AudioManager::playPortalClose()            const { PlaySound(sfxPortalClose); }
void AudioManager::playSkillUnlock()            const { PlaySound(sfxSkillUnlock); }
void AudioManager::playAchievement()            const { PlaySound(sfxAchievement); }
void AudioManager::playGeyserErupt()            const { PlaySound(sfxGeyserErupt); }
void AudioManager::playLavaDamage()             const { PlaySound(sfxLavaDmg); }
void AudioManager::playGhostWail()              const { PlaySound(sfxGhostWail); }
void AudioManager::playThunder()                const { PlaySound(sfxThunder); }
void AudioManager::playCritHit()                const { PlaySound(sfxCritHit); }
void AudioManager::playBossPhase()              const { PlaySound(sfxBossPhase); }

// ── Ambient helpers ───────────────────────────────────────────────────────────

Sound AudioManager::makeAmbientWind() {
    const int SR=44100; int N=(int)(SR*1.5f);
    std::vector<float> s(N);
    for(int i=0;i<N;++i){float swell=0.5f+0.5f*std::sin(2*kPI*0.3f*(float)i/SR); s[i]=rnd()*0.35f*swell;}
    Sound snd=sfxSynth(SR,N,s); SetSoundVolume(snd,0.18f); return snd;
}
Sound AudioManager::makeAmbientOwl() {
    const int SR=44100; int N=(int)(SR*0.8f);
    std::vector<float> s(N);
    for(int i=0;i<N;++i){
        float pct=(float)i/N; float t=(float)i/SR;
        float env=pct<0.1f?pct/0.1f:(pct>0.7f?(1.0f-pct)/0.3f:1.0f);
        float vib=1.0f+0.04f*std::sin(2*kPI*6.0f*t);
        float f=440.0f*vib*(pct<0.4f?1.0f:1.0f-0.12f*(pct-0.4f)/0.6f);
        s[i]=(std::sin(2*kPI*f*t)*0.4f+std::sin(2*kPI*f*2.0f*t)*0.08f)*env;
    }
    Sound snd=sfxSynth(SR,N,s); SetSoundVolume(snd,0.20f); return snd;
}
Sound AudioManager::makeAmbientRain() {
    const int SR=44100; int N=(int)(SR*2.0f);
    std::vector<float> s(N);
    for(int i=0;i<N;++i){float swell=0.6f+0.4f*std::sin(2*kPI*0.15f*(float)i/SR); s[i]=rnd()*0.25f*swell;}
    Sound snd=sfxSynth(SR,N,s); SetSoundVolume(snd,0.15f); return snd;
}
Sound AudioManager::makeAmbientLavaBubble() {
    const int SR=44100; int N=(int)(SR*0.6f);
    std::vector<float> s(N);
    for(int i=0;i<N;++i){
        float pct=(float)i/N; float t=(float)i/SR;
        float env=pct<0.15f?pct/0.15f:std::exp(-4.5f*(pct-0.15f));
        s[i]=clamp1(rnd()*0.4f*std::exp(-7.0f*pct)+std::sin(2*kPI*55.0f*t)*0.3f*env);
    }
    Sound snd=sfxSynth(SR,N,s); SetSoundVolume(snd,0.22f); return snd;
}
Sound AudioManager::makeAmbientMachineHum() {
    const int SR=44100; int N=(int)(SR*2.0f);
    std::vector<float> s(N);
    for(int i=0;i<N;++i){
        float t=(float)i/SR; float f=60.0f+2.0f*std::sin(2*kPI*0.2f*t);
        s[i]=std::sin(2*kPI*f*t)*0.22f+std::sin(2*kPI*f*3.0f*t)*0.08f+rnd()*0.04f;
    }
    Sound snd=sfxSynth(SR,N,s); SetSoundVolume(snd,0.12f); return snd;
}
Sound AudioManager::makeAmbientEcho() {
    const int SR=44100; int N=(int)(SR*1.0f);
    std::vector<float> s(N);
    for(int i=0;i<N;++i){
        float pct=(float)i/N; float t=(float)i/SR;
        float env=std::exp(-4.0f*pct); float f=320.0f*std::exp(-2.0f*pct);
        s[i]=(std::sin(2*kPI*f*t)*0.35f+rnd()*0.08f)*env;
    }
    Sound snd=sfxSynth(SR,N,s); SetSoundVolume(snd,0.16f); return snd;
}
Sound AudioManager::makeAmbientCrickets() {
    const int SR=44100; int N=(int)(SR*1.0f);
    std::vector<float> s(N);
    for(int i=0;i<N;++i){
        float t=(float)i/SR;
        float chirp=std::sin(2*kPI*4000.0f*t)*std::sin(2*kPI*12.0f*t);
        s[i]=chirp*(0.5f+0.5f*std::sin(2*kPI*12.0f*t))*0.12f;
    }
    Sound snd=sfxSynth(SR,N,s); SetSoundVolume(snd,0.14f); return snd;
}

void AudioManager::setupAmbientForZone(int zoneId) {
    if (zoneId == lastAmbientZone) return;
    lastAmbientZone = zoneId;
    for(int i=0;i<kAmbSlots;++i){
        if(ambients[i].loaded){UnloadSound(ambients[i].snd); ambients[i].loaded=false;}
        ambients[i].timer=(float)(rand()%4+1);
    }
    switch(zoneId) {
        case 4: case 7:
            ambients[0].snd=makeAmbientWind();    ambients[0].loaded=true; ambients[0].minWait=3; ambients[0].maxWait=8;
            ambients[1].snd=makeAmbientOwl();     ambients[1].loaded=true; ambients[1].minWait=5; ambients[1].maxWait=15;
            ambients[2].snd=makeAmbientCrickets();ambients[2].loaded=true; ambients[2].minWait=2; ambients[2].maxWait=6;
            break;
        case 5: case 9:
            ambients[0].snd=makeAmbientWind();    ambients[0].loaded=true; ambients[0].minWait=4; ambients[0].maxWait=10;
            ambients[1].snd=makeAmbientEcho();    ambients[1].loaded=true; ambients[1].minWait=8; ambients[1].maxWait=18;
            break;
        case 6:
            ambients[0].snd=makeAmbientRain();    ambients[0].loaded=true; ambients[0].minWait=1; ambients[0].maxWait=3;
            ambients[1].snd=makeAmbientEcho();    ambients[1].loaded=true; ambients[1].minWait=4; ambients[1].maxWait=12;
            break;
        case 8:
            ambients[0].snd=makeAmbientEcho();    ambients[0].loaded=true; ambients[0].minWait=3; ambients[0].maxWait=9;
            ambients[1].snd=makeAmbientWind();    ambients[1].loaded=true; ambients[1].minWait=6; ambients[1].maxWait=15;
            break;
        case 10:
            ambients[0].snd=makeAmbientLavaBubble(); ambients[0].loaded=true; ambients[0].minWait=1; ambients[0].maxWait=4;
            ambients[1].snd=makeAmbientLavaBubble(); ambients[1].loaded=true; ambients[1].minWait=2; ambients[1].maxWait=6;
            break;
        case 1: case 2:
            ambients[0].snd=makeAmbientMachineHum(); ambients[0].loaded=true; ambients[0].minWait=2; ambients[0].maxWait=5;
            break;
        default:
            ambients[0].snd=makeAmbientWind();    ambients[0].loaded=true; ambients[0].minWait=5; ambients[0].maxWait=12;
    }
}

void AudioManager::updateAmbient(float dt, int zoneId) {
    setupAmbientForZone(zoneId);
    for(int i=0;i<kAmbSlots;++i){
        if(!ambients[i].loaded) continue;
        ambients[i].timer -= dt;
        if(ambients[i].timer <= 0.0f) {
            PlaySound(ambients[i].snd);
            float range = ambients[i].maxWait - ambients[i].minWait;
            ambients[i].timer = ambients[i].minWait + (float)(rand()%1000)/1000.0f*range;
        }
    }
}

