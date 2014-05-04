#ifndef OSMSCOUT_AREAAREAINDEX_H
#define OSMSCOUT_AREAAREAINDEX_H

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

#include <vector>

#include <osmscout/TypeSet.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
    \ingroup Database

    AreaAreaIndex allows you to find areas in
    a given region.

    For areas result can be limited by the maximum level (which in turn
    defines the minimum size of the resulting areas since an area in a given
    level must fit into the cell size (but can cross cell borders)) and the
    maximum number of areas found.

    Internally the index is implemented as quadtree. As a result each index entry
    has 4 children (besides entries in the lowest level).
    */
  class OSMSCOUT_API AreaAreaIndex : public Referencable
  {
  private:
    /**
      An individual index entry in a index cell
      */
    struct IndexEntry
    {
      TypeId     type;
      FileOffset offset;
    };

    /**
      Datastructure for every index cell of our index.
      */
    struct IndexCell
    {
      FileOffset              children[4]; //! File index of each of the four children, or 0 if there is no child
      std::vector<IndexEntry> areas;
    };

    typedef Cache<FileOffset,IndexCell> IndexCache;

    struct IndexCacheValueSizer : public IndexCache::ValueSizer
    {
      unsigned long GetSize(const IndexCell& value) const
      {
        unsigned long memory=0;

        memory+=sizeof(value);

        // Areas
        memory+=value.areas.size()*sizeof(IndexEntry);

        return memory;
      }
    };

    struct CellRef
    {
      FileOffset offset;
      size_t     x;
      size_t     y;

      CellRef(FileOffset offset,
              size_t x,
              size_t y)
      : offset(offset),
        x(x),
        y(y)
      {
        // no code
      }
    };

  private:
    std::string                     filepart;       //! name of the data file
    std::string                     datafilename;   //! Fullpath and name of the data file
    mutable FileScanner             scanner;        //! Scanner instance for reading this file

    uint32_t                        maxLevel;       //! Maximum level in index
    FileOffset                      topLevelOffset; //! File offset of the top level index entry

    mutable IndexCache              indexCache;     //! Cached map of all index entries by file offset

  private:
    bool GetIndexCell(uint32_t level,
                      FileOffset offset,
                      IndexCache::CacheRef& cacheRef) const;

  public:
    AreaAreaIndex(size_t cacheSize);

    void Close();
    bool Load(const std::string& path);

    bool GetOffsets(double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    size_t maxLevel,
                    const TypeSet& types,
                    size_t maxCount,
                    std::vector<FileOffset>& offsets) const;

    void DumpStatistics();
  };

  typedef Ref<AreaAreaIndex> AreaAreaIndexRef;
}

#endif
