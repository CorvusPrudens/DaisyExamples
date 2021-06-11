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
 *  An unmodified wave is generated and written to channel 0.
 *  The spectrally manipulated signal is written to channel 1.
 */

using namespace daisy;
using namespace daicsp;

static DaisySeed hw;

#define FFTSIZE 4096
// Note divided by three -- this gives the maximum
// performance overhead with mostly negligeable 
// distortion
#define OVERLAP FFTSIZE / 3
#define WINDOW_SIZE FFTSIZE

static SpectralAnalyzerFifo<FFTSIZE, OVERLAP, WINDOW_SIZE> analyzer;
static PhaseVocoderFifo<FFTSIZE, OVERLAP, WINDOW_SIZE> vocoder;
static SpectralBlurFifo<FFTSIZE, OVERLAP, WINDOW_SIZE> blur;

#define BLURBUFF_SIZE 1024*512
float DSY_SDRAM_BSS blurBuff[BLURBUFF_SIZE];

static float sampleRate;

enum WAVE {
	SIN = 0,
	TRI,
	RECT,
};

size_t delay = 0;

float SimpleWave(WAVE wave, float frequency, float sampleRate);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	if (delay++ > 4)
	{
		for (size_t i = 0; i < size; i++) {
			// Feel free to feed in this oscillator if you don't have 
			// external audio set up. It will slow down the fft, though.
			// float sample = SimpleWave(WAVE::TRI, 440, sampleRate) * 0.75;
			float sample = in[0][i];
			analyzer.Sample(sample);
			out[1][i] = sample;
			out[0][i] = vocoder.Sample();
		}
	}
	
}

int main(void)
{
	hw.Configure();
	hw.Init(true);

	sampleRate = hw.AudioSampleRate();
	size_t block = hw.AudioBlockSize();

	analyzer.Init(SPECTRAL_WINDOW::HAMMING, sampleRate, block);
	analyzer.HaltOnError();

	blur.Init(analyzer.GetFsig(), 0.4, blurBuff, BLURBUFF_SIZE, sampleRate, block);
	blur.HaltOnError();
	
	vocoder.Init(blur.GetFsig(), sampleRate, block);
	vocoder.HaltOnError();

	hw.StartAudio(AudioCallback);
	bool toggle = false;
	while (1)
	{
		// blocks until the input is filled
		auto fsig = analyzer.Process();
		auto fsig2 = blur.Process(fsig);
		vocoder.Process(fsig2);
		hw.SetLed(toggle);
		toggle = !toggle;
	}
}

// NOTE -- t gets big after a while, 
// so it's not particularly reliable for long tests.
float SimpleWave(WAVE wave, float frequency, float sampleRate) {
	static unsigned int t = 0;
	float out = 0;
	float freq = (frequency / sampleRate);
	switch (wave) {
		case WAVE::SIN:
			{
				float angle = TWOPI_F * freq * t;
				out = sin(angle) * 1.0f;
			}
			break;
		case WAVE::TRI:
			{
				for (int h = 0; h < 4; h++) {
					float sign = h % 2 == 0 ? 1 : -1;
					float harm = h * 2 + 1;
					out += sign*sin(TWOPI_F * freq * harm * t) / (harm*harm);
				}
			}
			break;
		case WAVE::RECT:
			{
				for (int h = 0; h < 4; h++) {
					float harm = h * 2 + 1;
					out += sin(TWOPI_F * freq * harm * t) / harm;
				}
			}
			break;
	}
	t++;
	return out;
}
