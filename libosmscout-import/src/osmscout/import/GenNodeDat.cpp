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

  bool NodeDataGenerator::genNodeTextDict(ImportParameter const &parameter,
                                          Progress &progress,
                                          TypeConfig const &typeConfig,
                                          marisa::Trie &trie)
  {
      progress.SetAction("Generating nodetext.dict");

      std::string file_rawnodes=
              AppendFileToDir(parameter.GetDestinationDirectory(),
                              "rawnodes.dat");

      FileScanner scanner;
      if(!scanner.Open(file_rawnodes,
                       FileScanner::Sequential,
                       parameter.GetRawNodeDataMemoryMaped())) {
          progress.Error("Cannot open 'rawnodes.dat'");
          return false;
      }

      uint32_t rawNodeCount=0;
      if(!scanner.Read(rawNodeCount)) {
          progress.Error("Error reading node count in 'rawnodes.dat'");
          return false;
      }

      // Build a dictionary for node text data
      marisa::Keyset keyset;

      for(uint32_t n=1; n <= rawNodeCount; n++) {
          RawNode rawNode;
          if (!rawNode.Read(scanner)) {
            progress.Error(std::string("Error while reading data entry ")+
                           NumberToString(n)+" of "+
                           NumberToString(rawNodeCount)+
                           " in file '"+
                           scanner.GetFilename()+"'");
            return false;
          }

          if (rawNode.GetType()!=typeIgnore &&
              !typeConfig.GetTypeInfo(rawNode.GetType()).GetIgnore()) {

              // Use the tags for this raw node to see if
              // we want to save any string attributes
              std::vector<Tag> tags(rawNode.GetTags());
              std::string name;
              std::string name_alt;
              cutNameDataFromTags(typeConfig,tags,name,name_alt);

              // save to keyset
              if(!name.empty()) {
                  keyset.push_back(name.c_str());
              }
              if(!name_alt.empty()) {
                  keyset.push_back(name_alt.c_str());
              }
          }
      }
      if(!scanner.Close()) {
          progress.Error("Failed to close rawnodes.dat file");
      }

      // build trie from keyset
      try {
          trie.build(keyset,
                     MARISA_DEFAULT_NUM_TRIES |
                     MARISA_DEFAULT_TAIL |
                     MARISA_LABEL_ORDER |
                     MARISA_DEFAULT_CACHE);
      }
      catch(marisa::Exception const &ex) {
          std::string err_msg="Error building node text data dict:";
          err_msg.append(ex.what());
          progress.Error(err_msg);
          return false;
      }

      // write trie to file
      std::string file_trie=
              AppendFileToDir(parameter.GetDestinationDirectory(),
                              "nodetext.dict");
      try {
          trie.save(file_trie.c_str());
      }
      catch(marisa::Exception const &ex) {
          std::string err_msg="Error saving node text data dict: ";
          err_msg.append(ex.what());
          progress.Error(err_msg);
          return false;
      }

      return true;
  }

  bool NodeDataGenerator::Import(const ImportParameter& parameter,
                                 Progress& progress,
                                 const TypeConfig& typeConfig)
  {
      // Build a dictionary for node text data
      marisa::Trie trie;
      genNodeTextDict(parameter,progress,typeConfig,trie);


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

//    // Create an index for the node text dictionary
//    // that lets us lookup nodes based on dict key ids
//    OSMSCOUT_HASHMAP<TextId,std::vector<FileOffset> > table_dictkey_offsets;
//    OSMSCOUT_HASHMAP<TextId,std::vector<FileOffset> >::iterator table_it;

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

        // Save string attributes associated with the node
        std::string name;
        std::string name_alt;
        cutNameDataFromTags(typeConfig,tags,name,name_alt);

        TextId dict_name_id,dict_name_alt_id;

        if(!name.empty()) {
            // save the dict key id for this name text
            marisa::Agent agent;
            agent.set_query(name.c_str());

            // TODO Note:
            // In the libmarisa documentation its implied that
            // 'Trie::lookup()' doesn't restore the key id but
            // it does seem to (maybe it only applies to the key
            // text?, need to verify)
            if(trie.lookup(agent)) {
                dict_name_id=static_cast<TextId>(agent.key().id());
                node.SetNameId(dict_name_id);
                //progress.Info("DEBUG:"+NumberToString(dict_name_id));
            }
            else {
                std::string err_msg =
                        "Fatal: Could not lookup node text in dict: "+
                        NumberToString(rawNode.GetId())+": "+name;
                progress.Error(err_msg);
                return false;
            }
        }
        if(!name_alt.empty()) {
            // save the dict key id for this name alt text
            marisa::Agent agent;
            agent.set_query(name_alt.c_str());

            if(trie.lookup(agent)) {
                dict_name_alt_id=static_cast<TextId>(agent.key().id());
                node.SetNameAltId(dict_name_alt_id);
                //progress.Info("DEBUG:"+NumberToString(dict_name_alt_id));
            }
            else {
                std::string err_msg =
                        "Fatal: Could not lookup node text in dict: "+
                        NumberToString(rawNode.GetId())+": "+name_alt;
                progress.Error(err_msg);
                return false;
            }
        }

        node.SetTags(progress,
                     typeConfig,
                     tags);

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

//        // save the dict id and FileOffsets
//        // do these fileoffsets match the final nodes.dat? NO
//        if(!name.empty()) {
//            table_it = table_dictkey_offsets.find(dict_name_id);
//            if(table_it == table_dictkey_offsets.end()) {
//                // no entries for this id yet, create a new one
//                std::pair<TextId,std::vector<FileOffset> > data;
//                data.first = dict_name_id;
//                table_it = table_dictkey_offsets.insert(data);
//            }
//            table_it->second.push_back(fileOffset);
//        }
//        if(!name_alt.empty()) {
//            table_it = table_dictkey_offsets.find(dict_name_alt_id);
//            if(table_it == table_dictkey_offsets.end()) {
//                // no entries for this id yet, create a new one
//                std::pair<TextId,std::vector<FileOffset> > data;
//                data.first = dict_name_alt_id;
//                table_it = table_dictkey_offsets.insert(data);
//            }
//            table_it->second.push_back(fileOffset);
//        }

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
