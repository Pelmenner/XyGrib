/**********************************************************************
XyGrib: meteorological GRIB file viewer
Copyright (C) 2008-2012 - Jacques Zaninetti - http://www.zygrib.org

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#include "SatelliteImageEqualizer.h"
#include "gdal.h"
#include "gdal_priv.h"
#include <numeric>

SatelliteImageEqualizer::SatelliteImageEqualizer(GDALRasterBand *band)
{
    double mean = 0, stdDev = 0;
    band->ComputeStatistics(TRUE, &minValue, &maxValue, &mean, &stdDev, nullptr, nullptr);
    GUIntBig anHistogram[HISTOGRAM_BINS_NUMBER];
    band->GetHistogram(minValue, maxValue, HISTOGRAM_BINS_NUMBER, anHistogram, 
        FALSE, TRUE, GDALDummyProgress, nullptr);
    int valuesCount = std::accumulate(anHistogram, anHistogram + HISTOGRAM_BINS_NUMBER, 0);

    double pHistogram[HISTOGRAM_BINS_NUMBER];
    for (int i = 0; i < HISTOGRAM_BINS_NUMBER; ++i)
        pHistogram[i] = static_cast<double>(anHistogram[i]) / valuesCount;
    
    std::partial_sum(pHistogram, pHistogram + HISTOGRAM_BINS_NUMBER, transformMap);
    for (int i = 0; i <= HISTOGRAM_BINS_NUMBER; ++i)
        transformMap[i] *= 255;
}

int SatelliteImageEqualizer::getBin(float value)
{
    return static_cast<int>((value - minValue) / (maxValue - minValue) * HISTOGRAM_BINS_NUMBER);
}

float SatelliteImageEqualizer::transform(float value)
{
    int bin = getBin(value);
    if (bin < 0)
        return 0;
    if (bin > HISTOGRAM_BINS_NUMBER)
        return 255;
    return transformMap[getBin(value)];
}
