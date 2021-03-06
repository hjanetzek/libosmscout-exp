#ifndef OSMSCOUT_MAPSERVICE_H
#define OSMSCOUT_MAPSERVICE_H

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

#include <list>
#include <vector>

// Type and style sheet configuration
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeSet.h>

#include <osmscout/Database.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
    Parameter to influence the search result for searching for (drawable)
    objects in a given area.
    */
  class OSMSCOUT_API AreaSearchParameter
  {
  private:
    unsigned long maxAreaLevel;
    unsigned long maxNodes;
    unsigned long maxWays;
    unsigned long maxAreas;
    bool          useLowZoomOptimization;
    BreakerRef    breaker;
    bool          useMultithreading;

  public:
    AreaSearchParameter();

    void SetMaximumAreaLevel(unsigned long maxAreaLevel);

    void SetMaximumNodes(unsigned long maxNodes);
    void SetMaximumWays(unsigned long maxWays);
    void SetMaximumAreas(unsigned long maxAreas);

    void SetUseLowZoomOptimization(bool useLowZoomOptimization);

    void SetUseMultithreading(bool useMultithreading);

    void SetBreaker(const BreakerRef& breaker);

    unsigned long GetMaximumAreaLevel() const;

    unsigned long GetMaximumNodes() const;
    unsigned long GetMaximumWays() const;
    unsigned long GetMaximumAreas() const;

    bool GetUseLowZoomOptimization() const;

    bool GetUseMultithreading() const;

    bool IsAborted() const;
  };

  /**
   * \ingroup Service
   * MapService offers services for retrieving data in a way that is
   * helpful for drawing maps.
   *
   * Currently the following functionalities are supported:
   * - Get objects of a certain type in a given area and impose certain
   * limits on the resulting data (size of area, number of objects,
   * low zoom optimizations,...).
   */
  class OSMSCOUT_API MapService : public Referencable
  {
  private:
    DatabaseRef database;

  private:
    bool GetObjectsNodes(const AreaSearchParameter& parameter,
                         const TypeSet &nodeTypes,
                         double lonMin, double latMin,
                         double lonMax, double latMax,
                         std::string& nodeIndexTime,
                         std::string& nodesTime,
                         std::vector<NodeRef>& nodes) const;

    bool GetObjectsWays(const AreaSearchParameter& parameter,
                        const std::vector<TypeSet>& wayTypes,
                        const Magnification& magnification,
                        double lonMin, double latMin,
                        double lonMax, double latMax,
                        std::string& wayOptimizedTime,
                        std::string& wayIndexTime,
                        std::string& waysTime,
                        std::vector<WayRef>& ways) const;

    bool GetObjectsAreas(const AreaSearchParameter& parameter,
                               const TypeSet& areaTypes,
                               const Magnification& magnification,
                               double lonMin, double latMin,
                               double lonMax, double latMax,
                               std::string& areaOptimizedTime,
                               std::string& areaIndexTime,
                               std::string& areasTime,
                               std::vector<AreaRef>& areas) const;

  public:
    MapService(const DatabaseRef& database);
    virtual ~MapService();

    bool GetObjects(const TypeSet &nodeTypes,
                    const std::vector<TypeSet>& wayTypes,
                    const TypeSet& areaTypes,
                    double lonMin, double latMin,
                    double lonMax, double latMax,
                    const Magnification& magnification,
                    const AreaSearchParameter& parameter,
                    std::vector<NodeRef>& nodes,
                    std::vector<WayRef>& ways,
                    std::vector<AreaRef>& areas) const;

    bool GetObjects(const AreaSearchParameter& parameter,
                    const Magnification& magnification,
                    const TypeSet &nodeTypes,
                    double nodeLonMin, double nodeLatMin,
                    double nodeLonMax, double nodeLatMax,
                    std::vector<NodeRef>& nodes,
                    const std::vector<TypeSet>& wayTypes,
                    double wayLonMin, double wayLatMin,
                    double wayLonMax, double wayLatMax,
                    std::vector<WayRef>& ways,
                    const TypeSet& areaTypes,
                    double areaLonMin, double areaLatMin,
                    double areaLonMax, double areaLatMax,
                    std::vector<AreaRef>& areas) const;

    bool GetGroundTiles(double lonMin, double latMin,
                        double lonMax, double latMax,
                        const Magnification& magnification,
                        std::list<GroundTile>& tiles) const;
  };

  //! \ingroup Service
  //! Reference counted reference to an Database instance
  typedef Ref<MapService> MapServiceRef;
}

#endif
