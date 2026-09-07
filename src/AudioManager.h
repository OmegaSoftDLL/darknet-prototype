#pragma once
#include <raylib.h>
#include <vector>
#include "Zone.h"

// Per-zone ambient sound slot
struct AmbientSlot {
    Sound snd;
    bool  loaded   = false;
    float timer    = 0.0f;
    float minWait  = 2.0f;
    float maxWait  = 8.0f;
    float volume   = 0.18f;
};

class AudioManager {
public:
    // ── SFX ──────────────────────────────────────────────────────────────────
    Sound sfxLaser;
    Sound sfxEMP;
    Sound sfxHit;
    Sound sfxHitHeavy;
    Sound sfxHitAlien;
    Sound sfxExplosion;
    Sound sfxExplosionBig;
    Sound sfxPickup;
    Sound sfxPickupCredits;
    Sound sfxLevelUp;
    Sound sfxShield;
    Sound sfxPortal;
    Sound sfxPortalClose;
    Sound sfxFootstep;
    Sound sfxMeleeSwing;
    Sound sfxMeleeImpact;
    Sound sfxAlienScream;
    Sound sfxBossRoar;
    Sound sfxChargeUp;
    Sound sfxBurst;
    Sound sfxRicochets[3];

    // ── New SFX ──────────────────────────────────────────────────────────────
    Sound sfxPlayerHurt;      // pain yelp
    Sound sfxPlayerDeath;     // tragic descend
    Sound sfxEvolve;          // transformation sweep
    Sound sfxHeal;            // positive chime
    Sound sfxItemRare;        // 3-harmonic chord
    Sound sfxItemLegendary;   // 4-note epic fanfare
    Sound sfxSkillUnlock;     // ascending unlock
    Sound sfxAchievement;     // 3-note triumph
    Sound sfxLavaBubble;      // gloopy bubble
    Sound sfxGeyserErupt;     // hissing burst
    Sound sfxLavaDmg;         // sizzle hit
    Sound sfxGhostWail;       // ghost zone ambiance hit
    Sound sfxThunder;         // storm crack
    Sound sfxCritHit;         // critical hit crack
    Sound sfxBossPhase;       // boss phase transition
    Sound sfxPortalSuck;      // anomaly portal suction hum
    Sound sfxDeathCry;        // grito of voz of the character to the die

    // ── Controles of mute (menu) ──────────────────────────────────────────────
    bool musicEnabled = true;   // trilha sonora
    bool allSoundOn   = true;   // all the sounds (master)
    bool voiceEnabled = true;   // sounds/vozes of character
    void setMusicEnabled(bool b);
    void setAllSoundOn(bool b);
    void setVoiceEnabled(bool b) { voiceEnabled = b; }
    void playDeathCry() const;  // grito "estou morrendo" sintetizado

    // ── Music (expanded to all 11 zones + menu) ──────────────────────────────
    static constexpr int kNumZones = 11;
    Music bgMusic[kNumZones];
    bool  musicLoaded[kNumZones];
    Music menuMusic;
    bool  menuMusicLoaded = false;
    ZoneID currentMusicZone = ZoneID::LARuins;
    bool  inCombat  = false;
    float combatTimer = 0.0f;

    // ── Ambient system ────────────────────────────────────────────────────────
    static constexpr int kAmbSlots = 4;
    AmbientSlot ambients[kAmbSlots];
    int   lastAmbientZone = -1;

    // ── Core ──────────────────────────────────────────────────────────────────
    void init();
    void shutdown();
    void updateMusic();
    void updateAmbient(float dt, int zoneId);
    void setZone(ZoneID zone);
    void setZone(int zoneId) { setZone((ZoneID)zoneId); }
    void setCombat(bool fighting);
    void playMenuMusic();
    void stopMenuMusic();

    // ── Legacy play methods ───────────────────────────────────────────────────
    void playLaser()         const;
    void playEMP()           const;
    void playHit()           const;
    void playHitHeavy()      const;
    void playHitAlien()      const;
    void playExplosion()     const;
    void playExplosionBig()  const;
    void playPickup()        const;
    void playPickupCredits() const;
    void playLevelUp()       const;
    void playShield()        const;
    void playPortal()        const;
    void playFootstep()      const;
    void playMeleeSwing()    const;
    void playMeleeImpact()   const;
    void playAlienScream()   const;
    void playBossRoar()      const;
    void playChargeUp()      const;
    void playBurst()         const;
    void playRicochet()      const;

    // ── New contextual play methods ───────────────────────────────────────────
    void playMeleeHit(bool isCrit = false) const;
    void playPlasmaShot()         const;
    void playShotgun()            const;
    void playExplosion(bool large) const;
    void playEnemyHit()           const;
    void playEnemyDeath(bool isBoss = false) const;
    void playPlayerHurt()         const;
    void playPlayerDeath()        const;
    void playEvolve()             const;
    void playHeal()               const;
    void playItemPickup(int rarityLevel) const; // 0-5
    void playPortalOpen()         const;
    void playPortalClose()        const;
    void playSkillUnlock()        const;
    void playAchievement()        const;
    void playGeyserErupt()        const;
    void playLavaDamage()         const;
    void playGhostWail()          const;
    void playThunder()            const;
    void playCritHit()            const;
    void playBossPhase()          const;

private:
    std::vector<unsigned char> musicBuffers[kNumZones];
    std::vector<unsigned char> menuMusicBuffer;

    Sound generateTone(float duration, float frequency,
                       bool noise = false, float pitchSweep = 0.0f,
                       float attack = 0.01f, float release = 0.8f);
    Sound generateComplex(float duration, int sampleRate,
                          std::vector<float> freqs, std::vector<float> amps,
                          float pitchSweep, bool addNoise, float noiseAmt);
    std::vector<unsigned char> createWavBuffer(float duration, float freq,
                                               bool noise, float pitchSweep,
                                               float attack, float release);
    std::vector<unsigned char> buildMusicForZone(ZoneID zone, float duration, int SR);

    // Zone music synths
    std::vector<short> synthMenu(int SR, int N);
    std::vector<short> synthLARuins(int SR, int N);
    std::vector<short> synthBunker(int SR, int N);
    std::vector<short> synthFactory(int SR, int N);
    std::vector<short> synthCore(int SR, int N);
    std::vector<short> synthCemetery(int SR, int N);
    std::vector<short> synthCursedFarm(int SR, int N);
    std::vector<short> synthGhostCity(int SR, int N);
    std::vector<short> synthDarkForest(int SR, int N);
    std::vector<short> synthCatacombs(int SR, int N);
    std::vector<short> synthManor(int SR, int N);
    std::vector<short> synthInferno(int SR, int N);

    // Ambient generators
    Sound makeAmbientWind();
    Sound makeAmbientOwl();
    Sound makeAmbientRain();
    Sound makeAmbientLavaBubble();
    Sound makeAmbientMachineHum();
    Sound makeAmbientEcho();
    Sound makeAmbientCrickets();
    void  setupAmbientForZone(int zoneId);
    // Note: thunder ambient reuses sfxThunder via playThunder(); in the dedicated ambient method needed
};
