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

#include <osmscout/AreaWayIndex.h>

#include <iostream>

#include <osmscout/system/Math.h>
#include <osmscout/util/QuadIndex.h>

namespace osmscout {

  AreaWayIndex::TypeData::TypeData()
  : indexLevel(0),
    dataOffsetBytes(0),
    bitmapOffset(0),
    cellXStart(0),
    cellXEnd(0),
    cellYStart(0),
    cellYEnd(0),
    cellXCount(0),
    cellYCount(0)
  {
  }

  AreaWayIndex::AreaWayIndex()
  : filepart("areaway.idx")
  {
    // no code
  }

  void AreaWayIndex::Close()
  {
    if (scanner.IsOpen()) {
      scanner.Close();
    }
  }

  bool AreaWayIndex::Load(const std::string& path)
  {
    datafilename=path+"/"+filepart;

    if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open file '" << datafilename << "'" << std::endl;
      return false;
    }

    uint32_t indexEntries;

    scanner.Read(indexEntries);

    for (size_t i=0; i<indexEntries; i++) {
      TypeId type;

      scanner.ReadNumber(type);

      if (type>=wayTypeData.size()) {
        wayTypeData.resize(type+1);
      }

      scanner.ReadFileOffset(wayTypeData[type].bitmapOffset);

      if (wayTypeData[type].bitmapOffset>0) {
        scanner.Read(wayTypeData[type].dataOffsetBytes);

        scanner.ReadNumber(wayTypeData[type].indexLevel);

        scanner.ReadNumber(wayTypeData[type].cellXStart);
        scanner.ReadNumber(wayTypeData[type].cellXEnd);
        scanner.ReadNumber(wayTypeData[type].cellYStart);
        scanner.ReadNumber(wayTypeData[type].cellYEnd);

        wayTypeData[type].cellXCount=wayTypeData[type].cellXEnd-wayTypeData[type].cellXStart+1;
        wayTypeData[type].cellYCount=wayTypeData[type].cellYEnd-wayTypeData[type].cellYStart+1;
      }
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool AreaWayIndex::GetOffsets(const TypeData& typeData,
                                uint32_t minxc,
                                uint32_t minyc,
                                uint32_t maxxc,
                                uint32_t maxyc,
                                size_t maxWayCount,
                                OSMSCOUT_HASHSET<FileOffset>& offsets,
                                size_t currentSize,
                                bool& sizeExceeded) const
  {
    if (typeData.bitmapOffset==0) {
      // No data for this type available
      return true;
    }

    uint32_t levelShift = (QuadIndex::MAX_LEVEL - typeData.indexLevel);
    minxc >>= levelShift;
    minyc >>= levelShift;
    maxxc >>= levelShift;
    maxyc >>= levelShift;

    if (minxc > typeData.cellXEnd ||
            maxxc < typeData.cellXStart ||
            minyc > typeData.cellYEnd ||
            maxyc < typeData.cellYStart){
        // No data available in given bounding box
        return true;
    }

//    int level = typeData.indexLevel;
//    std::cerr << "scan: "  <<  level << " ... "<< minxc << " " << ((1 << level) - minyc -1)
//                                        << " " << maxxc << " " << ((1 << level) - maxyc -1)
//                                        << std::endl;

    minxc=std::max(minxc,typeData.cellXStart);
    maxxc=std::min(maxxc,typeData.cellXEnd);

    minyc=std::max(minyc,typeData.cellYStart);
    maxyc=std::min(maxyc,typeData.cellYEnd);

    FileOffset dataOffset=typeData.bitmapOffset+
                          typeData.cellXCount*typeData.cellYCount*(FileOffset)typeData.dataOffsetBytes;

    // For each row
    for (size_t y=minyc; y<=maxyc; y++) {
      FileOffset initialCellDataOffset=0;
      size_t     cellDataOffsetCount=0;
      FileOffset bitmapCellOffset=typeData.bitmapOffset+
                                  ((y-typeData.cellYStart)*typeData.cellXCount+
                                   minxc-typeData.cellXStart)*(FileOffset)typeData.dataOffsetBytes;

      if (!scanner.SetPos(bitmapCellOffset)) {
        std::cerr << "Cannot go to type cell index position " << bitmapCellOffset << std::endl;
        return false;
      }

      // For each column in row
      for (size_t x=minxc; x<=maxxc; x++) {
        FileOffset cellDataOffset;

        if (!scanner.ReadFileOffset(cellDataOffset,
                                    typeData.dataOffsetBytes)) {
          std::cerr << "Cannot read cell data position" << std::endl;
          return false;
        }

        if (cellDataOffset==0) {
          continue;
        }

        // We added +1 during import and now substract it again
        cellDataOffset--;

        if (initialCellDataOffset==0) {
          initialCellDataOffset=dataOffset+cellDataOffset;
        }

        cellDataOffsetCount++;
      }

      // We did not find any cells in the current row
      if (cellDataOffsetCount==0) {
        continue;
      }

      // The first data entry must be positioned behind the bitmap
      assert(initialCellDataOffset>=bitmapCellOffset);

      // first data entry in the row
      if (!scanner.SetPos(initialCellDataOffset)) {
        std::cerr << "Cannot go to cell data position " << initialCellDataOffset << std::endl;
        return false;
      }

      // For each data cell (in range) in row found
      for (size_t i=0; i<cellDataOffsetCount; i++) {
        uint32_t   dataCount;
        FileOffset lastOffset=0;


        if (!scanner.ReadNumber(dataCount)) {
          std::cerr << "Cannot read cell data count" << std::endl;
          return false;
        }

        if (currentSize+offsets.size()+dataCount>maxWayCount) {
          //std::cout << currentSize<< "+" << newOffsets.size() << "+" << dataCount << ">" << maxWayCount << std::endl;
          sizeExceeded=true;
          return true;
        }

        for (size_t d=0; d<dataCount; d++) {
          FileOffset objectOffset;

          scanner.ReadNumber(objectOffset);

          objectOffset+=lastOffset;

          offsets.insert(objectOffset);

          lastOffset=objectOffset;
        }
      }
    }

    return true;
  }

  bool AreaWayIndex::GetOffsets(double minlon,
                                double minlat,
                                double maxlon,
                                double maxlat,
                                const std::vector<TypeSet>& wayTypes,
                                size_t maxWayCount,
                                std::vector<FileOffset>& offsets) const
  {
    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    bool                         sizeExceeded=false;
    OSMSCOUT_HASHSET<FileOffset> newOffsets;

    offsets.reserve(std::min(100000u,(uint32_t)maxWayCount));

#if defined(OSMSCOUT_HASHSET_HAS_RESERVE)
    newOffsets.reserve(std::min(100000u,(uint32_t)maxWayCount));
#endif

    uint32_t minxc, maxxc, minyc, maxyc;
    QuadIndex::CellIds(minlon, maxlon,
                       minlat, maxlat,
                       QuadIndex::MAX_LEVEL,
                       minxc, maxxc,
                       minyc, maxyc);

    for (size_t i=0; i<wayTypes.size(); i++) {
      newOffsets.clear();

      for (size_t type=0;
          type<wayTypeData.size();
          ++type) {
        if (wayTypes[i].IsTypeSet(type)) {
          if (!GetOffsets(wayTypeData[type],
                          minxc,
                          minyc,
                          maxxc,
                          maxyc,
                          maxWayCount,
                          newOffsets,
                          offsets.size(),
                          sizeExceeded)) {
            return false;
          }

          if (sizeExceeded) {
            return true;
          }
        }
      }

      // Copy data from temporary set to final vector

      offsets.insert(offsets.end(),newOffsets.begin(),newOffsets.end());
    }

    //std::cout << "Found " << wayWayOffsets.size() << "+" << relationWayOffsets.size()<< " offsets in 'areaway.idx'" << std::endl;

    return true;
  }

  void AreaWayIndex::DumpStatistics()
  {
  }
}

