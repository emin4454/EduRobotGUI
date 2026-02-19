#include "audio_visualizer.h"
#include "pocketfft_hdronly.h"
#include <cmath>
#include <algorithm>

AudioVisualizer::AudioVisualizer(int N)
    : fftSize_(N),
      fftIn_(N, 0.f),
      fftOut_(N/2+1),
      magnitudes_(N/2, 0.f) {}


void AudioVisualizer::pushSamples(const int16_t* data, size_t n)
{
    std::lock_guard lk(mtx_);
    ring_.insert(ring_.end(), data, data + n);
    if (ring_.size() > fftSize_ * 4)
        ring_.erase(ring_.begin(),
                    ring_.end() - fftSize_ * 4);
}

void AudioVisualizer::update()
{
    std::vector<int16_t> buf;
    {
        std::lock_guard lk(mtx_);
        if (ring_.size() < fftSize_) return;
        buf.assign(ring_.end() - fftSize_, ring_.end());
    }

    constexpr float kInv32768 = 1.0f / 32768.0f;
    for (int i = 0; i < fftSize_; ++i) {
        float win = 0.5f * (1.f - std::cos(2.f * M_PI * i / (fftSize_-1)));
        fftIn_[i] = buf[i] * kInv32768 * win;
    }

    using size_v   = std::vector<size_t>;
    using stride_v = std::vector<ptrdiff_t>;

    pocketfft::r2c<float>(
        size_v{ static_cast<size_t>(fftSize_) },
        stride_v{ static_cast<ptrdiff_t>(sizeof(float)) },
        stride_v{ static_cast<ptrdiff_t>(sizeof(std::complex<float>)) },
        0, pocketfft::FORWARD,
        fftIn_.data(), fftOut_.data(),
        1.0f);

    for (int k = 0; k < fftSize_/2; ++k) {
        float mag = std::abs(fftOut_[k]);
        magnitudes_[k] = 20.f * std::log10(mag + 1e-12f);
    }
}

void AudioVisualizer::render(SDL_Renderer* r, int w, int h) const
{
    constexpr int GROUP = 16;                          
    const int srcBins   = static_cast<int>(magnitudes_.size());
    const int dstBins   = srcBins / GROUP;           
    const float binW    = static_cast<float>(w) / dstBins;
    const float hFloor  = h * 0.05f;                 

    SDL_SetRenderDrawColor(r, 60, 255, 60, 150);

    for (int i = 0; i < dstBins; ++i)
    {
        /*  Grubun ortalama dB değeri */
        float sum = 0.f;
        for (int k = 0; k < GROUP; ++k)
            sum += magnitudes_[ i*GROUP + k ];
        float dB   = std::clamp(sum / GROUP, -70.f, 0.f);
        float norm = (dB + 70.f) / 70.f;

        float barH = std::max(h * norm, hFloor);

        SDL_FRect bar{
            i * binW,
            h - barH,
            binW - 1,
            barH
        };
        SDL_RenderFillRectF(r, &bar);
    }

    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
}

