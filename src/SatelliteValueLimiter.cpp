#include "SatelliteValueLimiter.h"
#include "gdal.h"
#include "gdal_priv.h"
#include <numeric>

SatelliteValueLimiter::SatelliteValueLimiter(GDALRasterBand *band)
{
    double mean = 0, stdDev = 0;
    band->ComputeStatistics(TRUE, &minValue, &maxValue, &mean, &stdDev, nullptr, nullptr);
    GUIntBig anHistogram[HISTOGRAM_BINS_NUMBER];
    band->GetHistogram(minValue, maxValue, HISTOGRAM_BINS_NUMBER, anHistogram, FALSE, TRUE, GDALDummyProgress, nullptr);
    int valuesCount = std::accumulate(anHistogram, anHistogram + HISTOGRAM_BINS_NUMBER, 0);
    double pHistogram[HISTOGRAM_BINS_NUMBER];
    for (int i = 0; i < HISTOGRAM_BINS_NUMBER; ++i)
        pHistogram[i] = static_cast<double>(anHistogram[i]) / valuesCount;
    std::partial_sum(pHistogram, pHistogram + HISTOGRAM_BINS_NUMBER, transformMap);
    for (int i = 0; i <= HISTOGRAM_BINS_NUMBER; ++i)
        transformMap[i] *= 255;
    // if (currentWindow < windowSize)
    // {
    //     static const float eps = 1e-7;
    //     if (currentWindow < eps)
    //     {
    //         k = 1;
    //         b = mean;
    //     }
    //     else
    //     {
    //         k = 255 / currentWindow;
    //         b = (minValue + maxValue) / 2;
    //     }
    //     return;
    // }
    // k = 255 / windowSize * 100 / stdDev;
    // b = mean;
}

int SatelliteValueLimiter::getBin(float value)
{
    return static_cast<int>((value - minValue) / (maxValue - minValue) * HISTOGRAM_BINS_NUMBER);
}

float SatelliteValueLimiter::transform(float value)
{
    int bin = getBin(value);
    if (bin < 0)
        return 0;
    if (bin > HISTOGRAM_BINS_NUMBER)
        return 255;
    return transformMap[getBin(value)];
    // value = k * (value - b) + 128;
    // if (value < 0)
    //     return 0;
    // if (value > 255)
    //     return 255;
    // return value;
}
