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

#include "SatellitePlotter.h"
#include "qdebug.h"

SatellitePlotter::SatellitePlotter() : reader(nullptr), projection(nullptr)
{
}

SatellitePlotter::SatellitePlotter(const SatellitePlotter &p)
    : reader(p.reader), projection(p.projection), satelliteImage(p.satelliteImage)
{
}

float getPixelValue(double dfGeoX, double dfGeoY, double adfGeoTransform[6], GDALRasterBand *pBand)
{
    // int nPixelX = (int)((dfGeoX - adfGeoTransform[0]) / adfGeoTransform[1] + 0.5);
    // int nPixelY = (int)((dfGeoY - adfGeoTransform[3]) / adfGeoTransform[5] + 0.5);
    double invTransform[6];
    GDALInvGeoTransform(adfGeoTransform, invTransform);
    double pixelX, pixelY;
    GDALApplyGeoTransform(invTransform, dfGeoX, dfGeoY, &pixelX, &pixelY);
    float dfValue;
    pBand->RasterIO(GF_Read, (int)pixelX, (int)pixelY, 1, 1, &dfValue, 1, 1, GDT_Float32, 0, 0);
    return dfValue;
}

void SatellitePlotter::draw(QPainter &pnt, Projection *proj)
{
    if (!isReaderOk())
        return;
    // if (satelliteImage.isNull())
    //     return;
    // assert(getReader() != nullptr);

    // GriddedRecord *record = nullptr; // SATODO: getReader()->getRecord (currentDate);
    // if (record == nullptr)
    //     return;

    double lon, lat;
    int W = proj->getW();
    int H = proj->getH();
    QRgb rgb;
    QImage *image = new QImage(W, H, QImage::Format_ARGB32);
    auto band = reader->getRecord(time(nullptr));
    double adfGeoTransform[6];
    reader->getGeoTransform(adfGeoTransform);
    image->fill(qRgba(0, 0, 0, 0));
    for (int i = 0; i < W - 1; i += 2)
    {
        for (int j = 0; j < H - 1; j += 2)
        {
            proj->screen2map(i, j, &lon, &lat);
            float value = getPixelValue(lon, lat, adfGeoTransform, band);
            // int x, y;
            // // projection->map2screen(lon, lat, &x, &y);
            // x = (lon + 180) * (satelliteImage.width() / 360.0);
            // y = (-lat + 90) * (satelliteImage.height() / 180.0);

            // // if (!record->isXInMap(lon))
            // //     lon += 360.0; // tour complet ?
            // // if (!record->isPointInMap(lon, lat))
            // //     continue;
            // if (!satelliteImage.valid(x, y))
            //     continue;
            // auto color = satelliteImage.pixelColor(x, y);
            // auto data = record->getInterpolatedValue(lon, lat);

            // if (GribDataIsDef(data))
            // {
            // rgb = color.rgb(); // qRgb(0, 255, 0); // SATODO: get pixel color from data
            rgb = qRgb(value, value, value);
            image->setPixel(i, j, rgb);
            image->setPixel(i + 1, j, rgb);
            image->setPixel(i, j + 1, rgb);
            image->setPixel(i + 1, j + 1, rgb);
            // }
        }
    }
    pnt.drawImage(0, 0, *image);
    delete image;
}

void SatellitePlotter::loadFile(const QString &fileName,
                                LongTaskProgress *taskProgress, int nbrecs)
{
    delete reader;
    reader = new SatelliteReader();
    if (taskProgress != nullptr)
    {
        QObject::connect(reader, &LongTaskMessage::valueChanged,
                         taskProgress, &LongTaskProgress::setValue);

        QObject::connect(reader, &LongTaskMessage::newMessage,
                         taskProgress, &LongTaskProgress::setMessage);

        QObject::connect(taskProgress, &LongTaskProgress::canceled,
                         reader, &LongTaskMessage::cancel);
    }
    reader->openFile(fileName, nbrecs);
    if (reader->isOk())
    {
        // listDates = gribReader->getListDates();
        // setCurrentDate ( !listDates.empty() ? *(listDates.begin()) : 0);
    }
    this->fileName = fileName;
    if (!satelliteImage.load(fileName))
    {
        this->fileName = "";
        return;
        // SATODO: return error
    }
    delete projection;
    projection = new Projection_CENTRAL_CYL(satelliteImage.width(), satelliteImage.height(), satelliteImage.width() / 2, satelliteImage.height() / 2, 10.0);
}

void SatellitePlotter::setCurrentDate(time_t t)
{
    currentDate = t;
}

bool SatellitePlotter::isReaderOk() const
{
    if (reader == nullptr)
        return false;
    return reader->isOk();
}

// void SatellitePlotter::loadImages(LongTaskProgress *taskProgress, int nbrecs)
// {
// }
