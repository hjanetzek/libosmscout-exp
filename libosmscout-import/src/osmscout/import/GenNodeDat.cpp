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

    // Open up text data files to lookup text ids
    marisa::Trie trie_poi;
    marisa::Trie trie_loc;
    marisa::Trie trie_region;
    marisa::Trie trie_other;

    if(!loadTextDataTries(parameter,
                          progress,
                          trie_poi,
                          trie_loc,
                          trie_region,
                          trie_other)) {
        return false;
    }

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

        // Save string attributes associated with the node. We
        // need to remove name strings before calling SetTags().
        std::string name;
        std::string name_alt;
        cutNameDataFromTags(typeConfig,tags,name,name_alt);

        // SetTags must be called before setting
        // NameId or NameAltId
        node.SetTags(progress,
                     typeConfig,
                     tags);

        // Save TextIds to node
        if(!saveNodeTextIds(progress,
                            typeConfig,
                            name,
                            name_alt,
                            node,
                            trie_poi,
                            trie_loc,
                            trie_region,
                            trie_other)) {
            return false;
        }

        FileOffset fileOffset;

        if (!writer.GetPos(fileOffset)) {
          progress.Error(std::string("Error while reading current fileOffset in file '")+
                         writer.GetFilename()+"'");
          return false;
        }

        // write node data to file
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

  bool NodeDataGenerator::loadTextDataTries(ImportParameter const &parameter,
                                            Progress &progress,
                                            marisa::Trie &trie_poi,
                                            marisa::Trie &trie_loc,
                                            marisa::Trie &trie_region,
                                            marisa::Trie &trie_other)
  {
      std::vector<std::string> list_trie_files;
      list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                "poitext.dat"));

      list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                "loctext.dat"));

      list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                "regiontext.dat"));

      list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                "othertext.dat"));

      std::vector<marisa::Trie*> list_tries;
      list_tries.push_back(&trie_poi);
      list_tries.push_back(&trie_loc);
      list_tries.push_back(&trie_region);
      list_tries.push_back(&trie_other);

      for(size_t i=0; i < list_tries.size(); i++) {
          try {
              list_tries[i]->load(list_trie_files[i].c_str());
          }
          catch(marisa::Exception const &ex) {
              std::string err_msg=
                      "Error opening "+list_trie_files[i]+
                      ","+std::string(ex.what());
              progress.Error(err_msg);
              return false;
          }
      }
      return true;
  }

  bool NodeDataGenerator::saveNodeTextIds(Progress &progress,
                                          TypeConfig const &typeConfig,
                                          std::string const &name,
                                          std::string const &name_alt,
                                          Node &node,
                                          marisa::Trie &trie_poi,
                                          marisa::Trie &trie_loc,
                                          marisa::Trie &trie_region,
                                          marisa::Trie &trie_other)
  {
      marisa::Trie * trie;
      TypeInfo typeInfo=typeConfig.GetTypeInfo(node.GetType());

      // We use 32-bit integers to save each key id,
      // but set the first two MSB to denote which trie
      // the key belongs to (poi,loc,region,other)

      // The TextDataGenerator class already ensures
      // that no trie has a keycount > (2^30 -1), ie
      // the 2 MSB are 0

      uint32_t setbits=0;

      if(typeInfo.GetIndexAsPOI()) {
          trie = &trie_poi;
          setbits |= (1 << ((sizeof(uint32_t)*8)-2));
      }
      else if(typeInfo.GetIndexAsLocation()) {
          trie = &trie_loc;
          setbits |= (1 << ((sizeof(uint32_t)*8)-1));
      }
      else if(typeInfo.GetIndexAsRegion()) {
          trie = &trie_region;
          setbits |= (1 << ((sizeof(uint32_t)*8)-1));
          setbits |= (1 << ((sizeof(uint32_t)*8)-2));
      }
      else {
          trie = &trie_other;
      }

      if(!name.empty()) {
          // save the dict key id for this name text
          marisa::Agent agent;
          agent.set_query(name.c_str());

          // TODO Note:
          // In the libmarisa documentation its implied that
          // 'Trie::lookup()' doesn't restore the key id but
          // it does seem to (maybe it only applies to the key
          // text?, need to verify)
          if(trie->lookup(agent)) {
              TextId textid=static_cast<TextId>(agent.key().id());
              textid |= setbits;
              node.SetNameId(textid);
          }
          else {
              std::string err_msg = "Error looking up name text: "+name;
              progress.Error(err_msg);
              return false;
          }
      }
      if(!name_alt.empty()) {
          // save the dict key id for this name alt text
          marisa::Agent agent;
          agent.set_query(name_alt.c_str());

          if(trie->lookup(agent)) {
              TextId textid=static_cast<TextId>(agent.key().id());
              textid |= setbits;
              node.SetNameAltId(textid);
          }
          else {
              std::string err_msg = "Error looking up name_alt text: "+name_alt;
              progress.Error(err_msg);
              return false;
          }
      }

      return true;
  }

  void NodeDataGenerator::cutNameDataFromTags(TypeConfig const &typeConfig,
                                              std::vector<Tag> &tags,
                                              std::string &name,
                                              std::string &name_alt)
  {
      uint32_t name_prio=0;
      uint32_t name_alt_prio=0;
      std::vector<Tag>::iterator tag_it = tags.begin();
      while(tag_it != tags.end()) {
          uint32_t tag_name_prio,tag_name_alt_prio;
          bool isNameTag = typeConfig.IsNameTag(tag_it->key,tag_name_prio);
          bool isNameAltTag = typeConfig.IsNameAltTag(tag_it->key,tag_name_alt_prio);

          if(isNameTag && (name.empty() || tag_name_prio > name_prio)) {
              name = tag_it->value;
              name_prio = tag_name_prio;
          }

          if(isNameAltTag && (name.empty() || tag_name_alt_prio > name_alt_prio)) {
              name_alt = tag_it->value;
              name_alt_prio = tag_name_alt_prio;
          }

          if(isNameTag || isNameAltTag) {
              tag_it = tags.erase(tag_it);
          }

          else {
              ++tag_it;
          }
      }
  }



}
