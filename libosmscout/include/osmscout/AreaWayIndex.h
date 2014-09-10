#ifndef OSMSCOUT_AREAWAYINDEX_H
#define OSMSCOUT_AREAWAYINDEX_H

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

#include <vector>

#include <osmscout/TypeSet.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
    \ingroup Database
    AreaWayIndex allows you to find ways and way relations in
    a given area.

    Ways can be limited by type and result count.
    */
  class OSMSCOUT_API AreaWayIndex : public Referencable
  {
  private:
    struct TypeData
    {
      uint32_t   indexLevel;

      uint8_t    dataOffsetBytes;
      FileOffset bitmapOffset;

      uint32_t   cellXStart;
      uint32_t   cellXEnd;
      uint32_t   cellYStart;
      uint32_t   cellYEnd;
      uint32_t   cellXCount;
      uint32_t   cellYCount;

      TypeData();
    };

  private:
    std::string           filepart;       //! name of the data file
    std::string           datafilename;   //! Full path and name of the data file
    mutable FileScanner   scanner;        //! Scanner instance for reading this file

    std::vector<TypeData> wayTypeData;

  private:
    bool GetOffsets(const TypeData& typeData,
                    uint32_t minxc,
                    uint32_t minyc,
                    uint32_t maxxc,
                    uint32_t maxyc,
                    size_t maxWayCount,
                    OSMSCOUT_HASHSET<FileOffset>& offsets,
                    size_t currentSize,
                    bool& sizeExceeded) const;

  public:
    AreaWayIndex();

    void Close();
    bool Load(const std::string& path);

    bool GetOffsets(double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    const std::vector<TypeSet>& wayTypes,
                    size_t maxWayCount,
                    std::vector<FileOffset>& offsets) const;

    void DumpStatistics();
  };

  typedef Ref<AreaWayIndex> AreaWayIndexRef;
}

#endif
