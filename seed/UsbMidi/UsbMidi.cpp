#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Oscillator osc;
Svf        filt;
MidiUsbHandler midi;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    float sig;
    for(size_t i = 0; i < size; i++)
    {
        sig = osc.Process();
        filt.Process(sig);
        for(size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = filt.Low();
        }
    }
}

// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            // This is to avoid Max/MSP Note outs for now..
            if(m.data[1] != 0)
            {
                p = m.AsNoteOn();
                osc.SetFreq(mtof(p.note));
                osc.SetAmp((p.velocity / 127.0f));
            }
            else
            {
                osc.SetAmp(0);
            }
        }
        break;
        case NoteOff:
        {
            osc.SetAmp(0);
        }
        break;
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 1:
                    // CC 1 for cutoff.
                    filt.SetFreq(mtof((float)p.value));
                    break;
                case 2:
                    // CC 2 for res.
                    filt.SetRes(((float)p.value / 127.0f));
                    break;
                default: break;
            }
            break;
        }
        default: break;
    }
}

int main(void)
{
	// Init
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();

    // Synthesis
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    filt.Init(samplerate);
	
	MidiUsbHandler::Config config;
	midi.Init(config);
    midi.StartReceive();

    hw.StartAudio(AudioCallback);
    for(;;)
    {
        midi.Listen();
        // Handle MIDI Events
        while(midi.HasEvents())
        {
            HandleMidiMessage(midi.PopEvent());
        }
    }
}
