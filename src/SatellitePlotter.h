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

#ifndef SATELLITEPLOTTER_H
#define SATELLITEPLOTTER_H

#include <QPainter>
#include "gdal_priv.h"
#include <QSharedPointer>

#include "DataPointInfo.h"
#include "LongTaskProgress.h"
#include "IsoLine.h"
#include "SatelliteReader.h"
 
//===============================================================
class SatellitePlotter
{
    public:
        SatellitePlotter ();
        SatellitePlotter (const SatellitePlotter &);

        void draw(QPainter& pnt, Projection *proj);
        
		void loadFile (const QString &fileName,
						LongTaskProgress *taskProgress=nullptr);
		
		void setCurrentDate (time_t t);

        bool isReaderOk() const;
        void setLayer(int newLayer);
        void setLayer(int subdataset, int newLayer);
        SatelliteReader* getReader();
    
    private:
        QSharedPointer<SatelliteReader> reader;
        QString fileName;
        time_t currentDate;
        int layer;
        int subdataset;
        bool rgb;
};

#endif // SATELLITEPLOTTER_H
