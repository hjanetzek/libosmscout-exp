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
    TextDataGenerator::TextDataGenerator() :
        m_sz_offset(4)
    {
        // no code
    }

    std::string TextDataGenerator::GetDescription() const
    {
        return "Generate text data files 'text(poi,loc,region,other).dat'";
    }


    bool TextDataGenerator::Import(ImportParameter const &parameter,
                                 Progress &progress,
                                 TypeConfig const &typeConfig)
    {
        if(!this->setFileOffsetSize(parameter,
                                    progress)) {
            return false;
        }
        progress.Info("Using "+NumberToString(m_sz_offset)+"-byte offsets");

        // add node text data
        if(!this->addNodeTextToKeysets(parameter,
                                       progress,
                                       typeConfig)) {
            return false;
        }
        if(!this->addWayTextToKeysets(parameter,
                                      progress,
                                      typeConfig)) {
            return false;
        }
        if(!this->addAreaTextToKeysets(parameter,
                                       progress,
                                       typeConfig)) {
            return false;
        }

        // Create a file offset size string to indicate
        // how many bytes are used for offsets in the trie

        // We use an ASCII control character to denote
        // the start of the sz offset key:
        // 0x04: EOT
        std::string sz_offset_str;
        sz_offset_str.push_back(4);
        sz_offset_str+=NumberToString(m_sz_offset);

        // build and save tries
        std::vector<marisa::Keyset*> list_keysets;
        list_keysets.push_back(&m_keyset_poi);
        list_keysets.push_back(&m_keyset_loc);
        list_keysets.push_back(&m_keyset_region);
        list_keysets.push_back(&m_keyset_other);

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
            // add sz_offset to the keyset
            list_keysets[i]->push_back(sz_offset_str.c_str(),
                                       sz_offset_str.length());

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

    bool TextDataGenerator::setFileOffsetSize(ImportParameter const &parameter,
                                              Progress &progress)
    {
        progress.SetAction("Calculating required FileOffset size...");

        FileScanner scanner;

        uint8_t min_sz_node_offset=0;
        uint8_t min_sz_way_offset=0;
        uint8_t min_sz_area_offset=0;

        // The dat files use a 4 byte number to
        // indicate data count at the beginning
        // of the file
        uint32_t node_count=0;
        uint32_t way_count=0;
        uint32_t area_count=0;

        FileOffset node_filesize=0;
        FileOffset way_filesize=0;
        FileOffset area_filesize=0;

        // node count
        std::string file_nodes_dat=
                AppendFileToDir(parameter.GetDestinationDirectory(),
                                "nodes.dat");

        if(!scanner.Open(file_nodes_dat,
                         FileScanner::Sequential,
                         false)) {
            progress.Error("Cannot open 'nodes.dat'");
            return false;
        }
        if(!scanner.Read(node_count)) {
            progress.Error("Error reading node count in 'nodes.dat'");
            return false;
        }
        // seek to end of file (better way to do this?)
        for(uint32_t i=0; i < node_count; i++) {
            Node node;
            if(!node.Read(scanner)) {
                progress.Error("Error seeking to end of 'nodes'.dat!");
                return false;
            }
        }
        scanner.GetPos(node_filesize);
        scanner.Close();


        // way count
        std::string file_ways_dat=
                AppendFileToDir(parameter.GetDestinationDirectory(),
                                "ways.dat");

        if(!scanner.Open(file_ways_dat,
                         FileScanner::Sequential,
                         false)) {
            progress.Error("Cannot open 'ways.dat'");
            return false;
        }
        if(!scanner.Read(way_count)) {
            progress.Error("Error reading way count in 'ways.dat'");
            return false;
        }
        // seek to end of file
        for(uint32_t i=0; i < way_count; i++) {
            Way way;
            if(!way.Read(scanner)) {
                progress.Error("Error seeking to end of 'ways'.dat!");
                return false;
            }
        }
        scanner.GetPos(way_filesize);
        scanner.Close();

        // area count
        std::string file_areas_dat=
                AppendFileToDir(parameter.GetDestinationDirectory(),
                                "areas.dat");

        if(!scanner.Open(file_areas_dat,
                         FileScanner::Sequential,
                         false)) {
            progress.Error("Cannot open 'areas.dat'");
            return false;
        }
        if(!scanner.Read(area_count)) {
            progress.Error("Error reading area count in 'areas.dat'");
            return false;
        }
        // seek to end of file
        for(uint32_t i=0; i < area_count; i++) {
            Area area;
            if(!area.Read(scanner)) {
                progress.Error("Error seeking to end of 'areas'.dat!");
                return false;
            }
        }
        scanner.GetPos(area_filesize);
        scanner.Close();

        // Check if we need to increase num. bytes used
        // to store offsets
        min_sz_node_offset = getMinBytesForValue(node_filesize);
        min_sz_way_offset = getMinBytesForValue(way_filesize);
        min_sz_area_offset = getMinBytesForValue(area_filesize);

        progress.Info("Node filesize is " + NumberToString(node_filesize) + " bytes, "+
                      "req. " + NumberToString(min_sz_node_offset) +" bytes");

        progress.Info("Way filesize is " + NumberToString(way_filesize) + " bytes, "+
                      "req. " + NumberToString(min_sz_way_offset) +" bytes");

        progress.Info("Area filesize is " + NumberToString(area_filesize) + " bytes, "+
                      "req. " + NumberToString(min_sz_area_offset) +" bytes");

        m_sz_offset = 0;
        m_sz_offset = std::max(min_sz_node_offset,min_sz_way_offset);
        m_sz_offset = std::max(m_sz_offset,min_sz_area_offset);

        return true;
    }

    bool TextDataGenerator::addNodeTextToKeysets(ImportParameter const &parameter,
                                                 Progress &progress,
                                                 TypeConfig const &typeConfig)
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
                    keyset = &m_keyset_poi;
                }
                else if(typeInfo.GetIndexAsLocation()) {
                    keyset = &m_keyset_loc;
                }
                else if(typeInfo.GetIndexAsRegion()) {
                    keyset = &m_keyset_region;
                }
                else {
                    keyset = &m_keyset_other;
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
                                                TypeConfig const &typeConfig)
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
                    keyset = &m_keyset_poi;
                }
                else if(typeInfo.GetIndexAsLocation()) {
                    keyset = &m_keyset_loc;
                }
                else if(typeInfo.GetIndexAsRegion()) {
                    keyset = &m_keyset_region;
                }
                else {
                    keyset = &m_keyset_other;
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
                                                 TypeConfig const &typeConfig)
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
                    keyset = &m_keyset_poi;
                }
                else if(areaTypeInfo.GetIndexAsLocation()) {
                    keyset = &m_keyset_loc;
                }
                else if(areaTypeInfo.GetIndexAsRegion()) {
                    keyset = &m_keyset_region;
                }
                else {
                    keyset = &m_keyset_other;
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
        // ASCII 0x01 'SOH' - corresponds to refNode
        // ASCII 0x02 'STX' - corresponds to refArea
        // ASCII 0x03 'ETX' - corresponds to refWay

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

        char buffer[m_sz_offset];
        for(uint8_t i=0; i < m_sz_offset; i++) {
            uint8_t r=m_sz_offset-1-i;
            buffer[r] = ((offset >> (i*8)) & 0xff);
        }

        for(uint8_t i=0; i < m_sz_offset; i++) {
            keystr.push_back(buffer[i]);
        }

        return true;
    }

    uint8_t TextDataGenerator::getMinBytesForValue(uint64_t val) const
    {
        uint8_t n=0;
        while(val != 0) {
            val >>= 8;
            n++;
        }
        return n;
    }
}
