#ifndef SATELLITEVALUELIMITER_H
#define SATELLITEVALUELIMITER_H

#include "gdal_priv.h"

const int HISTOGRAM_BINS_NUMBER = 1000;

class SatelliteValueLimiter
{
public:
    SatelliteValueLimiter(GDALRasterBand* band);
    float transform(float value);

private:
    int getBin(float value);

    double minValue;
    double maxValue;
    float transformMap[HISTOGRAM_BINS_NUMBER + 1];
};

#endif // SATELLITEVALUELIMITER_H