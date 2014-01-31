#ifndef OSMSCOUT_NODE_H
#define OSMSCOUT_NODE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/GeoCoord.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

    class OSMSCOUT_API NodeAttributes
    {
    private:
        // availability flags (for optimized attribute storage)
        const static uint8_t hasCity         = 1 << 1; //! City object offset and...
        const static uint8_t hasCityAsNode   = 1 << 2; //! City object offset and...
        const static uint8_t hasStreet       = 1 << 3; //! Street object offset
        const static uint8_t hasNameAlt      = 1 << 4; //! We have an alternative name (mainly in a second language)
        const static uint8_t hasName         = 1 << 5; //! We have a name
        const static uint8_t hasAddress      = 1 << 6; //! ...house number
        const static uint8_t hasTags         = 1 << 7; //! We have additional tags

        // data
        mutable uint8_t  flags;
        std::string      name;     //! name
        std::string      nameAlt;  //! alternative name
        std::string      address;  //! house number

        bool             cityIsNode;
        std::vector<Tag> tags;     //! list of preparsed tags

        FileOffset       street;   //! Must be a way type
        FileOffset       city;     //! Can be a node or area
        bool streetIsEmpty;
        bool cityIsEmpty;

    private:
        void GetFlags(uint8_t& flags) const;
        bool Read(FileScanner& scanner);
        bool Write(FileWriter& writer) const;

        friend class Node;

    public:
        inline NodeAttributes()
        : flags(0),
          street(0),
          city(0),
          cityIsNode(false),
          streetIsEmpty(true),
          cityIsEmpty(true)
        {
          // no code
        }

        inline uint8_t GetFlags() const
        {
          return flags;
        }

        inline std::string GetName() const
        {
          return name;
        }

        inline std::string GetNameAlt() const
        {
          return nameAlt;
        }

        inline std::string GetAddress() const
        {
          return address;
        }

        inline FileOffset GetCity() const
        {
            return city;
        }

        inline bool GetCityTypeIsNode() const
        {
            return ((this->HasCity()) && (flags&hasCityAsNode)) != 0;
        }

        inline FileOffset GetStreet() const
        {
            return street;
        }

        inline bool HasCity() const
        {
            return ((flags&hasCity) && (!cityIsEmpty)) != 0;
        }

        inline bool HasStreet() const
        {
            return ((flags&hasStreet) && (!streetIsEmpty)) != 0;
        }

        inline bool HasTags() const
        {
          return !tags.empty();
        }

        inline const std::vector<Tag>& GetTags() const
        {
          return tags;
        }


        bool SetTags(Progress& progress,
                     const TypeConfig& typeConfig,
                     std::vector<Tag>& tags);

        void SetEmptyCity();
        void SetEmptyStreet();

        void SetCity(FileOffset cityOffset,
                     bool cityIsNode);

        void SetStreet(FileOffset streetOffset);

        bool operator==(const NodeAttributes& other) const;
        bool operator!=(const NodeAttributes& other) const;
    };

  class OSMSCOUT_API Node : public Referencable
  {
  private:
    FileOffset        fileOffset;

    TypeId            type;       //! Type of Node
    GeoCoord          coords;     //! Coordinates of node
    NodeAttributes    attributes; //! Attributes of the nodes

  public:
    inline Node()
    : fileOffset(0),
      type(typeIgnore)
    {
      // no code
    }

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

  public:
    inline TypeId GetType() const
    {
      return type;
    }

    inline const GeoCoord& GetCoords() const
    {
      return coords;
    }

    inline double GetLat() const
    {
      return coords.GetLat();
    }

    inline double GetLon() const
    {
      return coords.GetLon();
    }

    inline const NodeAttributes& GetAttributes() const
    {
      return attributes;
    }

    inline std::string GetName() const
    {
      return attributes.GetName();
    }

    inline std::string GetLocation() const
    {
        // TODO remove this function
        // altogether
        std::string s;
        return s;
    }

    inline std::string GetAddress() const
    {
      return attributes.GetAddress();
    }

    inline bool HasTags() const
    {
      return !attributes.GetTags().empty();
    }

    void SetType(TypeId type);
    void SetCoords(const GeoCoord& coords);
    bool SetTags(Progress& progress,
                 const TypeConfig& typeConfig,
                 std::vector<Tag>& tags);
    void SetEmptyCity();
    void SetEmptyStreet();
    void SetCity(FileOffset cityOffset,
                 bool cityIsNode);
    void SetStreet(FileOffset streetOffset);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };

  typedef Ref<Node> NodeRef;
}

#endif
