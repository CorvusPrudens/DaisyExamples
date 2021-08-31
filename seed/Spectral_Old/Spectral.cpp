#include "daisy_seed.h"
#include <string.h>

// if you'd like to reduce the memory footprint of the
// spectral classes, simply define the maximum fftsize
// with some power of two at 4096 or below before daicsp.h:
// #define __FFT_SIZE__ 2048

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
#define FFTSIZE 512
// Note divided by three -- this gives the maximum
// performance overhead with mostly negligeable 
// distortion
#define OVERLAP FFTSIZE / 4
#define WINDOW_SIZE FFTSIZE

static SpectralAnalyzer analyzer;
static PhaseVocoder vocoder;
// static SpectralBlur blur;
// static SpectralFreeze freeze;
// static SpectralScale scale;
// static DsyFFT fft;

// static float analyzer_audio_left[daicsp::kFFTMaxFrames];
// // static float DSY_SDRAM_BSS analyzer_audio_right[daicsp::kFFTMaxFrames];
// static float               analbuff_l_[daicsp::kFFTMaxFloats];
// static float               analbuffout_l_[daicsp::kFFTMaxFloats];
// static float               analwin_l_[daicsp::kFFTMaxFloats];
// static float               analhist_l_[daicsp::kFFTMaxFloats];
// static float               analbuff_r_[daicsp::kFFTMaxFloats];
// static float               analbuffout_r_[daicsp::kFFTMaxFloats];
// static float DSY_SDRAM_BSS analwin_r_[daicsp::kFFTMaxFloats];
// static float DSY_SDRAM_BSS analhist_r_[daicsp::kFFTMaxFloats];

// static SpectralWarp<FFTSIZE, OVERLAP, WINDOW_SIZE> warp;

enum PROCESSING {
	BLUR = 0,
	FREEZE,
	SCALE,
	WARP,
};

// Change to swap between processing modes
PROCESSING processing = PROCESSING::FREEZE;

// Avout 2 seconds of delay
// #define BLURBUFF_SIZE 1024*512
// float DSY_SDRAM_BSS blurBuff[BLURBUFF_SIZE];

static float sampleRate;

enum WAVE {
	SIN = 0,
	TRI,
	RECT,
};

float SimpleWave(WAVE wave, float frequency, float sampleRate);

float cb_buff_in[2][48];
float cb_buff_out[2][48];
bool fill = false;

void PreCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	// memcpy(cb_buff_in[0], in[0], size * sizeof(cb_buff_in[0][0]));
	// memcpy(cb_buff_in[1], in[1], size * sizeof(cb_buff_in[1][0]));
	// memcpy(out[0], cb_buff_out[0], size * sizeof(cb_buff_out[0][0]));
	// memcpy(out[1], cb_buff_out[1], size * sizeof(cb_buff_out[1][0]));

	for (size_t i = 0; i < size; i++)
	{
		cb_buff_in[0][i] = in[0][i];
		cb_buff_in[1][i] = in[1][i];
		out[0][i] = cb_buff_out[0][i];
		out[1][i] = cb_buff_out[1][i];
	}
	fill = true;
}

void AudioCallback(float in[][48], float out[][48], size_t size)
{

	float data[size];
	for (size_t i = 0; i < size; i++)
	{
		data[i] = SimpleWave(WAVE::RECT, 440, sampleRate);
	}

	auto& fsig = analyzer.Process(data, size);
	float* spectral_out = vocoder.Process(fsig, size);

	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = data[i];
		// out[1][i] = 0;
		out[1][i] = spectral_out[i];
	}

	// for (size_t i = 0; i < size; i++) {

	// 	// switch (processing)
	// 	// {
	// 	// 	case PROCESSING::FREEZE:
	// 	// 		{
	// 	// 			float s = fabs(SimpleWave(WAVE::SIN, 1.5, sampleRate)) * 1.5;
	// 	// 			freeze.SetAmplitude(s);
	// 	// 			freeze.SetFrequency(s);
	// 	// 		}
	// 	// 		break;
	// 	// 	case PROCESSING::SCALE:
	// 	// 		{
	// 	// 			float s = 1 + SimpleWave(WAVE::SIN, 0.25, sampleRate) * 0.05;
	// 	// 			scale.SetScale(s);
	// 	// 		}
	// 	// 		break;
	// 	// 	default:
	// 	// 		break;
	// 	// }

	// 	float sample = SimpleWave(WAVE::SIN, 440, sampleRate);

	// 	auto& fsig = analyzer.Process(sample);
	// 	auto* fsig2 = &fsig;

	// 	// switch (processing)
	// 	// {
	// 	// 	case PROCESSING::BLUR:
	// 	// 		fsig2 = &blur.ParallelProcess(fsig);
	// 	// 		break;
	// 	// 	case PROCESSING::FREEZE:
	// 	// 		fsig2 = &freeze.ParallelProcess(fsig);
	// 	// 		break;
	// 	// 	case PROCESSING::SCALE:
	// 	// 		fsig2 = &scale.ParallelProcess(fsig);
	// 	// 		break;
	// 	// 	// case PROCESSING::WARP:
	// 	// 	// 	fsig2 = warp.ParallelProcess(fsig);
	// 	// 	// 	break;
	// 	// 	default:
	// 	// 		break;
	// 	// }

	// 	out[0][i] = sample;
	// 	out[1][i] = vocoder.Process(*fsig2);

	// }
}

bool toggle = false;

int main(void)
{
	hw.Configure();
	// Boosted for performance
	hw.Init(true);

	// hw.StopAudio();

	// hw.SetAudioBlockSize(32);
	// hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_16KHZ);

	sampleRate = hw.AudioSampleRate();
	size_t blockSize = hw.AudioBlockSize();

	analyzer.Init(
			FFTSIZE, 
			OVERLAP, 
			WINDOW_SIZE,
			SPECTRAL_WINDOW::HAMMING, 
			sampleRate,
			blockSize
	);

	// blur.Init(analyzer.GetFsig(), 0.3, blurBuff, BLURBUFF_SIZE, sampleRate);
	// blur.HaltOnError();

	// freeze.Init(analyzer.GetFsig(), 1, 1, sampleRate);
	// freeze.HaltOnError();

	// scale.Init(analyzer.GetFsig(), 1, sampleRate, FORMANT::NONE, 1, 80);
	// scale.HaltOnError();

	// warp.Init(analyzer.GetFsig(), 1, 500, sampleRate);
	// warp.HaltOnError();
	
	// NOTE -- the effect fsigs will be equivalent in this case,
	// so any can be used to initialize the vocoder
	vocoder.Init(analyzer.GetFsig(), sampleRate, blockSize);
	// vocoder.HaltOnError();

	hw.StartAudio(PreCallback);
	for (;;)
	{
		if (fill)
		{
			hw.SetLed(toggle);
			toggle = !toggle;
			fill = false;
			AudioCallback(cb_buff_in, cb_buff_out, hw.AudioBlockSize());
		}
		// auto fsig = analyzer.ParallelProcess();
		// auto fsig2 = fsig;

		// switch (processing)
		// {
		// 	case PROCESSING::BLUR:
		// 		fsig2 = blur.ParallelProcess(fsig);
		// 		break;
		// 	case PROCESSING::FREEZE:
		// 		fsig2 = freeze.ParallelProcess(fsig);
		// 		break;
		// 	case PROCESSING::SCALE:
		// 		fsig2 = scale.ParallelProcess(fsig);
		// 		break;
		// 	// case PROCESSING::WARP:
		// 	// 	fsig2 = warp.ParallelProcess(fsig);
		// 	// 	break;
		// 	default:
		// 		break;
		// }

		// vocoder.ParallelProcess(fsig2);
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
				for (int h = 0; h < 8; h++) {
					float harm = h * 2 + 1;
					out += sin(TWOPI_F * freq * harm * t) / harm;
				}
			}
			break;
	}
	t++;
	return out;
}
