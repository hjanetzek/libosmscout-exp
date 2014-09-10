/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/AreaAreaIndex.h>

#include <iostream>

#include <osmscout/system/Math.h>
#include <osmscout/util/QuadIndex.h>

namespace osmscout {

  AreaAreaIndex::AreaAreaIndex(size_t cacheSize)
  : filepart("areaarea.idx"),
    maxLevel(0),
    topLevelOffset(0),
    indexCache(cacheSize)
  {
    // no code
  }

  void AreaAreaIndex::Close()
  {
    if (scanner.IsOpen()) {
      scanner.Close();
    }
  }

  bool AreaAreaIndex::GetIndexCell(uint32_t level,
                                   FileOffset offset,
                                   IndexCache::CacheRef& cacheRef) const
  {
    if (!indexCache.GetEntry(offset,cacheRef)) {
      IndexCache::CacheEntry cacheEntry(offset);

      cacheRef=indexCache.SetEntry(cacheEntry);

      if (!scanner.IsOpen()) {
        if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
          std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
          return false;
        }
      }

      scanner.SetPos(offset);

      // Read offsets of children if not in the bottom level

      if (level<maxLevel) {
        for (size_t c=0; c<4; c++) {
          if (!scanner.ReadNumber(cacheRef->value.children[c])) {
            std::cerr << "Cannot read index data at offset " << offset << std::endl;
            return false;
          }
        }
      }
      else {
        for (size_t c=0; c<4; c++) {
          cacheRef->value.children[c]=0;
        }
      }

      // Now read the way offsets by type in this index entry

      uint32_t offsetCount;

      // Areas

      if (!scanner.ReadNumber(offsetCount)) {
        std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
        return false;
      }

      cacheRef->value.areas.resize(offsetCount);

      FileOffset prevOffset=0;

      for (size_t c=0; c<offsetCount; c++) {
        if (!scanner.ReadNumber(cacheRef->value.areas[c].type)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }
        if (!scanner.ReadNumber(cacheRef->value.areas[c].offset)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }

        cacheRef->value.areas[c].offset+=prevOffset;

        prevOffset=cacheRef->value.areas[c].offset;
      }
    }

    return true;
  }

  bool AreaAreaIndex::Load(const std::string& path)
  {
    datafilename=path+"/"+filepart;

    if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open file '" << datafilename << "'" << std::endl;
      return false;
    }

    if (!scanner.ReadNumber(maxLevel)) {
      std::cerr << "Cannot read data" << std::endl;
      return false;
    }

    if (!scanner.ReadFileOffset(topLevelOffset)) {
      std::cerr << "Cannot read data" << std::endl;
      return false;
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool
  AreaAreaIndex::GetOffsets(double minlon,
                            double minlat,
                            double maxlon,
                            double maxlat,
                            size_t maxLevel,
                            const TypeSet& types,
                            size_t maxCount,
                            std::vector<FileOffset>& offsets) const
                            {
    std::vector<CellRef> cellRefs;     // cells to scan in this level
    std::vector<CellRef> nextCellRefs; // cells to scan for the next level
    OSMSCOUT_HASHSET<FileOffset> newOffsets;   // offsets collected in the current level

    // cells at MAX_LEVEL (for one axis)
    size_t msize=(1<<QuadIndex::MAX_LEVEL);

    // Convert bbox to cells at MAX_LEVEL
    uint32_t xmin, xmax, ymin, ymax;
    QuadIndex::CellIds(minlon,maxlon,
                       minlat,maxlat,
                       QuadIndex::MAX_LEVEL,
                       xmin,xmax,
                       ymin,ymax);
    //std::cerr << "query " << minlon << " " << minlat << " " << maxlon << " " << maxlat << std::endl;
    //std::cerr << " cells: " << minX << " " << maxX << " / " << minY << " " << maxY << std::endl;

    // Clear result datastructures
    offsets.clear();

    // Make the vector preallocate memory for the expected data size
    // This should void reallocation
    offsets.reserve(std::min(100000u,(uint32_t) maxCount));
#if defined(OSMSCOUT_HASHSET_HAS_RESERVE)
    newOffsets.reserve(std::min(100000u,(uint32_t) maxCount));
#endif
    cellRefs.reserve(1000);

    nextCellRefs.reserve(1000);

    cellRefs.push_back(CellRef(topLevelOffset,0,0));

    // For all levels:
    // * Take the tiles and offsets of the last level
    // * Calculate the new tiles and offsets that still interfere with given area
    // * Add the new offsets to the list of offsets and finish if we have
    //   reached maxLevel or maxAreaCount.
    // * copy no, ntx, nty to ctx, cty, co and go to next iteration
    bool stopArea=false;
    for (uint32_t level=0;
         !stopArea &&
         level<=this->maxLevel &&
         level<=maxLevel &&
         !cellRefs.empty();
         level++) {
      nextCellRefs.clear();

      newOffsets.clear();

      for (size_t i=0; !stopArea && i<cellRefs.size(); i++) {
        size_t               cx;
        size_t               cy;
        IndexCache::CacheRef cell;

        if (!GetIndexCell(level,cellRefs[i].offset,cell)) {
          std::cerr << "Cannot find offset " << cellRefs[i].offset << " in level " << level << " => aborting!" << std::endl;
          return false;
        }

        if (offsets.size()+newOffsets.size()+cell->value.areas.size()>=maxCount) {
          stopArea=true;
          continue;
        }

        for (std::vector<IndexEntry>::const_iterator entry=cell->value.areas.begin();
             entry!=cell->value.areas.end();
             ++entry) {
          if (types.IsTypeSet(entry->type)) {
            newOffsets.insert(entry->offset);
          }
        }

        // children cell index (bottom-left)
        cx=cellRefs[i].x <<= 1;
        cy=cellRefs[i].y <<= 1;

        size_t clevel = QuadIndex::MAX_LEVEL - level;

        size_t cxmin = xmin>>(clevel);
        size_t cymin = ymin>>(clevel);

        size_t cxmax = xmax>>(clevel);
        size_t cymax = ymax>>(clevel);

        // size of children cell (in cells at MAX_LEVEL)
        size_t csize=msize>>(level+1);

        //std::cerr << "cell: " << level << "/"<< cellRefs[i].x << "/" << ((1 << level) - cellRefs[i].y - 1) << std::endl;
        //std::cerr << "size: " << csize << " / " << msize << std::endl;

        if (cell->value.children[0]!=0) {
          // top-left cell offset
          size_t x=cx*csize;
          size_t y=(cy+1)*csize;

          if ((x>xmin-csize)&&(y>ymin-csize)&&(x<xmax)&&(y<ymax)) {
            nextCellRefs.push_back(CellRef(cell->value.children[0],cx,cy+1));
          }
        }

        if (cell->value.children[1]!=0) {
          // top-right cell offset
          size_t x=(cx+1)*csize;
          size_t y=(cy+1)*csize;

          if ((x>xmin-csize)&&(y>ymin-csize)&&(x<xmax)&&(y<ymax)) {
            nextCellRefs.push_back(CellRef(cell->value.children[1],cx+1,cy+1));
          }
        }

        if (cell->value.children[2]!=0) {
          // bottom-left cell offset
          size_t x=cx*csize;
          size_t y=cy*csize;

          if ((x>xmin-csize)&&(y>ymin-csize)&&(x<xmax)&&(y<ymax)) {
            nextCellRefs.push_back(CellRef(cell->value.children[2],cx,cy));
          }
        }

        if (cell->value.children[3]!=0) {
          // bottom-right cell offset
          size_t x=(cx+1)*csize;
          size_t y=cy*csize;

          if ((x>xmin-csize)&&(y>ymin-csize)&&(x<xmax)&&(y<ymax)) {
            nextCellRefs.push_back(CellRef(cell->value.children[3],cx+1,cy));
          }
        }
      }

      if (!stopArea) {
        offsets.insert(offsets.end(),newOffsets.begin(),newOffsets.end());
      }

      std::swap(cellRefs,nextCellRefs);
    }

    return true;
  }

  void AreaAreaIndex::DumpStatistics()
  {
    indexCache.DumpStatistics(filepart.c_str(),IndexCacheValueSizer());
  }
}

