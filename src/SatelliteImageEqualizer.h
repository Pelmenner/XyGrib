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

#ifndef SATELLITEVALUELIMITER_H
#define SATELLITEVALUELIMITER_H

#include "gdal_priv.h"

const int HISTOGRAM_BINS_NUMBER = 1000;

class SatelliteImageEqualizer
{
public:
    SatelliteImageEqualizer(GDALRasterBand* band);
    float transform(float value);

private:
    int getBin(float value);

    double minValue;
    double maxValue;
    float transformMap[HISTOGRAM_BINS_NUMBER + 1];
};

#endif // SATELLITEVALUELIMITER_H