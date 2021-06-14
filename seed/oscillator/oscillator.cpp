#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed  seed;
static Oscillator osc;
static Adsr adsr;

size_t tick = 0;

#define NUM_NOTES 4
float notes[] = {
    261.62, 329.63, 392, 493.88
};
size_t note_tick = 0;

#define SPEED 0.5

#define SAMPLES_PER_NOTE 48000 * SPEED
#define SUSTAIN SAMPLES_PER_NOTE * 0.25

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;
    for(size_t i = 0; i < size; i += 2)
    {
        bool trig = false;
        if (tick++ >= SAMPLES_PER_NOTE)
        {
            osc.SetFreq(notes[note_tick]);
            trig = true;
            if (tick > SAMPLES_PER_NOTE + SUSTAIN)
            {
                tick = 0;
                note_tick = (note_tick + 1) % NUM_NOTES;
            }
        }
        sig = osc.Process();
        float env = adsr.Process(trig);

        // left out
        out[i] = sig * env;

        // right out
        out[i + 1] = sig * env;
    }
}

int main(void)
{
    // initialize seed hardware and oscillator daisysp module
    float sample_rate;
    seed.Configure();
    seed.Init();
    sample_rate = seed.AudioSampleRate();
    osc.Init(sample_rate);

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SAW);
    osc.SetFreq(440);
    osc.SetAmp(0.75);

    adsr.Init(sample_rate);
    adsr.SetSustainLevel(0.5);
    adsr.SetTime(ADSR_SEG_ATTACK, 0.005);
    adsr.SetTime(ADSR_SEG_DECAY, SPEED * 0.5);
    adsr.SetTime(ADSR_SEG_RELEASE, SPEED * 0.3);


    // start callback
    seed.StartAudio(AudioCallback);


    while(1) {}
}
