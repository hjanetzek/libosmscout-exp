#ifndef OSMSCOUT_UTIL_QUADINDEX_H
#define OSMSCOUT_UTIL_QUADINDEX_H

#include <osmscout/system/SSEMathPublic.h>
#include <osmscout/Way.h>
#include <osmscout/Area.h>
#include <math.h>

namespace osmscout {
  static const double gradtorad=2*M_PI/360;
  /**
   * \ingroup Index
   */
  class OSMSCOUT_API QuadIndex
  {
  public:
    static const uint32_t MAX_LEVEL=20;

    static inline void
    CellId(double longitude, double latitude,
           uint32_t level,
           uint32_t &xcell, uint32_t &ycell){

#ifdef USE_MERCATOR_INDEX
      latitude = atanh(sin(latitude * gradtorad)) / (gradtorad * 2);
#endif
      double cellWidth=360.0/(1<<level);
      double cellHeight=180.0/(1<<level);
      xcell=(uint32_t) floor((longitude+180.0)/cellWidth);
      ycell=(uint32_t) floor((latitude+90.0)/cellHeight);
    }

    static inline void
    CellId(double longitude, double latitude,
           double cellWidth, double cellHeight,
           uint32_t &xcell, uint32_t &ycell){

#ifdef USE_MERCATOR_INDEX
      latitude = atanh(sin(latitude * gradtorad)) / (gradtorad * 2);
#endif
      xcell=(uint32_t) floor((longitude+180.0)/cellWidth);
      ycell=(uint32_t) floor((latitude+90.0)/cellHeight);
    }

    static inline void
    CellIds(double minLon, double maxLon,
            double minLat,
            double maxLat,
            uint32_t level,
            uint32_t &minxc, uint32_t &maxxc,
            uint32_t &minyc,
            uint32_t &maxyc){

#ifdef USE_MERCATOR_INDEX
      minLat = atanh(sin(minLat * gradtorad)) / (gradtorad * 2);
      maxLat = atanh(sin(maxLat * gradtorad)) / (gradtorad * 2);
#endif
      double cellWidth=360.0/(1<<level);
      double cellHeight=180.0/(1<<level);
      minxc=(uint32_t) floor((minLon+180.0)/cellWidth);
      maxxc=(uint32_t) floor((maxLon+180.0)/cellWidth);
      minyc=(uint32_t) floor((minLat+90.0)/cellHeight);
      maxyc=(uint32_t) floor((maxLat+90.0)/cellHeight);
    }

    /**
     * Calculate minimum and maximum tile ids that are covered
     * by the way.
     */
    static inline void
    WayCellIds(const Way *way, uint32_t level,
               uint32_t &minxc,
               uint32_t &maxxc,
               uint32_t &minyc,
               uint32_t &maxyc){
      double minLon;
      double maxLon;
      double minLat;
      double maxLat;
      way->GetBoundingBox(minLon,maxLon,minLat,maxLat);
#ifdef USE_MERCATOR_INDEX
      minLat = atanh(sin(minLat * gradtorad)) / (gradtorad * 2);
      maxLat = atanh(sin(maxLat * gradtorad)) / (gradtorad * 2);
#endif
      double cellWidth=360.0/(1<<level);
      double cellHeight=180.0/(1<<level);
      minxc=(uint32_t) floor((minLon+180.0)/cellWidth);
      maxxc=(uint32_t) floor((maxLon+180.0)/cellWidth);
      minyc=(uint32_t) floor((minLat+90.0)/cellHeight);
      maxyc=(uint32_t) floor((maxLat+90.0)/cellHeight);
    }

    /**
     * Calculate minimum and maximum tile ids that are covered
     * by the area.
     */
    static inline void
    AreaCellIds(const Area *area, uint32_t level,
                uint32_t &minxc,
                uint32_t &maxxc,
                uint32_t &minyc,
                uint32_t &maxyc){
      double minLon;
      double maxLon;
      double minLat;
      double maxLat;
      area->GetBoundingBox(minLon,maxLon,minLat,maxLat);
#ifdef USE_MERCATOR_INDEX
      minLat = atanh(sin(minLat*gradtorad))/(gradtorad*2);
      maxLat = atanh(sin(maxLat*gradtorad))/(gradtorad*2);
#endif
      double cellWidth=(1<<level)/360.0;
      double cellHeight=(1<<level)/180.0;
      minxc=(uint32_t) floor((minLon+180.0)*cellWidth);
      maxxc=(uint32_t) floor((maxLon+180.0)*cellWidth);
      minyc=(uint32_t) floor((minLat+90.0)*cellHeight);
      maxyc=(uint32_t) floor((maxLat+90.0)*cellHeight);
    }
  };
}

#endif
