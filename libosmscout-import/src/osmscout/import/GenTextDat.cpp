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

#include <osmscout/Node.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/String.h>
#include <osmscout/import/RawNode.h>
#include <osmscout/import/GenTextDat.h>

#include <marisa.h>

namespace osmscout
{
    std::string TextDataGenerator::GetDescription() const
    {
        return "Generate text data files '(poi,loc,region,other)text.dat'";
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

        // set the max keycount
        // We use 32-bit integers to save each key id,
        // but set the first two MSB to denote which trie
        // the key belongs to (poi,loc,region,other).
        // This gives us a max_keycount of 2^30-1 per trie.
        uint32_t max_keycount=(std::numeric_limits<uint32_t>::max() >> 2);


        // add node text data
        if(!this->addNodeTextToKeysets(parameter,
                                       progress,
                                       typeConfig,
                                       max_keycount,
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
                                                  "poitext.dat"));

        list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                  "loctext.dat"));

        list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                  "regiontext.dat"));

        list_trie_files.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                  "othertext.dat"));

        for(size_t i=0; i < list_keysets.size(); i++) {
            marisa::Trie trie;
            try {
                trie.build(*(list_keysets[i]),
                           MARISA_DEFAULT_NUM_TRIES |
                           MARISA_DEFAULT_TAIL |
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

        return false;
    }

    bool TextDataGenerator::addNodeTextToKeysets(ImportParameter const &parameter,
                                                 Progress &progress,
                                                 TypeConfig const &typeConfig,
                                                 uint32_t const max_keycount,
                                                 marisa::Keyset &keyset_poi,
                                                 marisa::Keyset &keyset_loc,
                                                 marisa::Keyset &keyset_region,
                                                 marisa::Keyset &keyset_other)
    {
        progress.SetAction("Getting node text data");

        // Open rawnodes.dat
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

        // Iterate through each node and add text
        // data to the corresponding keyset
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
                cutNodeNameDataFromTags(typeConfig,tags,name,name_alt);

                // pick right keyset based on type info
                // Note: This does not allow multiple indexes
                //       ie (POI and location) for the same type
                //       (should I change this behavior?)
                TypeInfo typeInfo=typeConfig.GetTypeInfo(rawNode.GetType());
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

                if(keyset->num_keys() >= max_keycount) {
                    progress.Error("Max keycount exceeded: "+
                                   NumberToString(keyset->num_keys()));
                    return false;
                }

                // save to keyset
                if(!name.empty()) {
                    keyset->push_back(name.c_str());
                }
                if(!name_alt.empty()) {
                    keyset->push_back(name_alt.c_str());
                }
            }
        }
        scanner.Close();

        return true;
    }

    void TextDataGenerator::cutNodeNameDataFromTags(TypeConfig const &typeConfig,
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
