#include "jv880.h"
#include <algorithm>

JV880_Emu::JV880_Emu() {}

void JV880_Emu::init(uint8_t *rom2) {
  parseRom(rom2);

  // config
  PCM_Write(0x3c, 0xc0);
  PCM_Write(0x3d, 0x7b);

  // voice mask
  PCM_Write(0x00, 0x0f);
  PCM_Write(0x01, 0xff);
  PCM_Write(0x02, 0xff);
  PCM_Write(0x03, 0xff);
  PCM_Read(0x00);

  // shifter
  PCM_Write(0x3e, 0x1e);
  PCM_Write(0x34, 0x00);
  PCM_Write(0x35, 0x01);

  // setup reverb
  PCM_Write(0x3e, 0x1d);
  PCM_Write(0x10, 0x1f);
  PCM_Write(0x11, 0xa6);
  PCM_Write(0x12, 0x23);
  PCM_Write(0x13, 0x02);
  PCM_Write(0x14, 0x25);
  PCM_Write(0x15, 0x6c);
  PCM_Write(0x16, 0x2a);
  PCM_Write(0x17, 0xd5);
  PCM_Write(0x18, 0x2c);
  PCM_Write(0x19, 0xac);
  PCM_Write(0x1a, 0x30);
  PCM_Write(0x1b, 0x39);
  PCM_Write(0x1c, 0x31);
  PCM_Write(0x1d, 0xb9);
  PCM_Write(0x1e, 0x36);
  PCM_Write(0x1f, 0xff);
  PCM_Write(0x30, 0x37);
  PCM_Write(0x31, 0xfe);

  PCM_Write(0x3e, 0x1e);
  PCM_Write(0x10, 0x7f); // tv dest/speed
  PCM_Write(0x11, 0xb0);
  PCM_Write(0x12, 0x03); // param 1
  PCM_Write(0x13, 0x3d);
  PCM_Write(0x14, 0x15); // param 2
  PCM_Write(0x15, 0x00);
  PCM_Write(0x16, 0x15); // param 3
  PCM_Write(0x17, 0x00);
  PCM_Write(0x18, 0x17); // param 4
  PCM_Write(0x19, 0x20);
  PCM_Write(0x1a, 0x00); // param 5
  PCM_Write(0x1b, 0x20);
  PCM_Write(0x1c, 0x1b); // param 6
  PCM_Write(0x1d, 0xe5);
  PCM_Write(0x1e, 0x1f); // param 7
  PCM_Write(0x1f, 0x1e);
  PCM_Write(0x30, 0x21); // param 8
  PCM_Write(0x31, 0x18);

  // setup chorus
  PCM_Write(0x3e, 0x1f);
  PCM_Write(0x09, 0x00); // address loop
  PCM_Write(0x0a, 0x3f);
  PCM_Write(0x0b, 0x40);
  PCM_Write(0x0d, 0x00); // address end
  PCM_Write(0x0e, 0x3f);
  PCM_Write(0x0f, 0x84);
  PCM_Write(0x05, 0x00); // address
  PCM_Write(0x06, 0x3f);
  PCM_Write(0x07, 0x41);
  PCM_Write(0x10, 0x00); // pitch
  PCM_Write(0x11, 0x58);
  PCM_Write(0x12, 0x00); // param 1
  PCM_Write(0x13, 0x00);
  PCM_Write(0x14, 0x00); // param 2
  PCM_Write(0x15, 0x00);
  PCM_Write(0x16, 0x00); // param 3
  PCM_Write(0x17, 0x00);
  PCM_Write(0x18, 0x00); // param 4
  PCM_Write(0x19, 0x00);
  PCM_Write(0x1a, 0x00); // param 5
  PCM_Write(0x1b, 0x00);
  PCM_Write(0x1e, 0x00); // high addr / nibble
  PCM_Write(0x1f, 0x7f); // subphase addr / key
  PCM_Write(0x30, 0x00); // tv counter / subphase state
  PCM_Write(0x31, 0x00);

  // write
  // 1d: ram2(00,01,02,03,04,05,06,07,08,09)
  // 1e: ram2(00,01,02,03,04,05,06,07,08,0a)
  // 1f: ram1(00,02,04) ram2(00,01,02,03,04,05,07,08)

  // read
  // 1e: ram2(0a) (many times)
  // 1f: ram1(04) (single time)

  // REVERB
  // ram2 1d 00: reverb eram address
  // ram2 1d 01: reverb eram address
  // ram2 1d 02: reverb eram address
  // ram2 1d 03: reverb eram address
  // ram2 1d 04: reverb eram address
  // ram2 1d 05: reverb eram address
  // ram2 1d 06: reverb eram address
  // ram2 1d 07: reverb eram address
  // ram2 1d 08: reverb eram address
  // ram2 1d 09: reverb eram address
  // ram2 1d 0a: chorus eram address (internal only)
  // ram2 1d 0b: chorus eram address (internal only)

  // REVERB
  // ram2 1e 00: some tv dest
  // ram2 1e 01: multipler
  // ram2 1e 02: multiplier
  // ram2 1e 03: multiplier
  // ram2 1e 04: multiplier
  // ram2 1e 05: multiplier
  // ram2 1e 06: multiplier
  // ram2 1e 07: multiplier
  // ram2 1e 08: multiplier
  // ram2 1e 09: dest for tv (internal only)
  // ram2 1e 0a: shifter (set once to 1, readt many times)

  // CHORUS
  // ram1 1f 00: address end
  // ram1 1f 01: sum l (internal only)
  // ram1 1f 02: address loop
  // ram1 1f 03: sum r (internal only)
  // ram1 1f 04: address
  // ram2 1f 00: chorus pitch
  // ram2 1f 01: multiplier
  // ram2 1f 02: multiplier
  // ram2 1f 03: multiplier
  // ram2 1f 04: multiplier
  // ram2 1f 05: multiplier
  // ram2 1f 07: chorus subphase addr/key/loop/hiaddr/nibble (set once to 7f)
  // ram2 1f 08: tv counter/subphase state (set once to 0)
}

void JV880_Emu::update(uint64_t cycles) {
  this->cycles = cycles;

  if (!midiQueue.empty()) {
    uint8_t midiByte = midiQueue.front();
    if (sysexMode && midiByte == 0xf7) {
      midiQueue.pop();
      sysexMode = false;
    } else if (sysexMode || (midiByte & 0xf0) == 0xf0) {
      midiQueue.pop();
    } else if (midiByte == 0xf0) {
      midiQueue.pop();
      sysexMode = true;
    }

    // Note on
    if ((midiByte & 0xf0) == 0x90 && midiQueue.size() >= 3) {
      uint8_t channel = midiByte & 0xf;
      midiQueue.pop();
      uint8_t note = midiQueue.front();
      midiQueue.pop();
      uint8_t velocity = midiQueue.front();
      midiQueue.pop();

      if (velocity == 0) {
        noteOff(channel, note, velocity);
      } else {
        noteOn(channel, note, velocity);
      }
    }

    // Note off
    if ((midiByte & 0xf0) == 0x80 && midiQueue.size() >= 3) {
      uint8_t channel = midiByte & 0xf;
      midiQueue.pop();
      uint8_t note = midiQueue.front();
      midiQueue.pop();
      uint8_t velocity = midiQueue.front();
      midiQueue.pop();

      noteOff(channel, note, velocity);
    }

    // Aftertouch
    if ((midiByte & 0xf0) == 0xa0 && midiQueue.size() >= 3) {
      uint8_t channel = midiByte & 0xf;
      midiQueue.pop();
      uint8_t note = midiQueue.front();
      midiQueue.pop();
      uint8_t touch = midiQueue.front();
      midiQueue.pop();

      // TODO
    }

    // Control change
    if ((midiByte & 0xf0) == 0xb0 && midiQueue.size() >= 3) {
      uint8_t channel = midiByte & 0xf;
      midiQueue.pop();
      uint8_t controller = midiQueue.front();
      midiQueue.pop();
      uint8_t value = midiQueue.front();
      midiQueue.pop();

      // TODO
    }

    // Patch change
    if ((midiByte & 0xf0) == 0xc0 && midiQueue.size() >= 3) {
      uint8_t channel = midiByte & 0xf;
      midiQueue.pop();
      uint8_t patch = midiQueue.front();
      midiQueue.pop();

      // TODO
    }

    // Channel pressure
    if ((midiByte & 0xf0) == 0xd0 && midiQueue.size() >= 3) {
      uint8_t channel = midiByte & 0xf;
      midiQueue.pop();
      uint8_t pressure = midiQueue.front();
      midiQueue.pop();

      // TODO
    }

    // Pitch bend
    if ((midiByte & 0xf0) == 0xe0 && midiQueue.size() >= 3) {
      uint8_t channel = midiByte & 0xf;
      midiQueue.pop();
      uint8_t bendA = midiQueue.front();
      midiQueue.pop();
      uint8_t bendB = midiQueue.front();
      midiQueue.pop();

      // TODO
    }
  }

  for (size_t voiceI = 0; voiceI < jv880_voices_count; voiceI++) {
    VoiceStatus *v = &voiceStatus[voiceI];
    Tone *tone = v->tone;
    if (!voiceStatus[voiceI].keyOn)
      continue;

    // TVF
    if (v->tvfEnvStage == 0 && cycles > v->tvfEnvNextStageCycles) {
      printf("tvfEnvStage 1\n");
      v->tvfEnvStage = 1;
      v->tvfEnvNextStageCycles = cycles + tone->tvfEnvTime2 * timeScaleFactor;

      uint8_t cutoff = std::min(
          (uint32_t)0x7f,
          tone->tvfCutoff + (uint32_t)(tone->tvfEnvDepth *
                                       ((float)tone->tvfEnvLevel2 / 0x80)));

      PCM_Write(0x3e, voiceI);
      PCM_Write(0x1a, cutoff);                   // lpf cutoff dest
      PCM_Write(0x1b, 0x7f - tone->tvfEnvTime2); // lpf cutoff speed

    } else if (v->tvfEnvStage == 1 && cycles > v->tvfEnvNextStageCycles) {
      printf("tvfEnvStage 2\n");
      v->tvfEnvStage = 2;
      v->tvfEnvNextStageCycles = cycles + tone->tvfEnvTime3 * timeScaleFactor;

      uint8_t cutoff = std::min(
          (uint32_t)0x7f,
          tone->tvfCutoff + (uint32_t)(tone->tvfEnvDepth *
                                       ((float)tone->tvfEnvLevel3 / 0x80)));

      PCM_Write(0x3e, voiceI);
      PCM_Write(0x1a, cutoff);                   // lpf cutoff dest
      PCM_Write(0x1b, 0x7f - tone->tvfEnvTime3); // lpf cutoff speed
    } else if (v->tvfEnvStage == 2 && cycles > v->tvfEnvNextStageCycles) {
      printf("tvfEnvStage 3\n");
      v->tvfEnvStage = 3;
      v->tvfEnvNextStageCycles = cycles + tone->tvfEnvTime3 * timeScaleFactor;

      uint8_t cutoff = std::min(
          (uint32_t)0x7f,
          tone->tvfCutoff + (uint32_t)(tone->tvfEnvDepth *
                                       ((float)tone->tvfEnvLevel4 / 0x80)));

      // PCM_Write(0x3e, voiceI);
      // PCM_Write(0x1a, cutoff);            // lpf cutoff dest
      // PCM_Write(0x1b, 0x7f - tone->tvfEnvTime4); // lpf cutoff speed
    }

    // TVA
    if (v->tvaEnvStage == 0 && cycles > v->tvaEnvNextStageCycles) {
      printf("tvaEnvStage 1\n");
      v->tvaEnvStage = 1;
      v->tvaEnvNextStageCycles = cycles + tone->tvaEnvTime2 * timeScaleFactor;

      PCM_Write(0x3e, voiceI);
      PCM_Write(0x16, tone->tvaEnvLevel2);       // volume1 dest
      PCM_Write(0x17, 0x7f - tone->tvaEnvTime2); // volume1 speed

    } else if (v->tvaEnvStage == 1 && cycles > v->tvaEnvNextStageCycles) {
      printf("tvaEnvStage 2\n");
      v->tvaEnvStage = 2;
      v->tvaEnvNextStageCycles = cycles + tone->tvaEnvTime3 * timeScaleFactor;

      PCM_Write(0x3e, voiceI);
      PCM_Write(0x16, tone->tvaEnvLevel3);       // volume1 dest
      PCM_Write(0x17, 0x7f - tone->tvaEnvTime3); // volume1 speed
    } else if (v->tvaEnvStage == 2 && cycles > v->tvaEnvNextStageCycles) {
      printf("tvaEnvStage 3\n");
      v->tvaEnvStage = 3;
      v->tvaEnvNextStageCycles = cycles + tone->tvaEnvTime3 * timeScaleFactor;

      // PCM_Write(0x3e, voiceI);
      // PCM_Write(0x16, 0x00);              // volume1 dest
      // PCM_Write(0x17, 0x7f - tone->tvaEnvTime4); // volume1 speed
    }
  }
}

void JV880_Emu::midiRx(uint8_t midiByte) { midiQueue.push(midiByte); }

void JV880_Emu::parseRom(uint8_t *rom2) {
  romInfo.waveformsCount = rom2[0] | (rom2[1] << 8);
  romInfo.samplesTablePtr = rom2[2] | (rom2[3] << 8);

  for (size_t i = 0; i < jv880_waveform_count; i++) {
    memcpy(&romInfo.waveforms[i].name[0], &rom2[4 + i * 60], 12);
    for (size_t j = 0; j < 16; j++) {
      romInfo.waveforms[i].maxNote[j] = rom2[4 + i * 60 + 12 + j];
      romInfo.waveforms[i].sampleId[j] =
          rom2[4 + i * 60 + 12 + 16 + j * 2 + 1] |
          (rom2[4 + i * 60 + 12 + 16 + j * 2] << 8);
    }
  }

  for (size_t i = 0; i < jv880_samples_count; i++) {
    size_t ofs = 4 + jv880_waveform_count * 60 + i * 18;
    romInfo.samples[i].volume = rom2[ofs];
    ofs += 1;
    romInfo.samples[i].startAddr =
        rom2[ofs + 2] | rom2[ofs + 1] << 8 | rom2[ofs + 0] << 16;
    ofs += 3;
    romInfo.samples[i].loopAddr =
        rom2[ofs + 2] | rom2[ofs + 1] << 8 | rom2[ofs + 0] << 16;
    ofs += 3;
    romInfo.samples[i].endAddr =
        rom2[ofs + 2] | rom2[ofs + 1] << 8 | rom2[ofs + 0] << 16;
    ofs += 3;
    romInfo.samples[i].unknown = rom2[ofs + 1] | rom2[ofs] << 8;
    ofs += 2;
    romInfo.samples[i].loopType = rom2[ofs];
    ofs += 1;
    romInfo.samples[i].rootKey = rom2[ofs];
    ofs += 1;
    romInfo.samples[i].fineTune = rom2[ofs + 1] | rom2[ofs] << 8;
    ofs += 2;
    romInfo.samples[i].loopFineTune = rom2[ofs + 1] | rom2[ofs] << 8;
  }

  memcpy(&romInfo.patchesUser, &rom2[0x008000 + 32 + 204 * 16],
         sizeof(romInfo.patchesUser));
  memcpy(&romInfo.patchesPresetA, &rom2[0x010000 + 32 + 204 * 16],
         sizeof(romInfo.patchesUser));
  memcpy(&romInfo.patchesPresetB, &rom2[0x018000 + 32 + 204 * 16],
         sizeof(romInfo.patchesUser));
}

void JV880_Emu::noteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  uint8_t patchI = 2;
  Patch *patch = &romInfo.patchesUser[patchI];
  // Patch *patch = &romInfo.patchesPresetA[patchI];

  for (size_t toneI = 0; toneI < 4; toneI++) {
    Tone *tone = &patch->tones[toneI];
    if ((tone->flags & 0x80) == 0x0)
      break;

    uint8_t voice = getNextFreeVoice();
    takeNote(voice, note, patch, tone);
    voiceStatus[voice].tvfEnvStage = 0;
    voiceStatus[voice].tvfEnvNextStageCycles =
        cycles + tone->tvfEnvTime1 * timeScaleFactor;
    voiceStatus[voice].tvaEnvStage = 0;
    voiceStatus[voice].tvaEnvNextStageCycles =
        cycles + tone->tvaEnvTime1 * timeScaleFactor;

    // TODO: Is the sample fetched before or after coarse tuning?
    uint8_t toneNote = note + tone->pitchCoarse;

    Waveform waveform = romInfo.waveforms[tone->waveNumber];
    int sampleI = waveform.sampleId[0];
    for (size_t i = 0; i < 16; i++) {
      sampleI = waveform.sampleId[i];
      if (waveform.maxNote[i] >= toneNote)
        break;
    }

    Sample sample = romInfo.samples[sampleI];

    // TODO: sample fine correct
    // pitch 0x4000 is unit
    float pitchFloat = (float)0x4000 * (pow(2, (float)toneNote / 12) /
                                        pow(2, (float)sample.rootKey / 12));
    uint8_t pitchCoarse = (uint32_t)pitchFloat >> 8;
    uint8_t pitchFine = (uint32_t)pitchFloat & 0xff;

    PCM_Write(0x3e, voice);

    PCM_Write(0x05, (sample.startAddr >> 16) & 0xff); // address
    PCM_Write(0x06, (sample.startAddr >> 8) & 0xff);
    PCM_Write(0x07, (sample.startAddr >> 0) & 0xff);
    PCM_Write(0x09, (sample.loopAddr >> 16) & 0xff); // address loop
    PCM_Write(0x0a, (sample.loopAddr >> 8) & 0xff);
    PCM_Write(0x0b, (sample.loopAddr >> 0) & 0xff);
    PCM_Write(0x0d, (sample.endAddr >> 16) & 0xff); // address end
    PCM_Write(0x0e, (sample.endAddr >> 8) & 0xff);
    PCM_Write(0x0f, (sample.endAddr >> 0) & 0xff);

    uint8_t level = tone->tvaLevel;
    uint8_t pan = tone->tvaPan;
    if (pan == 0x7f) {
      pan = rand() % 0x7f;
    }
    uint8_t levelL = level - (pan < 0x40 ? (0x80 - (0x40 - pan) * 2) : 0x00);
    uint8_t levelR = level - (pan >= 0x40 ? (0x80 - (pan - 0x40) * 2) : 0x00);

    PCM_Write(0x10, pitchCoarse);               // pitch coarse
    PCM_Write(0x11, pitchFine);                 // pitch fine
    PCM_Write(0x12, levelL);                    // level l
    PCM_Write(0x13, levelR);                    // level r
    PCM_Write(0x14, 0x00);                      // reverb send
    PCM_Write(0x15, 0x00);                      // chorus send
    PCM_Write(0x16, 0xff);                      // volume1 dest
    PCM_Write(0x17, 0x7f);                      // volume1 speed
    PCM_Write(0x18, 0x7f);                      // volume2 dest
    PCM_Write(0x19, 0x7f);                      // volume2 speed
    PCM_Write(0x1a, tone->tvfCutoff);           // lpf cutoff dest
    PCM_Write(0x1b, 0x7f);                      // lpf cutoff speed
    PCM_Write(0x1c, 0x40 - tone->tvfResonance); // resonance
    PCM_Write(0x1d, 0x00); // filter mode (0x=lpf, 1x=hpf), irq (0=off, 1=on)
    // 00 soft: 0x40 0x00
    // 01 soft: 0x3f 0x01
    // 02 soft: 0x3e 0x02
    // 03 soft: 0x3d 0x03
    // 04 soft: 0x3d 0x03
    // 05 soft: 0x3c 0x04
    // 06 soft: 0x3b 0x05
    // 07 soft: 0x3b 0x05
    // 08 soft: 0x3a 0x06
    // 09 soft: 0x3a 0x06
    // 0a soft: 0x39 0x07
    // 20 soft: 0x2d 0x13
    // 40 soft: 0x20 0x20
    // 60 soft: 0x16 0x2a
    // 70 soft: 0x13 0x2d
    // 7f soft: 0x10 0x30

    // 00 hard: 0x40
    // 01 hard: 0x3e
    // 02 hard: 0x3d
    // 03 hard: 0x3b
    // 04 hard: 0x3a
    // 05 hard: 0x39
    // 06 hard: 0x38
    // 07 hard: 0x36
    // 08 hard: 0x35
    // 09 hard: 0x34
    // 0a hard: 0x33
    // 20 hard: 0x20
    // 40 hard: 0x10
    // 60 hard: 0x08
    // 70 hard: 0x05
    // 71 hard: 0x05
    // 72 hard: 0x05
    // 73 hard: 0x05
    // 74 hard: 0x05
    // 75 hard: 0x05
    // 76 hard: 0x04
    // 77 hard: 0x04
    // 78 hard: 0x04
    // 79 hard: 0x04
    // 7a hard: 0x04
    // 7b hard: 0x04
    // 7c hard: 0x04
    // 7d hard: 0x04
    // 7e hard: 0x04
    // 7f hard: 0x04

    uint8_t nibble = 0;
    PCM_Write(0x1e, (nibble << 4) | (sample.startAddr >> 20));

    uint8_t key = 0;
    uint8_t subPhaseAddr = voice;
    PCM_Write(0x1f, subPhaseAddr | ((sample.loopType & 1) << 6) | (key << 5));

    uint8_t cutoff = std::min(
        (uint32_t)0x7f,
        tone->tvfCutoff +
            (uint32_t)(tone->tvfEnvDepth * ((float)tone->tvfEnvLevel1 / 0x80)));
    PCM_Write(0x1a, cutoff);                   // lpf cutoff dest
    PCM_Write(0x1b, 0x7f - tone->tvfEnvTime1); // lpf cutoff speed

    PCM_Write(0x16, tone->tvaEnvLevel1);       // volume1 dest
    PCM_Write(0x17, 0x7f - tone->tvaEnvTime1); // volume1 speed
  }
}

void JV880_Emu::noteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  for (size_t i = 0; i < jv880_voices_count; i++) {
    if (voiceStatus[i].midiNote == note) {
      PCM_Write(0x3e, i);
      PCM_Write(0x18, 0x00); // volume2 dest
      PCM_Write(0x19, 0x50); // volume2 speed

      // TODO: tail
      voiceStatus[i].keyOn = false;
    }
  }
}

uint8_t JV880_Emu::getNextFreeVoice() {
  uint8_t voice = 0;
  for (size_t i = 0; i < jv880_voices_count; i++) {
    if (!voiceStatus[i].keyOn) {
      voice = i;
      break;
    }
  }
  return voice;
}

void JV880_Emu::takeNote(uint8_t voice, uint8_t midiNote, Patch *patch,
                         Tone *tone) {
  voiceStatus[voice].keyOn = true;
  voiceStatus[voice].tail = false;
  voiceStatus[voice].midiNote = midiNote;
  voiceStatus[voice].patch = patch;
  voiceStatus[voice].tone = tone;
}
