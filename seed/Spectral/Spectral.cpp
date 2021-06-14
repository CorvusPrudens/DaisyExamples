#include "daisy_seed.h"
#include "daicsp.h"

/** This program demonstrates basic use of the spectral modules.
 *  If an error occurs during initialization of any of the
 *  classes, the program will stall and not produce any
 *  output. The error can be inspected with debugging.
 * 
 *  Currently, sliding synthesis is not supported, so the
 *  Smallest valid fftsize is 256. The maximum size, limited
 *  by memory, is 4096. fftsize must be a power of 2 due to
 *  shy_fft.
 * 
 *  An unmodified wave is generated and written to channel 0.
 *  The spectrally manipulated signal is written to channel 1.
 *  The spectral manipulation can be set with the `processing`
 *  variable;
 */

using namespace daisy;
using namespace daicsp;

static DaisySeed hw;


/** Current FFTSIZE performance:
 *	\param SpectralBlur - Can only manage at 4096 with a maximum delay time of 0.4s
 *  \param SpectralFreeze - Good down to 512, artefacts at 256
 *  \param SpectralScale - Can only manage at 4096 in mode 0 (no formant preservation).
 *  True envelope mode is a bit scary, and I think it's bugged.
 */
#define FFTSIZE 4096
// Note divided by three -- this gives the maximum
// performance overhead with mostly negligeable 
// distortion
#define OVERLAP FFTSIZE / 3
#define WINDOW_SIZE FFTSIZE

static SpectralAnalyzerFifo<FFTSIZE, OVERLAP, WINDOW_SIZE> analyzer;
static PhaseVocoderFifo<FFTSIZE, OVERLAP, WINDOW_SIZE> vocoder;
static SpectralBlurFifo<FFTSIZE, OVERLAP, WINDOW_SIZE> blur;
static SpectralFreezeFifo<FFTSIZE, OVERLAP, WINDOW_SIZE> freeze;
static SpectralScale<FFTSIZE, OVERLAP, WINDOW_SIZE> scale;

enum PROCESSING {
	BLUR = 0,
	FREEZE,
	SCALE,
};

// Change to swap between processing modes
PROCESSING processing = PROCESSING::SCALE;

// Avout 2 seconds of delay
#define BLURBUFF_SIZE 1024*512
float DSY_SDRAM_BSS blurBuff[BLURBUFF_SIZE];

static float sampleRate;

enum WAVE {
	SIN = 0,
	TRI,
	RECT,
};

float SimpleWave(WAVE wave, float frequency, float sampleRate);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	for (size_t i = 0; i < size; i++) {

		switch (processing)
		{
			case PROCESSING::FREEZE:
				{
					float s = fabs(SimpleWave(WAVE::SIN, 1.5, sampleRate)) * 1.5;
					freeze.SetAmplitude(s);
					freeze.SetFrequency(s);
				}
				break;
			case PROCESSING::SCALE:
				{
					float s = 1 + SimpleWave(WAVE::SIN, 0.25, sampleRate) * 0.05;
					scale.SetScale(s);
				}
				break;
			default:
				break;
		}

		float sample = in[0][i];

		analyzer.Sample(sample);

		out[1][i] = sample;
		out[0][i] = vocoder.Sample();

	}
}

int main(void)
{
	hw.Configure();
	// Boosted for performance
	hw.Init(true);

	sampleRate = hw.AudioSampleRate();

	analyzer.Init(SPECTRAL_WINDOW::HAMMING, sampleRate);
	analyzer.HaltOnError();

	blur.Init(analyzer.GetFsig(), 0.4, blurBuff, BLURBUFF_SIZE, sampleRate);
	blur.HaltOnError();

	freeze.Init(analyzer.GetFsig(), 1, 1, sampleRate);
	freeze.HaltOnError();

	scale.Init(analyzer.GetFsig(), 1, sampleRate, FORMANT::NONE, 1, 80);
	scale.HaltOnError();
	
	// NOTE -- the effect fsigs will be equivalent in this case,
	// so any can be used to initialize the vocoder
	vocoder.Init(blur.GetFsig(), sampleRate);
	vocoder.HaltOnError();

	hw.StartAudio(AudioCallback);
	while (1)
	{
		// blocks until the input is filled
		auto fsig = analyzer.Process();
		auto fsig2 = fsig;

		switch (processing)
		{
			case PROCESSING::BLUR:
				fsig2 = blur.Process(fsig);
				break;
			case PROCESSING::FREEZE:
				fsig2 = freeze.Process(fsig);
				break;
			case PROCESSING::SCALE:
				fsig2 = scale.Process(fsig);
				break;
			default:
				break;
		}

		vocoder.Process(fsig2);
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
