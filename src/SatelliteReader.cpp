#include "SatelliteReader.h"
#include "Util.h"
#include <stdio.h>
#include <QDebug>

SatelliteReader::SatelliteReader()
{
    GDALAllRegister();
}

SatelliteReader::~SatelliteReader()
{
    closeCurrentFile();
}

void SatelliteReader::openFile(const QString &fileName, int nbrecs)
{
    qDebug() << "Satellite reader opens file";
    closeCurrentFile();
    emit newMessage (LongTaskMessage::LTASK_OPEN_FILE);
    dataset = (GDALDataset *)GDALOpen(qPrintable(fileName), GA_ReadOnly);
    if (dataset == nullptr)
    {
        fprintf(stderr, "ERROR: GDAL: could not load %s", qPrintable(fileName));
        return;
    }
}

bool SatelliteReader::isOk() const
{
    return dataset != nullptr;
}

void SatelliteReader::closeCurrentFile()
{
    if (dataset != nullptr) {
        GDALClose(dataset);
    }
}

GDALRasterBand* SatelliteReader::getRecord(time_t date)
{
    return dataset->GetRasterBand(1);
}

void SatelliteReader::getGeoTransform(double adfGeoTransform[6])
{
    if (dataset != nullptr)
        dataset->GetGeoTransform(adfGeoTransform);
}
