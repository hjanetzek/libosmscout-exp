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

#include <osmscout/AreaNodeIndex.h>

#include <iostream>

#include <osmscout/system/Math.h>
#include <osmscout/util/QuadIndex.h>

namespace osmscout {

  AreaNodeIndex::TypeData::TypeData()
  : indexLevel(0),
    indexOffset(0),
    dataOffsetBytes(0),
    cellXStart(0),
    cellXEnd(0),
    cellYStart(0),
    cellYEnd(0),
    cellXCount(0),
    cellYCount(0)
  {
  }

  AreaNodeIndex::AreaNodeIndex()
  : filepart("areanode.idx")
  {
    // no code
  }

  void AreaNodeIndex::Close()
  {
    if (scanner.IsOpen()) {
      scanner.Close();
    }
  }

  bool AreaNodeIndex::Load(const std::string& path)
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

      if (type>=nodeTypeData.size()) {
        nodeTypeData.resize(type+1);
      }

      scanner.ReadFileOffset(nodeTypeData[type].indexOffset);
      scanner.Read(nodeTypeData[type].dataOffsetBytes);

      scanner.ReadNumber(nodeTypeData[type].indexLevel);

      scanner.ReadNumber(nodeTypeData[type].cellXStart);
      scanner.ReadNumber(nodeTypeData[type].cellXEnd);
      scanner.ReadNumber(nodeTypeData[type].cellYStart);
      scanner.ReadNumber(nodeTypeData[type].cellYEnd);

      nodeTypeData[type].cellXCount=nodeTypeData[type].cellXEnd-nodeTypeData[type].cellXStart+1;
      nodeTypeData[type].cellYCount=nodeTypeData[type].cellYEnd-nodeTypeData[type].cellYStart+1;
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool AreaNodeIndex::GetOffsets(const TypeData& typeData,
                                 uint32_t minxc,
                                 uint32_t minyc,
                                 uint32_t maxxc,
                                 uint32_t maxyc,
                                 size_t maxNodeCount,
                                 std::vector<FileOffset>& offsets,
                                 size_t currentSize,
                                 bool& sizeExceeded) const
  {
    if (typeData.indexOffset==0) {
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

    minxc=std::max(minxc,typeData.cellXStart);
    maxxc=std::min(maxxc,typeData.cellXEnd);

    minyc=std::max(minyc,typeData.cellYStart);
    maxyc=std::min(maxyc,typeData.cellYEnd);

    OSMSCOUT_HASHSET<FileOffset> newOffsets;

    FileOffset dataOffset=typeData.indexOffset+
                          typeData.cellXCount*typeData.cellYCount*(FileOffset)typeData.dataOffsetBytes;

    // For each row
    for (size_t y=minyc; y<=maxyc; y++) {
      FileOffset initialCellDataOffset=0;
      size_t     cellDataOffsetCount=0;
      FileOffset cellIndexOffset=typeData.indexOffset+
                                 ((y-typeData.cellYStart)*typeData.cellXCount+
                                  minxc-typeData.cellXStart)*typeData.dataOffsetBytes;

      if (!scanner.SetPos(cellIndexOffset)) {
        std::cerr << "Cannot go to type cell index position " << cellIndexOffset << std::endl;
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

      if (cellDataOffsetCount==0) {
        continue;
      }

      assert(initialCellDataOffset>=cellIndexOffset);

      if (!scanner.SetPos(initialCellDataOffset)) {
        std::cerr << "Cannot go to cell data position " << initialCellDataOffset << std::endl;
        return false;
      }

      // For each data cell in row found
      for (size_t i=0; i<cellDataOffsetCount; i++) {
        uint32_t   dataCount;
        FileOffset lastOffset=0;


        if (!scanner.ReadNumber(dataCount)) {
          std::cerr << "Cannot read cell data count" << std::endl;
          return false;
        }

        if (currentSize+newOffsets.size()+dataCount>maxNodeCount) {
          sizeExceeded=true;
          return true;
        }

        for (size_t d=0; d<dataCount; d++) {
          FileOffset objectOffset;

          scanner.ReadNumber(objectOffset);

          objectOffset+=lastOffset;

          newOffsets.insert(objectOffset);

          lastOffset=objectOffset;
        }
      }
    }

    offsets.insert(offsets.end(),newOffsets.begin(),newOffsets.end());

    return true;
  }

  bool AreaNodeIndex::GetOffsets(double minlon,
                                 double minlat,
                                 double maxlon,
                                 double maxlat,
                                 const TypeSet& nodeTypes,
                                 size_t maxNodeCount,
                                 std::vector<FileOffset>& nodeOffsets) const
  {
    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    bool sizeExceeded=false;

    uint32_t minxc, maxxc, minyc, maxyc;
    QuadIndex::CellIds(minlon, maxlon,
                       minlat, maxlat,
                       QuadIndex::MAX_LEVEL,
                       minxc, maxxc,
                       minyc, maxyc);

    for (size_t i=0; i<nodeTypeData.size(); i++) {
      if (nodeTypes.IsTypeSet(i)) {
        if (!GetOffsets(nodeTypeData[i],
                        minxc,
                        minyc,
                        maxxc,
                        maxyc,
                        maxNodeCount,
                        nodeOffsets,
                        nodeOffsets.size(),
                        sizeExceeded)) {
          return false;
        }

        if (sizeExceeded) {
          break;
        }
      }
    }

    return true;
  }

  void AreaNodeIndex::DumpStatistics()
  {
  }
}

