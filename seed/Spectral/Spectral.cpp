#include "daisy_seed.h"
#include "daicsp.h"
// #include "daisysp.h"

using namespace daisy;
using namespace daicsp;
// using namespace daisysp;

static DaisySeed hw;

static SpectralAnalyzer analyzer;
// PhaseVocoder vocoder;

static float sampleStep;
static long t = 0;
float samples[64];

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{

	for (size_t i = 0; i < size; i++) {
		float angle = TWOPI_F * sampleStep * t++;
		samples[i] = sin(angle);
	}

	// TODO -- the fft is causing the program to crash for some reason
	SpectralBuffer fsig = analyzer.Process(samples, size);

	// spectral effects here
	// NOTE -- fsig in and out cannot be the same object as undefined behavior can occur

	// vocoder.Process(out[0], size, fsig);
}

int main(void)
{
	hw.Configure();
	hw.Init();

	float sampleRate = hw.AudioSampleRate();
	size_t block = hw.AudioBlockSize();

	analyzer.Init(256, 64, 512, SPECTRAL_WINDOW::HAMMING, sampleRate, block);
	// vocoder.Init(sample_rate);

	sampleStep = 10000.0 / sampleRate;

	hw.StartAudio(AudioCallback);
	while(1) {}
}
