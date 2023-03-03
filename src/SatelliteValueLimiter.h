#ifndef SATELLITEVALUELIMITER_H
#define SATELLITEVALUELIMITER_H

#include "gdal_priv.h"

class SatelliteValueLimiter
{
public:
    SatelliteValueLimiter(GDALRasterBand* band, float windowSize);
    float transform(float value);

private:
    float k;
    float b;
};

#endif // SATELLITEVALUELIMITER_H