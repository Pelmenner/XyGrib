// /**********************************************************************
// XyGrib: meteorological GRIB file viewer
// Copyright (C) 2008-2012 - Jacques Zaninetti - http://www.zygrib.org

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ***********************************************************************/

// /*************************
// Lecture mise en mémoire d'un fichier GRIB

// *************************/

#ifndef SATELLITEREADER_H
#define SATELLITEREADER_H

#include "gdal_priv.h"
#include <QString>
#include <QSharedPointer>

#include "RegularGridded.h"
#include "LongTaskMessage.h"

class SatelliteReader : public LongTaskMessage
{
public:
    struct Subdataset
    {
        Subdataset(const QString& metaName, const QString& metaDescription);

        QString path;
        QString description;
    };

    SatelliteReader();
    ~SatelliteReader();

    void openFile(const QString &fname);
    bool isOk() const;
    void closeCurrentFile();

    GDALRasterBand *getRecord(int bandNumber);
    GDALRasterBand *getRecord(int subdatasetNumber, int bandNumber);
    GDALRasterBand* getSubdataset(int subdatasetNumber);

    void transformMapToScreen(double lon, double lat, double *x, double *y);
    void transformScreenToMap(double x, double y, double *lon, double *lat);

    int getSubdatasetsNumber() const;
    int getBandsNumber() const;
    int getSubdatasetBandsNumber(int subdatasetNumber);
    QString getSubdatasetDescription(int subdatasetNumber);
    QString getBandDescription(int bandNumber) const;
    QString getSubdatasetBandDescriptiion(int subdatasetNumber, int bandNumber);

private:
    void initTransform();
    bool setGCPTransform();
    bool setGeoTransform(double adfGeoTransform[6]);
    void initSubdatasets();
    void openSubdataset(int subdatasetNumber);
    void closeSubdataset();

    enum class Transformation 
    {
        None,
        TransofrmArray,
        GCPTransform
    } transformation;

    QSharedPointer<char> transformAlg;
    GDALDataset *dataset;
    GDALDataset* subdataset;
    GDALDataset* activeDataset;
    int openSubdatasetNumber;
    double transform[6];
    double invTransform[6];
    QVector<Subdataset> subdatasets;
};
#endif // SATTELLITEREADER_H
