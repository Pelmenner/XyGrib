#include "SatelliteReader.h"
#include "Util.h"
#include <stdio.h>
#include <QDebug>
#include "gdal_alg.h"
#include "cpl_conv.h"

SatelliteReader::SatelliteReader()
    : transformation(Transformation::None), dataset(nullptr)
{
    CPLSetConfigOption("L1B_HIGH_GCP_DENSITY", "NO");
    GDALAllRegister();
}

SatelliteReader::~SatelliteReader()
{
    closeCurrentFile();
}

void SatelliteReader::openFile(const QString &fileName)
{
    closeCurrentFile();
    emit newMessage(LongTaskMessage::LTASK_OPEN_FILE);
    dataset = (GDALDataset *)GDALOpen(qPrintable(fileName), GA_ReadOnly);
    if (dataset == nullptr)
        fprintf(stderr, "ERROR: GDAL: could not load %s", qPrintable(fileName));
    else
        initTransform();

    // dataset->GetSpatialRef();
    // OGRSpatialReference oSourceSRS, oTargetSRS;

    // OGRCoordinateTransformation *poCT;
    // double x, y;

    // oSourceSRS.importFromEPSG( atoi(papszArgv[i+1]) );
    // oTargetSRS.importFromEPSG( atoi(papszArgv[i+2]) );

    // poCT = OGRCreateCoordinateTransformation( &oSourceSRS,
    //                                         &oTargetSRS );
    // x = atof( papszArgv[i+3] );
    // y = atof( papszArgv[i+4] );

    // if( poCT == NULL || !poCT->Transform( 1, &x, &y ) )
    //     printf( "Transformation failed.\n" );
    // else
    // {
    //     printf( "(%f,%f) -> (%f,%f)\n",
    //             atof( papszArgv[i+3] ),
    //             atof( papszArgv[i+4] ),
    //             x, y );
    // }
}

bool SatelliteReader::isOk() const
{
    return dataset != nullptr && dataset->GetRasterCount() > 0;
}

void SatelliteReader::closeCurrentFile()
{
    if (dataset == nullptr)
        return;

    GDALClose(dataset);
    dataset = nullptr;
}

GDALRasterBand *SatelliteReader::getRecord(int bandNumber) const
{
    return dataset->GetRasterBand(bandNumber);
}

GDALRasterBand *SatelliteReader::getRecord() const
{
    return dataset->GetRasterBand(getBandsNumber());
}

bool SatelliteReader::setGeoTransform(double adfGeoTransform[6])
{
    auto err = dataset->GetGeoTransform(adfGeoTransform);
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
        GDALApplyGeoTransform(transform, x, y, lon, lat);
    else if (transformation == Transformation::GCPTransform)
    {
        *lon = x;
        *lat = y;
        double z = 0;
        int success;
        GDALGCPTransform(transformAlg.data(), false, 1, lon, lat, &z, &success);
    }
}

void SatelliteReader::initTransform()
{
    if (setGeoTransform(transform))
    {
        GDALInvGeoTransform(transform, invTransform);
        return;
    }
    auto GCPs = dataset->GetGCPs();
    int GCPCount = dataset->GetGCPCount();
    transformAlg = QSharedPointer<char>((char*)GDALCreateTPSTransformer(GCPCount, GCPs, false),
                                        GDALDestroyTPSTransformer);

    if (transformAlg != nullptr)
    {
        transformation = Transformation::GCPTransform;
        return;
    }
    
    qDebug() << "Could not create transform";
    transformation = Transformation::None;
}

int SatelliteReader::getBandsNumber() const
{
    if (dataset == nullptr)
        return 0;
    return dataset->GetRasterCount();
}

const char *SatelliteReader::getBandDescription(int bandNumber) const
{
    if (dataset == nullptr)
        return "";
    return dataset->GetRasterBand(bandNumber)->GetDescription();
}
