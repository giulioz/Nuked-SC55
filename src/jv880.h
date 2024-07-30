#pragma once

#include "mcu.h"
#include "pcm.h"

#include <queue>

#define jv880_voices_count 28
#define jv880_waveform_count 129
#define jv880_samples_count 577
#define timeScaleFactor 80000

struct Sample {
  uint8_t volume;     // max:7f
  uint32_t startAddr; // 3 bytes
  uint32_t loopAddr;  // 3 bytes
  uint32_t endAddr;   // 3 bytes
  uint16_t unknown;
  uint8_t loopType; // Known values: 0x00 (no loop), 0x01 (forward loop), 0x02
                    // (no loop), 0x06 (reverse, non-JD only)
  uint8_t rootKey;
  uint16_t fineTune;     // 1/1024ths of a semitone
  uint16_t loopFineTune; // 1/1024ths of a semitone
};                       // 18 bytes

struct Waveform {
  char name[12];
  uint8_t maxNote[16];
  uint16_t sampleId[16];
}; // 60 bytes

#pragma pack(push, 1)
struct Tone {
  uint8_t flags;
  // 0-1: wave group
  // 7: tone switch
  uint8_t waveNumber;
  uint8_t fxmConfig; // 7:enable low:depth
  uint8_t velocityRangeLow;
  uint8_t velocityRangeUp;
  uint8_t unk_0x05;
  uint8_t unk_0x06;
  uint8_t unk_0x07;
  uint8_t unk_0x08;
  uint8_t unk_0x09;
  uint8_t unk_0x0A;
  uint8_t unk_0x0B;
  uint8_t unk_0x0C;
  uint8_t unk_0x0D;
  uint8_t unk_0x0E;
  uint8_t unk_0x0F;
  uint8_t unk_0x10;
  uint8_t unk_0x11;
  uint8_t unk_0x12;
  uint8_t unk_0x13;
  uint8_t unk_0x14;
  uint8_t unk_0x15;
  uint8_t unk_0x16;
  uint8_t lfo1Flags;
  // 0-1-2: waveform
  // 3-4-5: offset
  // 6: synchro
  // 7: fade in/out
  uint8_t lfo1Rate;
  uint8_t lfo1Delay;
  uint8_t lfo1Fade;
  uint8_t lfo2Flags;
  uint8_t lfo2Rate;
  uint8_t lfo2Delay;
  uint8_t lfo2Fade;
  uint8_t lfo1PitchDepth;
  uint8_t lfo1TvfDepth;
  uint8_t lfo1TvaDepth;
  uint8_t lfo2PitchDepth;
  uint8_t lfo2TvfDepth;
  uint8_t lfo2TvaDepth;
  int8_t pitchCoarse;
  int8_t pitchFine;
  uint8_t tvaPanningKFRandomPitch; // high:tva panning low:random pitch
  uint8_t tvpTimeKFKeyfollow;
  uint8_t tvpVelocity;
  uint8_t tvpT1T4Velocity;
  uint8_t tvpEnvDepth;
  uint8_t tvpEnvTime1;
  uint8_t tvpEnvLevel1;
  uint8_t tvpEnvTime2;
  uint8_t tvpEnvLevel2;
  uint8_t tvpEnvTime3;
  uint8_t tvpEnvLevel3;
  uint8_t tvpEnvTime4;
  uint8_t tvpEnvLevel4;
  uint8_t tvfCutoff;
  uint8_t tvfResonance;       // 7: soft/hard
  uint8_t tvfTimeKFKeyfollow; // check
  uint8_t tvfVeloCurveLpfHpf; // (check) 3-4: LPF/HPF
  uint8_t tvfVelocity;        // check
  uint8_t tvfT1T4Velocity;
  uint8_t tvfEnvDepth;
  uint8_t tvfEnvTime1;
  uint8_t tvfEnvLevel1;
  uint8_t tvfEnvTime2;
  uint8_t tvfEnvLevel2;
  uint8_t tvfEnvTime3;
  uint8_t tvfEnvLevel3;
  uint8_t tvfEnvTime4;
  uint8_t tvfEnvLevel4;
  uint8_t tvaLevel;
  uint8_t tvaPan;
  uint8_t unk_0x45;
  uint8_t tvaTimeKFDelayTimeKeyfollow; // (check again) low:key follow
  uint8_t tvaDelayModeVeloCurve;       // low:velo curve
  uint8_t tvaVelocity;
  uint8_t tvaT1T4Velocity; // check again
  uint8_t tvaEnvTime1;
  uint8_t tvaEnvLevel1;
  uint8_t tvaEnvTime2;
  uint8_t tvaEnvLevel2;
  uint8_t tvaEnvTime3;
  uint8_t tvaEnvLevel3;
  uint8_t tvaEnvTime4;
  uint8_t drySend;
  uint8_t reverbSend;
  uint8_t chorusSend;
} __attribute__((__packed__)); // 84 bytes
#pragma pack(pop)

#pragma pack(push, 1)
struct Patch {
  char name[12];
  uint8_t recChorConfig;
  // 0-3: rev type
  // 4-5: chorus type
  // 6:   ??
  // 7:   velocity switch
  uint8_t reverbLevel;
  uint8_t reverbTime;
  uint8_t reverbFeedback;
  uint8_t chorusLevel; // 7: chorus mode
  uint8_t chorusDepth;
  uint8_t chorusRate;
  uint8_t chorusFeedback;
  uint8_t analogFeel;
  uint8_t level;
  uint8_t pan;
  uint8_t bendRange;
  uint8_t flags;
  // 0: ??
  // 1: ??
  // 2: ??
  // 3: ??
  // 4: portamento mode
  // 5: solo legato
  // 6: portamento switch
  // 7: key assign
  uint8_t portamentoTime; // 7: portamento type
  Tone tones[4];
} __attribute__((__packed__)); // 362 bytes
#pragma pack(pop)

struct Performance {
  char name[12];
  uint8_t data[192];
}; // 204 bytes

struct ROM2 {
  // 0x000000
  uint16_t waveformsCount;
  uint16_t samplesTablePtr;
  Waveform waveforms[jv880_waveform_count];
  Sample samples[jv880_samples_count];

  // 0x008000
  char introStringUser[32]; // "Roland JV-80N±P¯JV-80 Initialize" 0x4EB1 0x50AF
  Performance performancesUser[16];
  Patch patchesUser[64];
  uint8_t drumKitUser[6304];

  // 0x010000
  char introStringPresetA[32]; // "Roland JV-80D»P¯JV-80 Preset A  " 0x44BB
                               // 0x50AF
  Performance performancesPresetA[16];
  Patch patchesPresetA[64];
  uint8_t drumKitPresetA[6304];

  // 0x018000
  char introStringPresetB[32]; // "Roland JV-80D»P¯JV-80 Preset B  " 0x44BB
                               // 0x50AF
  Performance performancesPresetB[16];
  Patch patchesPresetB[64];
  uint8_t drumKitPresetB[6304];

  // 0x020000
  uint32_t demoSongPtr[16]; // relative to 0x020000

  // 0x039811
  Performance initPerformance;
  Patch initPatch;

  // Tables:
  // 0x04c52
  // 0x05160
  // 0x05718
  // 0x05728
  // 0x0572c
  // 0x05730
  // 0x05768
  // 0x05784
  // 0x0579e
  // 0x057be
  // 0x057de
  // 0x057fe
  // 0x0581c down ramp (16bit) pitch?
  // 0x0583e
  // 0x05860 ramp split (16bit) pitch?
  // 0x05c60
  // 0x05e60
  // 0x06160
  // 0x06260
  // 0x06360
  // 0x06460 ramp split (256x16bit) pitch?
  // 0x06560 slow ramp (128x16bit)
  // 0x06660 ramp (16bit) pitch?
  // 0x0688a
  // 0x06a8a
  // 0x06b8a

  // 0x30000-0x30043
  // 0x392e6-0x392f3
  // 0x394d8
  // 0x396ac-0x39cd4
  // 0x3a402-0x3a404
  // 0x3a930-0x3a931
};

struct VoiceStatus {
  bool keyOn;
  bool tail;
  uint8_t midiNote;
  Patch *patch;
  Tone *tone;

  uint8_t tvaEnvStage;
  uint8_t tvfEnvStage;
  uint8_t tvpEnvStage;
  uint64_t tvaEnvNextStageCycles;
  uint64_t tvfEnvNextStageCycles;
  uint64_t tvpEnvNextStageCycles;
};

class JV880_Emu {
public:
  JV880_Emu();
  void init(uint8_t *rom2);
  void update(uint64_t cycles);
  void midiRx(uint8_t midiByte);

private:
  void parseRom(uint8_t *rom2);
  void noteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  void noteOff(uint8_t channel, uint8_t note, uint8_t velocity);

  ROM2 romInfo;
  Patch currentPatch;

  std::queue<uint8_t> midiQueue;
  bool sysexMode = false;

  uint64_t cycles = 0;
  VoiceStatus voiceStatus[jv880_voices_count] = {0};
  uint8_t getNextFreeVoice();
  void takeNote(uint8_t voice, uint8_t midiNote, Patch *patch, Tone *tone);
};
