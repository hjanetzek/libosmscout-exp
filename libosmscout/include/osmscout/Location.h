#ifndef OSMSCOUT_LOCATION_H
#define OSMSCOUT_LOCATION_H

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

#include <osmscout/ObjectRef.h>

#include <osmscout/util/Reference.h>

namespace osmscout {
  /**
   * \defgroup Location Location related data structures and services
   *
   * Classes and methods for handling location aspects of object
   * in the libosmscout database.
   */

  /**
   \ingroup Location
   A named administrative region. It is used to build up hierarchical,
   structured containment information like "Streets in City". Most of
   the time an administrative region is just the area of a city, but
   depending on the data quality it may also be parts of the city
   or bigger administrative regions like states.

   AdminRegions are currently returned by
   Database.GetMatchingAdminRegions() using the CityStreetIndex.
   */
  class OSMSCOUT_API AdminRegion : public Referencable
  {
  public:
    class OSMSCOUT_API RegionAlias
    {
    public:
      std::string name;         //<! Alias
      FileOffset  objectOffset; //<! Node data offset of the alias
    };

    FileOffset               regionOffset;       //<! Offset of this entry in the index
    FileOffset               dataOffset;         //<! Offset of the data part of this entry
    FileOffset               parentRegionOffset; //<! Offset of the parent region index entry
    std::string              name;               //<! name of the region
    ObjectFileRef            object;             //<! The object that represents this region
    std::string              aliasName;          //<! Additional optional alias name
    ObjectFileRef            aliasObject;        //<! Additional optional alias reference
    std::vector<RegionAlias> aliases;            //<! The list of alias for this region

  public:
    bool Match(const ObjectFileRef& object) const;
  };

  typedef Ref<AdminRegion> AdminRegionRef;

  /**
   * \ingroup Location
   * Visitor that gets called for every region found.
   * It is the task of the visitor to decide if a region matches the given criteria.
   */
  class OSMSCOUT_API AdminRegionVisitor
  {
  public:
    enum Action {
      //! Do not visit child regions, but continue with traversal
      skipChildren,
      //! Visit child regions
      visitChildren,
      //! Stop
      stop,
      //! Signal an error
      error
    };

  public:
    virtual ~AdminRegionVisitor();

    virtual Action Visit(const AdminRegion& region) = 0;
  };

  /**
   * \ingroup Location
   * A POI is an object within an area, which has been indexed by
   * its name.
   */
  class OSMSCOUT_API POI : public Referencable
  {
  public:
    FileOffset    regionOffset; //!< Offset of the region this location is in
    std::string   name;         //!< name of the POI
    ObjectFileRef object;       //!< Reference to the object
  };

  typedef Ref<POI> POIRef;

  /**
    \ingroup Location
    A location is a named point, way, area or relation on the map.
    Something you can search for. Locations are currently returned
    by Database.GetMatchingLocations() which uses CityStreetIndex
    internally.
   */
  class OSMSCOUT_API Location : public Referencable
  {
  public:
    FileOffset                 locationOffset;  //!< Offset to location
    FileOffset                 regionOffset;    //!< Offset of the admin region this location is in
    FileOffset                 addressesOffset; //!< Offset to the list of addresses
    std::string                name;            //!< name of the location
    std::vector<ObjectFileRef> objects;         //!< List of objects that build up this location
  };

  typedef Ref<Location> LocationRef;

  /**
   * \ingroup Location
   * Visitor that gets called for every location found in the given area.
   * It is the task of the visitor to decide if a locations matches the given criteria.
   */
  class OSMSCOUT_API LocationVisitor
  {
  public:
    virtual ~LocationVisitor();

    virtual bool Visit(const AdminRegion& adminRegion,
                       const POI &poi) = 0;
    virtual bool Visit(const AdminRegion& adminRegion,
                       const Location &location) = 0;
  };

  /**
    \ingroup Location
    An address is a unique place at a given location, normally a building that
    is addressed by its house number.
   */
  class OSMSCOUT_API Address : public Referencable
  {
  public:
    FileOffset    addressOffset;  //!< Offset of the address entry
    FileOffset    locationOffset; //!< Offset to location
    FileOffset    regionOffset;   //!< Offset of the admin region this location is in
    std::string   name;           //!< name of the address
    ObjectFileRef object;         //!< Object that represents the address
  };

  typedef Ref<Address> AddressRef;

  /**
   * \ingroup Location
   * Visitor that gets called for every address found at a given location.
   * It is the task of the visitor to decide if a address matches the given criteria.
   */
  class OSMSCOUT_API AddressVisitor
  {
  public:
    virtual ~AddressVisitor();

    virtual bool Visit(const AdminRegion& adminRegion,
                       const Location& location,
                       const Address& address) = 0;
  };

  /**
   * \ingroup Location
   */
  class OSMSCOUT_API AddressListVisitor : public AddressVisitor
  {
  public:
    class OSMSCOUT_API AddressResult
    {
    public:
      AdminRegionRef adminRegion; //!< The admin region the address is contained by
      LocationRef    location;    //!< The location the address belongs to
      AddressRef     address;     //!< The address itself
    };

  private:
    size_t                   limit;

  public:
    std::list<AddressResult> results;
    bool                     limitReached;

  public:
    AddressListVisitor(size_t limit);

    bool Visit(const AdminRegion& adminRegion,
               const Location& location,
               const Address& address);
  };
}

#endif
