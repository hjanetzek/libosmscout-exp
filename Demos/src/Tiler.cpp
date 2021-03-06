/*
  Tiles - a demo program for libosmscout
  Copyright (C) 2011  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iostream>
#include <iomanip>
#include <limits>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/MapPainterAgg.h>

#include <osmscout/util/StopClock.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory), drawing the "Ruhrgebiet":

  src/Tiler ../TravelJinni/ ../TravelJinni/standard.oss 51.2 6.5 51.7 8 10 13
*/

static unsigned long tileWidth=256;
static unsigned long tileHeight=256;

// See http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames for details about
// coordinate transformation

size_t long2tilex(double lon, double z)
{
  return (size_t)(floor((lon + 180.0) / 360.0 *pow(2.0,z)));
}

size_t lat2tiley(double lat, double z)
{
  return (size_t)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0,z)));
}

double tilex2long(int x, double z)
{
  return x / pow(2.0,z) * 360.0 - 180;
}

double tiley2lat(int y, double z)
{
  double n = M_PI - 2.0 * M_PI * y / pow(2.0,z);

  return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

bool write_ppm(const agg::rendering_buffer& buffer,
               const char* file_name)
{
  FILE* fd=fopen(file_name, "wb");

  if (fd) {
    fprintf(fd,"P6 %d %d 255\n", buffer.width(),buffer.height());

    for (size_t y=0; y<buffer.height();y++)
    {
      const unsigned char* row=buffer.row_ptr(y);

      fwrite(row,1,buffer.width()*3,fd);
    }

    fclose(fd);
    return true;
  }

  return false;
}

int main(int argc, char* argv[])
{
  std::string   map;
  std::string   style;
  double        latTop,latBottom,lonLeft,lonRight;
  unsigned long xTileStart,xTileEnd,xTileCount,yTileStart,yTileEnd,yTileCount;
  unsigned long startZoom;
  unsigned long endZoom;

  if (argc!=9) {
    std::cerr << "DrawMap ";
    std::cerr << "<map directory> <style-file> ";
    std::cerr << "<lat_top> <lon_left> <lat_bottom> <lon_right> ";
    std::cerr << "<start_zoom>" << std::endl;
    std::cerr << "<end_zoom>" << std::endl;
    return 1;
  }

  map=argv[1];
  style=argv[2];

  if (sscanf(argv[3],"%lf",&latTop)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[4],"%lf",&lonLeft)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[5],"%lf",&latBottom)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[6],"%lf",&lonRight)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[7],"%lu",&startZoom)!=1) {
    std::cerr << "start zoom is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[8],"%lu",&endZoom)!=1) {
    std::cerr << "end zoom is not numeric!" << std::endl;
    return 1;
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database(new osmscout::Database(databaseParameter));
  osmscout::MapServiceRef     mapService(new osmscout::MapService(database));

  if (!database->Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::StyleConfigRef styleConfig(new osmscout::StyleConfig(database->GetTypeConfig()));

  if (!styleConfig->Load(style)) {
    std::cerr << "Cannot open style" << std::endl;
  }

  osmscout::MercatorProjection  projection;
  osmscout::MapParameter        drawParameter;
  osmscout::AreaSearchParameter searchParameter;
  osmscout::MapData             data;

  // Change this, to match your system
  drawParameter.SetFontName("/usr/share/fonts/truetype/msttcorefonts/Verdana.ttf");
  drawParameter.SetFontName("/usr/share/fonts/TTF/DejaVuSans.ttf");
  drawParameter.SetFontSize(6.0);
  // Fadings make problems with tile approach, we disable it
  drawParameter.SetDrawFadings(false);
  // To get accurate label drawing at tile borders, we take into account labels
  // of other than the current tile, too.
  drawParameter.SetDropNotVisiblePointLabels(false);

  searchParameter.SetUseLowZoomOptimization(false);
  searchParameter.SetMaximumAreaLevel(3);
  searchParameter.SetMaximumNodes(std::numeric_limits<unsigned long>::max());
  searchParameter.SetMaximumWays(std::numeric_limits<unsigned long>::max());
  searchParameter.SetMaximumAreas(std::numeric_limits<unsigned long>::max());

  osmscout::MapPainterAgg painter(styleConfig);
  osmscout::Magnification magnification;

  for (size_t zoom=std::min(startZoom,endZoom);
       zoom<=std::max(startZoom,endZoom);
       zoom++) {
    xTileStart=long2tilex(std::min(lonLeft,lonRight),zoom);
    xTileEnd=long2tilex(std::max(lonLeft,lonRight),zoom);
    xTileCount=xTileEnd-xTileStart+1;

    yTileStart=lat2tiley(std::max(latTop,latBottom),zoom);
    yTileEnd=lat2tiley(std::min(latTop,latBottom),zoom);
    yTileCount=yTileEnd-yTileStart+1;

    std::cout << "Drawing zoom " << zoom << ", " << (xTileCount)*(yTileCount) << " tiles [" << xTileStart << "," << yTileStart << " - " <<  xTileEnd << "," << yTileEnd << "]" << std::endl;

    unsigned long bitmapSize=tileWidth*tileHeight*3*xTileCount*yTileCount;
    unsigned char *buffer=new unsigned char[bitmapSize];

    magnification.SetLevel(zoom);

    memset(buffer,0,bitmapSize);

    agg::rendering_buffer rbuf(buffer,
                               tileWidth*xTileCount,
                               tileHeight*yTileCount,
                               tileWidth*xTileCount*3);

    double minTime=std::numeric_limits<double>::max();
    double maxTime=0.0;
    double totalTime=0.0;

    osmscout::TypeSet              nodeTypes;
    std::vector<osmscout::TypeSet> wayTypes;
    osmscout::TypeSet              areaTypes;

    styleConfig->GetNodeTypesWithMaxMag(magnification,
                                        nodeTypes);

    styleConfig->GetWayTypesByPrioWithMaxMag(magnification,
                                             wayTypes);

    styleConfig->GetAreaTypesWithMaxMag(magnification,
                                        areaTypes);

    for (size_t y=yTileStart; y<=yTileEnd; y++) {
      for (size_t x=xTileStart; x<=xTileEnd; x++) {
        double              minLat2,minLat,lat,maxLat,maxLat2;
        double              minLon2,minLon,lon,maxLon,maxLon2;
        agg::pixfmt_rgb24   pf(rbuf);
        osmscout::StopClock timer;

        minLat2=tiley2lat(y+2,zoom);
        minLat=tiley2lat(y+1,zoom);
        maxLat=tiley2lat(y,zoom);
        maxLat2=tiley2lat(y-1,zoom);

        minLon2=tilex2long(x-1,zoom);
        minLon=tilex2long(x,zoom);
        maxLon=tilex2long(x+1,zoom);
        maxLon2=tilex2long(x+2,zoom);

        lat=(minLat+maxLat)/2;
        lon=(minLon+maxLon)/2;

        std::cout << "Drawing tile [" << minLat << "," << lat << "," << maxLat << "]x[" << minLon << "," << lon << "," << maxLon << "]" << std::endl;
        //std::cout << x << "," << y << "/";
        //std::cout << x-xTileStart << "," << y-yTileStart << std::endl;

        projection.Set(lon,lat,
                       magnification,
                       tileWidth,
                       tileHeight);


        mapService->GetObjects(searchParameter,
                               projection.GetMagnification(),
                               nodeTypes,
                               minLon2,
                               minLat2,
                               maxLon2,
                               maxLat2,
                               data.nodes,
                               wayTypes,
                               minLon,
                               minLat,
                               maxLon,
                               maxLat,
                               data.ways,
                               areaTypes,
                               minLon2,
                               minLat2,
                               maxLon2,
                               maxLat2,
                               data.areas);

        size_t bufferOffset=xTileCount*tileWidth*3*(y-yTileStart)*tileHeight+
                            (x-xTileStart)*tileWidth*3;

        rbuf.attach(buffer+bufferOffset,
                    tileWidth,tileHeight,
                    tileWidth*xTileCount*3);

        painter.DrawMap(projection,
                        drawParameter,
                        data,
                        &pf);

        timer.Stop();

        double time=timer.GetMilliseconds();

        minTime=std::min(minTime,time);
        maxTime=std::max(maxTime,time);
        totalTime+=time;

        std::string output=osmscout::NumberToString(zoom)+"_"+osmscout::NumberToString(x)+"_"+osmscout::NumberToString(y)+".ppm";

        write_ppm(rbuf,output.c_str());
      }
    }

    rbuf.attach(buffer,
                tileWidth*xTileCount,
                tileHeight*yTileCount,
                tileWidth*xTileCount*3);

    std::string output=osmscout::NumberToString(zoom)+"_full_map.ppm";

    write_ppm(rbuf,output.c_str());

    delete[] buffer;

    std::cout << "=> Time: ";
    std::cout << "total: " << totalTime << " msec ";
    std::cout << "min: " << minTime << " msec ";
    std::cout << "avg: " << totalTime/(xTileCount*yTileCount) << " msec ";
    std::cout << "max: " << maxTime << " msec" << std::endl;
  }

  database->Close();

  return 0;
}
