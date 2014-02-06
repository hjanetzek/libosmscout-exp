/*
 This source is part of the libosmscout library
 Copyright (C) 2013 Preet Desai

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

#include <osmscout/Way.h>
#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/String.h>
#include <osmscout/import/GenTextDat.h>

#include <marisa.h>

namespace osmscout
{
    std::string TextDataGenerator::GetDescription() const
    {
        return "Generate text data files 'text(poi,loc,region,other).dat'";
    }


    bool TextDataGenerator::Import(ImportParameter const &parameter,
                                 Progress &progress,
                                 TypeConfig const &typeConfig)
    {
        // create keysets for text data
        marisa::Keyset keyset_poi;
        marisa::Keyset keyset_loc;
        marisa::Keyset keyset_region;
        marisa::Keyset keyset_other;

        // add node text data
        if(!this->addNodeTextToKeysets(parameter,
                                       progress,
                                       typeConfig,
                                       keyset_poi,
                                       keyset_loc,
                                       keyset_region,
                                       keyset_other)) {
            return false;
        }
        if(!this->addWayTextToKeysets(parameter,
                                      progress,
                                      typeConfig,
                                      keyset_poi,
                                      keyset_loc,
                                      keyset_region,
                                      keyset_other)) {
            return false;
        }
        if(!this->addAreaTextToKeysets(parameter,
                                       progress,
                                       typeConfig,
                                       keyset_poi,
                                       keyset_loc,
                                       keyset_region,
                                       keyset_other)) {
            return false;
        }


        // build and save tries
        std::vector<marisa::Keyset*> list_keysets;
        list_keysets.push_back(&keyset_poi);
        list_keysets.push_back(&keyset_loc);
        list_keysets.push_back(&keyset_region);
        list_keysets.push_back(&keyset_other);

        std::vector<std::string> list_trie_files;
        list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                  "textpoi.dat"));

        list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                  "textloc.dat"));

        list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                  "textregion.dat"));

        list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                  "textother.dat"));

        for(size_t i=0; i < list_keysets.size(); i++) {
            marisa::Trie trie;
            try {
                trie.build(*(list_keysets[i]),
                           MARISA_DEFAULT_NUM_TRIES |
                           MARISA_BINARY_TAIL |
                           MARISA_LABEL_ORDER |
                           MARISA_DEFAULT_CACHE);
            }
            catch (marisa::Exception const &ex){
                std::string err_msg="Error building:" +list_trie_files[i];
                err_msg.append(ex.what());
                progress.Error(err_msg);
                return false;
            }

            try {
                trie.save(list_trie_files[i].c_str());
            }
            catch (marisa::Exception const &ex){
                std::string err_msg="Error saving:" +list_trie_files[i];
                err_msg.append(ex.what());
                progress.Error(err_msg);
                return false;
            }
        }

        return true;
    }

    bool TextDataGenerator::addNodeTextToKeysets(ImportParameter const &parameter,
                                                 Progress &progress,
                                                 TypeConfig const &typeConfig,
                                                 marisa::Keyset &keyset_poi,
                                                 marisa::Keyset &keyset_loc,
                                                 marisa::Keyset &keyset_region,
                                                 marisa::Keyset &keyset_other)
    {
        progress.SetAction("Getting node text data");

        // Open nodes.dat
        std::string file_nodes_dat=
                AppendFileToDir(parameter.GetDestinationDirectory(),
                                "nodes.dat");

        FileScanner scanner;
        if(!scanner.Open(file_nodes_dat,
                         FileScanner::Sequential,
                         false)) {
            progress.Error("Cannot open 'nodes.dat'");
            return false;
        }

        uint32_t node_count=0;
        if(!scanner.Read(node_count)) {
            progress.Error("Error reading node count in 'nodes.dat'");
            return false;
        }

        // Iterate through each node and add text
        // data to the corresponding keyset
        for(uint32_t n=1; n <= node_count; n++) {
            Node node;
            if (!node.Read(scanner)) {
              progress.Error(std::string("Error while reading data entry ")+
                             NumberToString(n)+" of "+
                             NumberToString(node_count)+
                             " in file '"+
                             scanner.GetFilename()+"'");
              return false;
            }

            if(node.GetType() != typeIgnore &&
               !typeConfig.GetTypeInfo(node.GetType()).GetIgnore()) {

                NodeAttributes attr=node.GetAttributes();
                if(attr.GetName().empty() &&
                   attr.GetNameAlt().empty()) {
                    continue;
                }

                // Save name attributes of this node
                // in the right keyset
                TypeInfo typeInfo=typeConfig.GetTypeInfo(node.GetType());
                marisa::Keyset * keyset;
                if(typeInfo.GetIndexAsPOI()) {
                    keyset = &keyset_poi;
                }
                else if(typeInfo.GetIndexAsLocation()) {
                    keyset = &keyset_loc;
                }
                else if(typeInfo.GetIndexAsRegion()) {
                    keyset = &keyset_region;
                }
                else {
                    keyset = &keyset_other;
                }

                if(!(attr.GetName().empty())) {
                    std::string keystr;
                    if(buildKeyStr(attr.GetName(),
                                   node.GetFileOffset(),
                                   refNode,
                                   keystr))
                    {
                        keyset->push_back(keystr.c_str(),
                                          keystr.length());
                    }
                }
                if(!(attr.GetNameAlt().empty())) {
                    std::string keystr;
                    if(buildKeyStr(attr.GetNameAlt(),
                                   node.GetFileOffset(),
                                   refNode,
                                   keystr))
                    {
                        keyset->push_back(keystr.c_str(),
                                          keystr.length());
                    }
                }
            }
        }
        scanner.Close();

        return true;
    }

    bool TextDataGenerator::addWayTextToKeysets(ImportParameter const &parameter,
                                                Progress &progress,
                                                TypeConfig const &typeConfig,
                                                marisa::Keyset &keyset_poi,
                                                marisa::Keyset &keyset_loc,
                                                marisa::Keyset &keyset_region,
                                                marisa::Keyset &keyset_other)
    {
        progress.SetAction("Getting way text data");

        // Open ways.dat
        std::string file_ways_dat=
                AppendFileToDir(parameter.GetDestinationDirectory(),
                                "ways.dat");

        FileScanner scanner;
        if(!scanner.Open(file_ways_dat,
                         FileScanner::Sequential,
                         false)) {
            progress.Error("Cannot open 'ways.dat'");
            return false;
        }

        uint32_t way_count=0;
        if(!scanner.Read(way_count)) {
            progress.Error("Error reading way count in 'ways.dat'");
            return false;
        }

        // Iterate through each way and add text
        // data to the corresponding keyset
        for(uint32_t n=1; n <= way_count; n++) {
            Way way;
            if (!way.Read(scanner)) {
              progress.Error(std::string("Error while reading data entry ")+
                             NumberToString(n)+" of "+
                             NumberToString(way_count)+
                             " in file '"+
                             scanner.GetFilename()+"'");
              return false;
            }

            if(way.GetType() != typeIgnore &&
               !typeConfig.GetTypeInfo(way.GetType()).GetIgnore()) {

                WayAttributes attr=way.GetAttributes();
                if(attr.GetName().empty() &&
                   attr.GetNameAlt().empty() &&
                   attr.GetRefName().empty()) {
                    continue;
                }

                // Save name attributes of this node
                // in the right keyset
                TypeInfo typeInfo=typeConfig.GetTypeInfo(way.GetType());
                marisa::Keyset * keyset;
                if(typeInfo.GetIndexAsPOI()) {
                    keyset = &keyset_poi;
                }
                else if(typeInfo.GetIndexAsLocation()) {
                    keyset = &keyset_loc;
                }
                else if(typeInfo.GetIndexAsRegion()) {
                    keyset = &keyset_region;
                }
                else {
                    keyset = &keyset_other;
                }

                if(!(attr.GetName().empty())) {
                    std::string keystr;
                    if(buildKeyStr(attr.GetName(),
                                   way.GetFileOffset(),
                                   refWay,
                                   keystr))
                    {
                        keyset->push_back(keystr.c_str(),
                                          keystr.length());
                    }
                }
                if(!(attr.GetNameAlt().empty())) {
                    std::string keystr;
                    if(buildKeyStr(attr.GetNameAlt(),
                                   way.GetFileOffset(),
                                   refWay,
                                   keystr))
                    {
                        keyset->push_back(keystr.c_str(),
                                          keystr.length());
                    }
                }
                if(!(attr.GetRefName().empty())) {
                    std::string keystr;
                    if(buildKeyStr(attr.GetRefName(),
                                   way.GetFileOffset(),
                                   refWay,
                                   keystr))
                    {
                        keyset->push_back(keystr.c_str(),
                                          keystr.length());
                    }
                }
            }
        }
        scanner.Close();

        return true;
    }

    bool TextDataGenerator::addAreaTextToKeysets(ImportParameter const &parameter,
                                                 Progress &progress,
                                                 TypeConfig const &typeConfig,
                                                 marisa::Keyset &keyset_poi,
                                                 marisa::Keyset &keyset_loc,
                                                 marisa::Keyset &keyset_region,
                                                 marisa::Keyset &keyset_other)
    {
        progress.SetAction("Getting area text data");

        // Open areas.dat
        std::string file_areas_dat=
                AppendFileToDir(parameter.GetDestinationDirectory(),
                                "areas.dat");

        FileScanner scanner;
        if(!scanner.Open(file_areas_dat,
                         FileScanner::Sequential,
                         false)) {
            progress.Error("Cannot open 'areas.dat'");
            return false;
        }

        uint32_t area_count=0;
        if(!scanner.Read(area_count)) {
            progress.Error("Error reading area count in 'areas.dat'");
            return false;
        }

        // Iterate through each area and add text
        // data to the corresponding keyset
        for(uint32_t n=1; n <= area_count; n++) {
            Area area;
            if(!area.Read(scanner)) {
                progress.Error(std::string("Error while reading data entry ")+
                               NumberToString(n)+" of "+
                               NumberToString(area_count)+
                               " in file '"+
                               scanner.GetFilename()+"'");
                return false;
            }

            // Rings might have different types and names
            // so we check  each ring individually
            for(size_t r=0; r < area.rings.size(); r++) {

                TypeId areaType=area.rings[r].GetType();
                TypeInfo areaTypeInfo=typeConfig.GetTypeInfo(areaType);
                if(areaType==typeIgnore || areaTypeInfo.GetIgnore()) {
                    continue;
                }

                marisa::Keyset * keyset;
                if(areaTypeInfo.GetIndexAsPOI()) {
                    keyset = &keyset_poi;
                }
                else if(areaTypeInfo.GetIndexAsLocation()) {
                    keyset = &keyset_loc;
                }
                else if(areaTypeInfo.GetIndexAsRegion()) {
                    keyset = &keyset_region;
                }
                else {
                    keyset = &keyset_other;
                }

                AreaAttributes attr=area.rings[r].GetAttributes();
                if(!(attr.GetName().empty())) {
                    std::string keystr;
                    if(buildKeyStr(attr.GetName(),
                                   area.GetFileOffset(),
                                   refArea,
                                   keystr))
                    {
                        keyset->push_back(keystr.c_str(),
                                          keystr.length());
                    }
                }
                if(!(attr.GetNameAlt().empty())) {
                    std::string keystr;
                    if(buildKeyStr(attr.GetNameAlt(),
                                   area.GetFileOffset(),
                                   refArea,
                                   keystr))
                    {
                        keyset->push_back(keystr.c_str(),
                                          keystr.length());
                    }
                }
            }
        }

        return true;
    }

    bool TextDataGenerator::buildKeyStr(std::string const &text,
                                        FileOffset const offset,
                                        RefType const reftype,
                                        std::string &keystr) const
    {
        if(text.empty()) {
            return false;
        }

        keystr=text;

        // Use ASCII control characters to denote
        // the start of a file offset:
        // ASCII 0x01 'SOH', means a node
        // ASCII 0x02 'STX', means a way
        // ASCII 0x03 'ETX', means a area

        if(reftype == refNode) {
            char c=static_cast<char>(refNode);
            keystr.push_back(c);
        }
        else if(reftype == refWay) {
            char c=static_cast<char>(refWay);
            keystr.push_back(c);
        }
        else if(reftype == refArea) {
            char c=static_cast<char>(refArea);
            keystr.push_back(c);
        }
        else {
            return false;
        }

        // Write the FileOffset into an 8-byte buffer

        // Note that the order is MSB! This is done to
        // maximize the number of common string overlap
        // in the trie.

        // Consider the offsets
        // 0010, 0011, 0024, 0035
        // A trie would have one common branch for
        // '00', with different edges (1,2,3). If
        // LSB was written first, it would have four
        // branches immediately from its root.

        char buffer[8];
        buffer[7]=((offset >>  0) & 0xff);
        buffer[6]=((offset >>  8) & 0xff);
        buffer[5]=((offset >> 16) & 0xff);
        buffer[4]=((offset >> 24) & 0xff);
        buffer[3]=((offset >> 32) & 0xff);
        buffer[2]=((offset >> 40) & 0xff);
        buffer[1]=((offset >> 48) & 0xff);
        buffer[0]=((offset >> 56) & 0xff);

        for(uint8_t i=0; i < 8; i++) {
            keystr.push_back(buffer[i]);
        }

        return true;
    }
}
