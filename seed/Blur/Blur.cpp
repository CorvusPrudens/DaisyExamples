#include "daisy_seed.h"
#include "daicsp.h"

using namespace daisy;
using namespace daicsp;

static DaisySeed hw;

static SpectralAnalyzer analyzer;
static SpectralBlur blur;
static PhaseVocoder vocoder;

#define BLURBUFF_SIZE 1024*512
float DSY_SDRAM_BSS blurBuff[BLURBUFF_SIZE];

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
		samples[i] = SimpleWave(WAVE::TRI, 440, sampleRate) * 0.75;
	}

	auto fsig = analyzer.Process(samples, size);
	
	// auto fsig2 = blur.Process(fsig, size);

	auto output = vocoder.Process(fsig, size);

	// Outputting input wave and resynthesized wave on channels 0 and 1
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

	int fftsize = 256;
	int overlap = fftsize / 4;
	int winsize = fftsize;

	analyzer.Init(fftsize, overlap, winsize, SPECTRAL_WINDOW::HAMMING, sampleRate, block);
	analyzer.HaltOnError();

	// blur.Init(analyzer.GetFsig(), 0.1, blurBuff, BLURBUFF_SIZE, sampleRate, block);
	// blur.HaltOnError();
	
	vocoder.Init(analyzer.GetFsig(), sampleRate, block);
	vocoder.HaltOnError();
	
	hw.StartAudio(AudioCallback);

	while(1) 
	{

	}
}

// NOTE -- t gets big after a while, 
// so it's not particularly reliable for long tests.
float SimpleWave(WAVE wave, float frequency, float sr) {
	static int t = 0;
	float out = 0;
	switch (wave) {
		case WAVE::SIN:
			{
				float angle = TWOPI_F * (frequency / sr) * t;
				out = sin(angle) * 1.0f;
				
			}
			break;
		case WAVE::TRI:
			{
				for (int h = 0; h < 4; h++) {
					out += powf(h*2 + 1, -2)*powf(-1, h)*sin(TWOPI_F*((frequency*(h*2 + 1))/sr)*t);
				}
			}
			break;
		case WAVE::RECT:
			{
				for (int h = 0; h < 4; h++) {
					out += powf(h*2 + 1, -1)*sin(TWOPI_F*((frequency*(h*2 + 1))/sr)*t);
				}
			}
			break;
	}
	t++;
	return out;
}
