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

#include <osmscout/import/GenNodeDat.h>

#include <iostream>
#include <map>

#include <osmscout/Node.h>

#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/String.h>

#include <osmscout/import/RawNode.h>

namespace osmscout {

  std::string NodeDataGenerator::GetDescription() const
  {
    return "Generate 'nodes.tmp'";
  }

  bool NodeDataGenerator::Import(const ImportParameter& parameter,
                                 Progress& progress,
                                 const TypeConfig& typeConfig)
  {
    double   minLon=-10.0;
    double   minLat=-10.0;
    double   maxLon=10.0;
    double   maxLat=10.0;
    uint32_t rawNodeCount=0;
    uint32_t nodesReadCount=0;
    uint32_t nodesWrittenCount=0;

    //
    // Iterator over all raw nodes, hcekc they type, and convert them from raw nodes
    // to nodes if the type is interesting (!=typeIgnore).
    //
    // Count the bounding box by the way...
    //

    progress.SetAction("Generating nodes.tmp");

    FileScanner scanner;
    FileWriter  writer;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawnodes.dat"),
                      FileScanner::Sequential,
                      parameter.GetRawNodeDataMemoryMaped())) {
      progress.Error("Cannot open 'rawnodes.dat'");
      return false;
    }

    if (!scanner.Read(rawNodeCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "nodes.tmp"))) {
      progress.Error("Cannot create 'nodes.tmp'");
      return false;
    }

    writer.Write(nodesWrittenCount);

//    // Create a data file and index for the set
//    // of unique strings in each node saved
//    FileOffset nsOffset;
//    OSMSCOUT_HASHSET<std::string> setUnqNodeStrings;

//    FileScanner nsWriter;
//    if(!nsWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
//                                      "nodestrings.dat")))   {
//        progress.Error("Cannot create 'nodestrings.dat'");
//        return false;
//    }


    for (uint32_t n=1; n<=rawNodeCount; n++) {
      progress.SetProgress(n,rawNodeCount);

      RawNode rawNode;
      Node    node;

      if (!rawNode.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(rawNodeCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (nodesReadCount==0) {
        minLat=rawNode.GetLat();
        minLon=rawNode.GetLon();
        maxLat=rawNode.GetLat();
        maxLon=rawNode.GetLon();
      }
      else {
        minLat=std::min(minLat,rawNode.GetLat());
        minLon=std::min(minLon,rawNode.GetLon());
        maxLat=std::max(maxLat,rawNode.GetLat());
        maxLon=std::max(maxLon,rawNode.GetLon());
      }

      nodesReadCount++;

      if (rawNode.GetType()!=typeIgnore &&
          !typeConfig.GetTypeInfo(rawNode.GetType()).GetIgnore()) {
        std::vector<Tag> tags(rawNode.GetTags());

        node.SetType(rawNode.GetType());
        node.SetCoords(rawNode.GetCoords());
        node.SetTags(progress,
                     typeConfig,
                     tags);

//        // Save string attributes associated with the node
//        uint32_t name_prio=0;
//        uint32_t name_alt_prio=0;

//        std::string name;
//        std::string name_alt;
//        std::string street;
//        std::string address;

//        std::vector<Tag>::iterator tag_it = tags.begin();
//        while(tag_it != tags.end()) {
//            uint32_t tag_name_prio,tag_name_alt_prio;
//            bool isNameTag = typeConfig.IsNameTag(tag_it->key,tag_name_prio);
//            bool isNameAltTag = typeConfig.IsNameAltTag(tag_it->key,tag_name_alt_prio);

//            if(isNameTag && (name.empty() || tag_name_prio > name_prio)) {
//                name = tag_it->value;
//                name_prio = tag_name_prio;
//            }

//            if(isNameAltTag && (name.empty() || tag_name_alt_prio > name_alt_prio)) {
//                name_alt = tag_it->value;
//                name_alt_prio = tag_name_alt_prio;
//            }

//            if(isNameTag || isNameAltTag) {
//                tag_it = tags.erase(tag_it);
//            }
//            else if(tag_it->key == typeConfig.tagStreet) {
//                street = tag_it->value;
//                tag_it = tags.erase(tag_it);
//            }
//            else if(tag_it->key == typeConfig.tagHouseNr) {
//                address = tag_it->value;
//                tag_it = tags.erase(tag_it);
//            }
//            else {
//                ++tag_it;
//            }
//        }

        if(!node.GetAttributes().GetName().empty())   {
            // For the initial import and first pass
            // of the data, we specify a negative file
            // offset id for city/street attributes to
            // write some placeholder bytes to file
            if(typeConfig.GetTypeInfo(rawNode.GetType()).GetFindCity()) {
                node.SetEmptyCity();
            }
            if(typeConfig.GetTypeInfo(rawNode.GetType()).GetFindStreet()) {
                node.SetEmptyStreet();
            }
        }

        FileOffset fileOffset;

        if (!writer.GetPos(fileOffset)) {
          progress.Error(std::string("Error while reading current fileOffset in file '")+
                         writer.GetFilename()+"'");
          return false;
        }

        writer.Write(rawNode.GetId());
        if(!node.Write(writer)) {
            progress.Error(std::string("Error trying to write node:")+
                           NumberToString(rawNode.GetId()));
        }

        nodesWrittenCount++;
      }
    }

    if (!scanner.Close()) {
      return false;
    }

    writer.SetPos(0);
    writer.Write(nodesWrittenCount);

    if (!writer.Close()) {
      return false;
    }

    progress.Info(std::string("Read "+NumberToString(nodesReadCount)+" nodes, wrote "+NumberToString(nodesWrittenCount)+" nodes"));

    progress.SetAction("Generating bounding.dat");

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "bounding.dat"))) {
      progress.Error("Cannot create 'bounding.dat'");
      return false;
    }

    // TODO: Dump bounding box to debug

    uint32_t minLatDat=(uint32_t)floor((minLat+90.0)*conversionFactor+0.5);
    uint32_t minLonDat=(uint32_t)floor((minLon+180.0)*conversionFactor+0.5);
    uint32_t maxLatDat=(uint32_t)floor((maxLat+90.0)*conversionFactor+0.5);
    uint32_t maxLonDat=(uint32_t)floor((maxLon+180.0)*conversionFactor+0.5);

    writer.WriteNumber(minLatDat);
    writer.WriteNumber(minLonDat);
    writer.WriteNumber(maxLatDat);
    writer.WriteNumber(maxLonDat);

    writer.Close();

    return true;
  }
}

