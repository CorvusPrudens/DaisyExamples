#include "daisy_seed.h"
#include "daicsp.h"

/** This program demonstrates basic use of the spectral modules.
 *  If an error occurs during initialization of either the
 *  `SpectralAnalyzer` or the `PhaseVocoder`, the program
 *  will stall and not produce any output. The error can
 *  be inspected with debugging.
 * 
 *  Currently, sliding synthesis is not supported, so the
 *  Smallest valid fftsize is 256. The maximum size, limited
 *  by memory, is 4096. fftsize must be a power of 2 due to
 *  shy_fft.
 * 
 *  An unmodified wave is generated and written to channel zero.
 *  The spectrally manipulated signal is written to channel one.
 */

using namespace daisy;
using namespace daicsp;

static DaisySeed hw;

static SpectralAnalyzer analyzer;
static PhaseVocoder vocoder;

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
		samples[i] = SimpleWave(WAVE::TRI, 1000, sampleRate) * 0.75;
	}

	auto fsig = analyzer.Process(samples, size);
	// NOTE -- auto just makes this easy, but the proper type is of course valid:
	// SpectralBuffer fsig = analyzer.Process(samples, size);

	// spectral effects here

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

	int fftsize = 1024;
	int overlap = fftsize / 4;
	int winsize = fftsize;

	analyzer.Init(fftsize, overlap, winsize, SPECTRAL_WINDOW::HAMMING, sampleRate, block);
	if (analyzer.GetStatus() != SpectralAnalyzer::STATUS::OK)
		while (1);
	
	vocoder.Init(analyzer.GetFsig(), sampleRate, block);
	if (vocoder.GetStatus() != PhaseVocoder::STATUS::OK)
		while (1);
	
	hw.StartAudio(AudioCallback);
	while(1) {}
}

// NOTE -- t gets big after a while, 
// so it's not particularly reliable for long tests.
float SimpleWave(WAVE wave, float frequency, float sampleRate) {
	static unsigned int t = 0;
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
