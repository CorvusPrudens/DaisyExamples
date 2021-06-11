# Spectral

## Author

Gabriel Ball

## Description

This program demonstrates basic use of the spectral modules. If an error occurs during initialization of either the `SpectralAnalyzer` or the `PhaseVocoder`, the program will stall and not produce any output. The error can be inspected with debugging.

Currently, sliding synthesis is not supported, so the smallest valid fftsize is 256. The maximum size, limited by memory, is 4096. fftsize must be a power of 2 due to shy_fft.

An unmodified wave is generated and written to channel 0. The spectrally manipulated signal is written to channel 1.
