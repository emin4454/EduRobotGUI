/* audio_visualizer.h */
#pragma once
#include <pocketfft_hdronly.h>
#include <SDL2/SDL.h>
#include <complex>
#include <vector>
#include <mutex>
#include <cstdint>

class AudioVisualizer {
public:
    explicit AudioVisualizer(int fftSize = 1024);
    ~AudioVisualizer() = default;              

    void pushSamples(const int16_t* pcm, size_t n); 
    void update();                              
    void render(SDL_Renderer* r, int w, int h) const;

private:
    /* sabit parametreler */
    const int                         fftSize_;

    /* FFT tamponları */
    std::vector<float>                fftIn_;      
    std::vector<std::complex<float>>  fftOut_;     
    std::vector<float>                magnitudes_; 

    /* basit ring-buffer */
    std::mutex                        mtx_;
    std::vector<int16_t>              ring_;
};
