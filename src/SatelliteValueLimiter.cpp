#include "SatelliteValueLimiter.h"

SatelliteValueLimiter::SatelliteValueLimiter(GDALRasterBand *band, float windowSize)
{
    double adfMinMax[2];
    int bGotMin, bGotMax;
    adfMinMax[0] = band->GetMinimum(&bGotMin);
    adfMinMax[1] = band->GetMaximum(&bGotMax);
    if (!(bGotMin && bGotMax))
        GDALComputeRasterMinMax((GDALRasterBandH)band, TRUE, adfMinMax);   
    float minValue = adfMinMax[0];
    float maxValue = adfMinMax[1];
    float currentWindow = maxValue - minValue;
    if (currentWindow < windowSize)
    {
        static const float eps = 1e-7;
        if (currentWindow < eps)
        {
            k = 1;
            b = 0;
        }
        else
        {
            k = 255 / currentWindow;
            b = -minValue * k;
        }
        return;
    }
    auto colorTable = band->GetColorTable();
    k = 255 / windowSize;
    b = -(minValue + (currentWindow - windowSize) / 2) * k;
}

float SatelliteValueLimiter::transform(float value)
{
    value = value * k + b;
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return value;
}
