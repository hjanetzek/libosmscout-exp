#ifndef OSMSCOUT_OPTIMIZEWAYSLOWZOOM_H
#define OSMSCOUT_OPTIMIZEWAYSLOWZOOM_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <set>
#include <string>

#include <osmscout/TypeSet.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class OSMSCOUT_API OptimizeWaysLowZoom : public Referencable
  {
  private:
    struct TypeData
    {
      uint32_t   optLevel;       //! The display level this data was optimized for
      uint32_t   indexLevel;     //! Magnification level of index

      uint32_t   cellXStart;
      uint32_t   cellXEnd;
      uint32_t   cellYStart;
      uint32_t   cellYEnd;

      FileOffset bitmapOffset;   //! Position in file where the offset of the bitmap is written
      uint8_t    dataOffsetBytes;

      uint32_t   cellXCount;
      uint32_t   cellYCount;

    };

  private:
    std::string                           datafile;      //! Basename part for the data file name
    std::string                           datafilename;  //! complete filename for data file
    mutable FileScanner                   scanner;       //! File stream to the data file

    double                                magnification; //! Magnification, upto which we support optimization
    std::map<TypeId,std::list<TypeData> > wayTypesData;  //! Index information for all way types

  private:
    bool ReadTypeData(FileScanner& scanner,
                      TypeData& data);

    bool GetOffsets(const TypeData& typeData,
                    uint32_t minxc,
                    uint32_t minyc,
                    uint32_t maxxc,
                    uint32_t maxyc,
                    std::vector<FileOffset>& offsets) const;

  public:
    OptimizeWaysLowZoom();
    virtual ~OptimizeWaysLowZoom();

    bool Open(const std::string& path);
    bool Close();

    bool HasOptimizations(double magnification) const;

    bool GetWays(double lonMin, double latMin,
                 double lonMax, double latMax,
                 const Magnification& magnification,
                 size_t maxWayCount,
                 std::vector<TypeSet>& wayTypes,
                 std::vector<WayRef>& ways) const;
  };

  typedef Ref<OptimizeWaysLowZoom> OptimizeWaysLowZoomRef;
}

#endif
