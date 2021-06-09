#include "daisy_seed.h"
#include "daicsp.h"

using namespace daisy;
using namespace daicsp;

static DaisySeed hw;

static SpectralAnalyzer analyzer;
PhaseVocoder vocoder;

static float sampleRate;
float samples[64];

enum WAVE {
	SIN = 0,
	TRI,
	RECT,
};

float SimpleWave(WAVE wave, float frequency, float sampleRate);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{

	for (size_t i = 0; i < size; i++) {
		samples[i] = SimpleWave(WAVE::SIN, 1000, sampleRate);
	}

	auto fsig = analyzer.Process(samples, size);

	// spectral effects here

	auto output = vocoder.Process(fsig, size);

	// Outputting input wave and resynthesized wave on channels 1 and 2
	for (size_t i = 0; i < size; i++) {
		out[0][i] = samples[i];
		out[1][i] = output[i];
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();

	sampleRate = hw.AudioSampleRate();
	size_t block = hw.AudioBlockSize();

	// TODO -- sliding configuration causes a hard fault
	int fftsize = 256;
	int overlap = fftsize / 4;
	int winsize = fftsize;

	analyzer.Init(fftsize, overlap, winsize, SPECTRAL_WINDOW::HAMMING, sampleRate, block);
	vocoder.Init(analyzer.GetFsig(), sampleRate, block);
	
	hw.StartAudio(AudioCallback);
	while(1) {}
}

// note -- t gets big after a while XD
float SimpleWave(WAVE wave, float frequency, float sampleRate) {
	static int t = 0;
	float out = 0;
	switch (wave) {
		case WAVE::SIN:
			{
				float angle = TWOPI_F * (frequency / sampleRate) * t;
				out = sin(angle) * 1.0f;
				
			}
			break;
		case WAVE::TRI:
			{
				for (int h = 0; h < 16; h++) {
					out += pow(h*2 + 1, -2)*pow(-1, h)*sin(2.0*PI_F*((frequency*(h*2 + 1))/sampleRate)*t);
				}
			}
			break;
		case WAVE::RECT:
			{
				for (int h = 0; h < 16; h++) {
					out += pow(h*2 + 1, -1)*sin(2.0*PI_F*((frequency*(h*2 + 1))/sampleRate)*t);
				}
			}
			break;
	}
	t++;
	return out;
}
