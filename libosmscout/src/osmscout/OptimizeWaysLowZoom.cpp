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

#include <osmscout/OptimizeWaysLowZoom.h>

#include <osmscout/Way.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>
#include <osmscout/util/QuadIndex.h>

#include <iostream>

namespace osmscout
{
  OptimizeWaysLowZoom::OptimizeWaysLowZoom()
  : datafile("waysopt.dat"),
    magnification(0.0)
  {
    // no code
  }

  OptimizeWaysLowZoom::~OptimizeWaysLowZoom()
  {
    if (scanner.IsOpen()) {
      Close();
    }
  }

  bool OptimizeWaysLowZoom::ReadTypeData(FileScanner& scanner,
                                         OptimizeWaysLowZoom::TypeData& data)
  {
    scanner.Read(data.optLevel);
    scanner.Read(data.indexLevel);
    scanner.Read(data.cellXStart);
    scanner.Read(data.cellXEnd);
    scanner.Read(data.cellYStart);
    scanner.Read(data.cellYEnd);

    scanner.ReadFileOffset(data.bitmapOffset);
    scanner.Read(data.dataOffsetBytes);

    data.cellXCount=data.cellXEnd-data.cellXStart+1;
    data.cellYCount=data.cellYEnd-data.cellYStart+1;


    return !scanner.HasError();
  }

  bool OptimizeWaysLowZoom::Open(const std::string& path)
  {
    datafilename=AppendFileToDir(path,datafile);

    if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
      std::cout << "Cannot open file '" << scanner.GetFilename() << "'!" << std::endl;
      return false;
    }

    FileOffset indexOffset;

    if (!scanner.ReadFileOffset(indexOffset)) {
      std::cout << "Cannot read index offset!" << std::endl;
      return false;
    }

    if (!scanner.SetPos(indexOffset)) {
      std::cout << "Cannot goto to start of index at position " << indexOffset << "!" << std::endl;
      return false;
    }

    uint32_t optimizationMaxMag;
    uint32_t wayTypeCount;

    scanner.Read(optimizationMaxMag);
    scanner.Read(wayTypeCount);

    if (scanner.HasError()) {
      return false;
    }

    magnification=pow(2.0,(int)optimizationMaxMag);

    for (size_t i=1; i<=wayTypeCount; i++) {
      TypeId typeId;

      scanner.Read(typeId);

      TypeData typeData;

      if (!ReadTypeData(scanner,
                        typeData)) {
        return false;
      }

      wayTypesData[typeId].push_back(typeData);
    }

    return !scanner.HasError();
  }

  bool OptimizeWaysLowZoom::Close()
  {
    bool success=true;

    if (scanner.IsOpen()) {
      if (!scanner.Close()) {
        success=false;
      }
    }

    return success;
  }

  bool OptimizeWaysLowZoom::HasOptimizations(double magnification) const
  {
    return magnification<=this->magnification;
  }

  bool OptimizeWaysLowZoom::GetOffsets(const TypeData& typeData,
                                       uint32_t minxc,
                                       uint32_t minyc,
                                       uint32_t maxxc,
                                       uint32_t maxyc,
                                       std::vector<FileOffset>& offsets) const
  {
    std::set<FileOffset> newOffsets;

    if (typeData.bitmapOffset==0) {
      // No data for this type available
      return true;
    }

    uint32_t shift = (QuadIndex::MAX_LEVEL - typeData.indexLevel);
	minxc >>= shift;
	minyc >>= shift;
	maxxc >>= shift;
	maxyc >>= shift;

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

    FileOffset dataOffset=typeData.bitmapOffset+
                          typeData.cellXCount*typeData.cellYCount*(FileOffset)typeData.dataOffsetBytes;

    // For each row
    for (size_t y=minyc; y<=maxyc; y++) {
      FileOffset initialCellDataOffset=0;
      size_t     cellDataOffsetCount=0;
      FileOffset cellIndexOffset=typeData.bitmapOffset+
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

        for (size_t d=0; d<dataCount; d++) {
          FileOffset objectOffset;

          scanner.ReadNumber(objectOffset);

          objectOffset+=lastOffset;

          newOffsets.insert(objectOffset);

          lastOffset=objectOffset;
        }
      }
    }

    for (std::set<FileOffset>::const_iterator offset=newOffsets.begin();
         offset!=newOffsets.end();
         ++offset) {
      offsets.push_back(*offset);
    }

    return true;
  }

  bool OptimizeWaysLowZoom::GetWays(double minlon, double minlat,
                                    double maxlon, double maxlat,
                                    const Magnification& magnification,
                                    size_t /*maxWayCount*/,
                                    std::vector<TypeSet>& wayTypes,
                                    std::vector<WayRef>& ways) const
  {
    std::vector<FileOffset> offsets;

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    offsets.reserve(20000);

    uint32_t minxc, maxxc, minyc, maxyc;
    QuadIndex::CellIds(minlon, maxlon,
                       minlat, maxlat,
                       QuadIndex::MAX_LEVEL,
                       minxc, maxxc,
                       minyc, maxyc);

    for (size_t i=0; i<wayTypes.size(); i++) {
      for (std::map<TypeId,std::list<TypeData> >::const_iterator type=wayTypesData.begin();
          type!=wayTypesData.end();
          ++type) {
        if (wayTypes[i].IsTypeSet(type->first)) {
          std::list<TypeData>::const_iterator match=type->second.end();

          for (std::list<TypeData>::const_iterator typeData=type->second.begin();
              typeData!=type->second.end();
              ++typeData) {
            if (typeData->optLevel==magnification.GetLevel()) {
              match=typeData;
            }
          }

          if (match!=type->second.end()) {
            if (match->bitmapOffset!=0) {
              if (!GetOffsets(*match,
                              minxc,
                              minyc,
                              maxxc,
                              maxyc,
                              offsets)) {
                return false;
              }

              for (std::vector<FileOffset>::const_iterator offset=offsets.begin();
                  offset!=offsets.end();
                  ++offset) {
                if (!scanner.SetPos(*offset)) {
                  std::cerr << "Error while positioning in file " << datafilename  << std::endl;
                  type++;
                  continue;
                }

                WayRef way=new Way();

                if (!way->ReadOptimized(scanner)) {
                  std::cerr << "Error while reading data entry of type " << type->first << " from file " << datafilename  << std::endl;
                  continue;
                }

                ways.push_back(way);
              }

              offsets.clear();
            }
          }

          if (match!=type->second.end()) {
            wayTypes[i].UnsetType(type->first);
          }
        }
      }
    }

    return true;
  }
}

