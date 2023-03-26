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
#include "SatelliteImageEqualizer.h"

#include "gdal_priv.h"
#include <QDebug>

SatellitePlotter::SatellitePlotter() : reader(nullptr), layer(0), subdataset(-1), rgb(false)
{
}

SatellitePlotter::SatellitePlotter(const SatellitePlotter &p) : reader(p.reader), layer(p.layer), subdataset(p.subdataset), rgb(p.rgb)
{
}

float getPixelValue(int pixelX, int pixelY, GDALRasterBand *pBand)
{
    float dfValue;
    pBand->RasterIO(GF_Read, pixelX, pixelY, 1, 1, &dfValue, 1, 1, GDT_Float32, 0, 0);
    return dfValue;
}

bool isPointInsideBounds(int x, int y, GDALRasterBand *pBand)
{
    return 0 <= x && x < pBand->GetXSize() && 0 <= y && y < pBand->GetYSize();
}

float getNoDataValue(GDALRasterBand* band)
{
    int hasNoDataValue = false;
    float noDataValue = band->GetNoDataValue(&hasNoDataValue);
    if (!hasNoDataValue)
        return 0;
    return noDataValue;
}

void drawPixelGrayscale(QImage *image, int imageX, int imageY, int dataX, int dataY, GDALRasterBand *band, SatelliteImageEqualizer& equalizer)
{
    float value = getPixelValue(dataX, dataY, band);
    if (value == getNoDataValue(band))
        return;

    value = equalizer.transform(value);
    QRgb rgb = qRgb(value, value, value);
    image->setPixel(imageX, imageY, rgb);
    image->setPixel(imageX + 1, imageY, rgb);
    image->setPixel(imageX, imageY + 1, rgb);
    image->setPixel(imageX + 1, imageY + 1, rgb);
}

void drawPixelRGB(QImage *image, int imageX, int imageY, int dataX, int dataY, GDALRasterBand *bands[3])
{
    float colors[3] = {};
    int noValueBandsCount = 0;
    for (int i = 0; i < 3; ++i)
    {
        colors[i] = getPixelValue(dataX, dataY, bands[i]);
        if (colors[i] == getNoDataValue(bands[i]))
            ++noValueBandsCount;
    }
    if (noValueBandsCount == 3)
        return;

    QRgb rgb = qRgb(colors[0], colors[1], colors[2]);
    image->setPixel(imageX, imageY, rgb);
    image->setPixel(imageX + 1, imageY, rgb);
    image->setPixel(imageX, imageY + 1, rgb);
    image->setPixel(imageX + 1, imageY + 1, rgb);
}

void screenToMap(const Projection* proj, int pixelX, int pixelY, double* lon, double *lat)
{
    proj->screen2map(pixelX, pixelY, lon, lat);
    while (*lon > 180)
        *lon -= 360;
    while (*lon < -180)
        *lon += 360;
}

void SatellitePlotter::draw(QPainter &pnt, Projection *proj)
{
    if (!isReaderOk())
        return;

    int W = proj->getW();
    int H = proj->getH();
    QImage *image = new QImage(W, H, QImage::Format_ARGB32);
    image->fill(qRgba(0, 0, 0, 0));

    GDALRasterBand *bands[3];
    fillBands(bands);

    SatelliteImageEqualizer equalizer(bands[0]);
        
    double lon, lat, pixelX, pixelY;
    for (int i = 0; i < W - 1; i += 2)
    {
        for (int j = 0; j < H - 1; j += 2)
        {
            screenToMap(proj, i, j, &lon, &lat);
            reader->transformMapToScreen(lon, lat, &pixelX, &pixelY);
            if (!isPointInsideBounds(pixelX, pixelY, bands[0]))
                continue;

            if (rgb)
                drawPixelRGB(image, i, j, pixelX, pixelY, bands);
            else
                drawPixelGrayscale(image, i, j, pixelX, pixelY, bands[0], equalizer);
        }
    }
    pnt.drawImage(0, 0, *image);
    delete image;
}

void SatellitePlotter::fillBands(GDALRasterBand *bands[3])
{
    if (rgb)
    {
        for (int i = 0; i < 3; ++i)
        {
            bands[i] = reader->getRecord(i + 1);
            if (bands[i] == nullptr)
                return;
        }
    }
    else
    {
        if (subdataset == -1)
            bands[0] = reader->getRecord(layer + 1);
        else
            bands[0] = reader->getRecord(subdataset, layer + 1);

        if (bands[0] == nullptr)
            return;
    }
}

void SatellitePlotter::loadFile(const QString &fileName,
                                LongTaskProgress *taskProgress)
{
    reader = QSharedPointer<SatelliteReader>(new SatelliteReader());
    if (taskProgress != nullptr)
    {
        QObject::connect(reader.data(), &LongTaskMessage::valueChanged,
                         taskProgress, &LongTaskProgress::setValue);

        QObject::connect(reader.data(), &LongTaskMessage::newMessage,
                         taskProgress, &LongTaskProgress::setMessage);

        QObject::connect(taskProgress, &LongTaskProgress::canceled,
                         reader.data(), &LongTaskMessage::cancel);
    }
    reader->openFile(fileName);
    this->fileName = fileName;
}

void SatellitePlotter::setCurrentDate(time_t t)
{
    currentDate = t;
}

bool SatellitePlotter::isReaderOk() const
{
    return reader != nullptr && reader->isOk();
}

void SatellitePlotter::setLayer(int newLayer)
{
    subdataset = -1;
    layer = newLayer;
    if (reader != nullptr && layer == reader->getBandsNumber())
        rgb = true;
    else
        rgb = false;
}

void SatellitePlotter::setLayer(int newSubdataset, int newLayer)
{
    subdataset = newSubdataset;
    layer = newLayer;
}

SatelliteReader *SatellitePlotter::getReader()
{
    return reader.data();
}
