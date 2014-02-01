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
    // Attribute availability flags (for optimized attribute storage)
    const static uint8_t hasNameAlt      = 1 << 3; //! We have an alternative name (mainly in a second language)
    const static uint8_t hasName         = 1 << 4; //! We have a name
    const static uint8_t hasLocation     = 1 << 5; //! Street and...
    const static uint8_t hasAddress      = 1 << 6; //! ...house number
    const static uint8_t hasTags         = 1 << 7; //! We have additional tags

  private:
    mutable uint8_t  flags;

    std::string      location; //! street and...
    std::string      address;  //! ...house number
    std::vector<Tag> tags;     //! list of preparsed tags

    TextId nameId;
    TextId nameAltId;

  private:
    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;

    friend class Node;

  public:
    inline NodeAttributes()
    : flags(0),
      nameId(0),
      nameAltId(0)
    {
      // no code
    }

    inline uint8_t GetFlags() const
    {
      return flags;
    }

    inline std::string GetName() const
    {
        std::string s;
        return s;
    }

    inline std::string GetNameAlt() const
    {
       std::string s;
       return s;
    }

    inline TextId GetNameId() const {
        return nameId;
    }

    inline TextId GetNameAltId() const {
        return nameAltId;
    }

    inline std::string GetLocation() const
    {
      return location;
    }

    inline std::string GetAddress() const
    {
      return address;
    }

    inline bool HasTags() const
    {
      return !tags.empty();
    }

    inline bool HasName() const {
        return (flags&hasName)!=0;
    }

    inline bool HasNameAlt() const {
        return (flags&hasNameAlt)!=0;
    }

    inline const std::vector<Tag>& GetTags() const
    {
      return tags;
    }

    bool SetTags(Progress& progress,
                 const TypeConfig& typeConfig,
                 std::vector<Tag>& tags);

    void SetNameId(TextId nameId);
    void SetNameAltId(TextId nameAltId);

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
      return attributes.GetLocation();
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

    void SetNameId(TextId nameId);
    void SetNameAltId(TextId nameAltId);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };

  typedef Ref<Node> NodeRef;
}

#endif
