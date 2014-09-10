#ifndef OSMSCOUT_UTIL_TRANSFORMER_H
#define OSMSCOUT_UTIL_TRANSFORMER_H

//#include <stdio.h>
#include "osmscout/util/Transformation.h"
#include "osmscout/util/Projection.h"

namespace osmscout {
  class TileProjection : MercatorProjection {

  };

  class Transformer {
  private:
    MercatorProjection projection;
    TransPolygon::OptimizeMethod optimize;
    Magnification magnification;
    CoordBufferImpl<Vertex2D> *coords;
    TransBuffer transbuffer;

  public:
    Transformer() : coords(new CoordBufferImpl<Vertex2D>()),
      transbuffer(coords)      {
      //projection = new TileProjection();
      optimize = TransPolygon::OptimizeMethod::none;
      //magnification = new Magnification();
    }

    ~Transformer(){
      //delete transbuffer;
      //delete coords;
      //delete projection;
    }

    void
    setProjection(double lat, double lon, double scale, size_t size){
      projection.Set(lon, lat, scale, size, size);

      //magnification.SetMagnification(scale);
      //projection.Set(lon, lat, magnification, size, size);
    }
    void
     setProjection(double minLat, double minLon, double maxLat, double maxLon, size_t size){
          projection.Set(minLon, minLat, maxLon, maxLat, size);

          //magnification.SetMagnification(scale);
          //projection.Set(lon, lat, magnification, size, size);
    }

    bool
    transformWay(const Way& way, size_t& start, size_t& end){
      return transbuffer.TransformWay(projection, optimize, way.nodes, start, end, 0);
    }

    bool
    transformArea(const Area::Ring& ring, size_t& start, size_t& end){
      transbuffer.TransformArea(projection, optimize, ring.nodes, start, end, 0);
      return true;
    }

    bool
    copyRange(size_t start, size_t end, float points[]){
      for (int i = 0, n = end - start + 1; i < n; i += 1) {
	const Vertex2D *c = &(coords->buffer[start + i]);
	//std::cout << c.GetX() << " " << c.GetY() << std::endl;
	points[(i * 2) + 0] = (float) c->GetX();
	points[(i * 2) + 1] = (float) c->GetY();
      }
      return true;
    }

    void reset(){
      transbuffer.Reset();
    }
  };
}

#endif
