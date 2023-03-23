#include "SatelliteReader.h"
#include "Util.h"
#include <qvector.h>
#include <stdio.h>
#include <QDebug>
#include "cpl_error.h"
#include "gdal.h"
#include "gdal_alg.h"
#include "cpl_conv.h"
#include "gdal_priv.h"

SatelliteReader::SatelliteReader()
    : transformation(Transformation::None), dataset(nullptr), subdataset(nullptr), activeDataset(nullptr), openSubdatasetNumber(-1)
{
    CPLSetConfigOption("L1B_HIGH_GCP_DENSITY", "NO");
    GDALAllRegister();
}

SatelliteReader::~SatelliteReader()
{
    closeCurrentFile();
}

SatelliteReader::Subdataset::Subdataset(const QString& metaName, const QString& metaDescription)
{
    path = metaName.section('=', 1);
    description  = metaDescription.section('=', 1);
}

void SatelliteReader::openFile(const QString &fileName)
{
    closeCurrentFile();
    emit newMessage(LongTaskMessage::LTASK_OPEN_FILE);
    dataset = (GDALDataset *)GDALOpen(qPrintable(fileName), GA_ReadOnly);
    if (dataset == nullptr)
    {
        fprintf(stderr, "ERROR: GDAL: could not load %s", qPrintable(fileName));
        return;
    }
    initSubdatasets();
    initTransform();
}

void SatelliteReader::openSubdataset(int subdatasetNumber)
{
    if (openSubdatasetNumber == subdatasetNumber)
        return;
    closeSubdataset();
    subdataset = static_cast<GDALDataset*>(GDALOpen(qPrintable(subdatasets[subdatasetNumber].path), GA_ReadOnly));
    activeDataset = subdataset;
    openSubdatasetNumber = subdatasetNumber;
    if (subdataset != nullptr)
        initTransform();
}

void SatelliteReader::closeSubdataset()
{
    if (subdataset == nullptr)
        return;

    GDALClose(subdataset);
    subdataset = nullptr;
    activeDataset = dataset;
    openSubdatasetNumber = -1;
    initTransform();
}


bool SatelliteReader::isOk() const
{
    return dataset != nullptr && getBandsNumber() + getSubdatasetsNumber() > 0;
}

void SatelliteReader::closeCurrentFile()
{
    closeSubdataset();
    if (dataset != nullptr)
    {
        GDALClose(dataset);
        dataset = nullptr;
        subdatasets.clear();
        activeDataset = nullptr;
    }
}

GDALRasterBand* SatelliteReader::getRecord(int bandNumber)
{
    if (dataset == nullptr || bandNumber > dataset->GetRasterCount())
        return nullptr;

    closeSubdataset();
    return dataset->GetRasterBand(bandNumber);
}

GDALRasterBand *SatelliteReader::getRecord(int subdatasetNumber, int bandNumber)
{
    openSubdataset(subdatasetNumber);
    if (subdataset == nullptr || bandNumber > subdataset->GetRasterCount())
        return nullptr;
    return subdataset->GetRasterBand(bandNumber);
}

GDALRasterBand* SatelliteReader::getSubdataset(int subdatasetNumber)
{
    openSubdataset(subdatasetNumber);
    if (subdataset == nullptr)
    {
        qDebug() << "Could not open subdataset: " << subdatasets[subdatasetNumber].path;
        return nullptr;
    }
    if (subdataset->GetRasterCount() == 0)
    {
        qDebug() << "Subdataset is empty";
        return nullptr;
    }
    return subdataset->GetRasterBand(subdataset->GetRasterCount());
}

bool SatelliteReader::setGeoTransform(double adfGeoTransform[6])
{
    CPLErr err = activeDataset->GetGeoTransform(adfGeoTransform);
    if (err != CE_None)
        return false;

    transformation = Transformation::TransofrmArray;
    return true;
}

void SatelliteReader::transformMapToScreen(double lon, double lat, double *x, double *y)
{
    if (transformation == Transformation::TransofrmArray)
        GDALApplyGeoTransform(invTransform, lon, lat, x, y);
    else if (transformation == Transformation::GCPTransform)
    {
        *x = lon;
        *y = lat;
        double z = 0;
        int success;
        GDALTPSTransform(transformAlg.data(), true, 1, x, y, &z, &success);
    }
}

void SatelliteReader::transformScreenToMap(double x, double y, double *lon, double *lat)
{
    if (transformation == Transformation::TransofrmArray)
    {
        GDALApplyGeoTransform(transform, x, y, lon, lat);
    }
    else if (transformation == Transformation::GCPTransform)
    {
        *lon = x;
        *lat = y;
        double z = 0;
        int success;
        GDALGCPTransform(transformAlg.data(), false, 1, lon, lat, &z, &success);
    }
}

void SatelliteReader::initSubdatasets()
{
    char **papszSubdatasets = dataset->GetMetadata("SUBDATASETS");
    if (papszSubdatasets == nullptr)
        return;
    for (int i = 0; papszSubdatasets[i] != nullptr; i += 2)
        subdatasets.push_back(Subdataset(papszSubdatasets[i], papszSubdatasets[i + 1]));
}

void SatelliteReader::initTransform()
{
    if (activeDataset == nullptr)
    {
        transformation = Transformation::None;
        return;
    }
    if (setGeoTransform(transform))
    {
        GDALInvGeoTransform(transform, invTransform);
        return;
    }
    if (setGCPTransform())
        return;
    
    
    qDebug() << "Could not create transform";
    transformation = Transformation::None;
}

QVector<GDAL_GCP> filterGCPS(const GDAL_GCP* GCPs, int GCPCount)
{
    QVector<GDAL_GCP> filtered;
    for (int i = 0; i < GCPCount; ++i)
    {
        if (-180 <= GCPs[i].dfGCPX && GCPs[i].dfGCPX <= 180)
            filtered.push_back(GCPs[i]);
    }
    return filtered;
}

bool SatelliteReader::setGCPTransform()
{
    auto GCPs = activeDataset->GetGCPs();
    int GCPCount = activeDataset->GetGCPCount();
    QVector<GDAL_GCP> filteredGCPs = filterGCPS(GCPs, GCPCount);
    if (filteredGCPs.empty())
        return false;
    transformAlg = QSharedPointer<char>((char*)GDALCreateTPSTransformer(filteredGCPs.count(), &filteredGCPs[0], false),
                                        GDALDestroyTPSTransformer);

    if (transformAlg != nullptr)
    {
        transformation = Transformation::GCPTransform;
        return true;
    }
    return false;
}

int SatelliteReader::getSubdatasetsNumber() const
{
    return subdatasets.size();
}

int SatelliteReader::getBandsNumber() const
{
    if (dataset == nullptr)
        return 0;
    return dataset->GetRasterCount();
}

int SatelliteReader::getSubdatasetBandsNumber(int subdatasetNumber)
{
    openSubdataset(subdatasetNumber);
    if (subdataset == nullptr)
        return 0;
    return subdataset->GetRasterCount();
}

QString SatelliteReader::getSubdatasetDescription(int subdatasetNumber)
{
    if (subdatasetNumber >= subdatasets.size())
        return tr("Subdataset not found");
    return subdatasets[subdatasetNumber].description;
}

QString SatelliteReader::getBandDescription(int bandNumber) const
{
    if (dataset != nullptr && bandNumber <= dataset->GetRasterCount())
        return dataset->GetRasterBand(bandNumber)->GetDescription();
    return tr("Layer not found");
}

QString SatelliteReader::getSubdatasetBandDescriptiion(int subdatasetNumber, int bandNumber)
{
    openSubdataset(subdatasetNumber);
    if (subdataset != nullptr && bandNumber <= subdataset->GetRasterCount())
        return tr("Band ") + "#" + QString::number(bandNumber);
    return tr("Layer not found");
}
