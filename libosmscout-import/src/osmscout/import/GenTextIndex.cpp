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
#include <osmscout/import/GenTextIndex.h>

#include <marisa.h>

namespace osmscout
{

    std::string TextIndexGenerator::GetDescription() const
    {
        return "Generate text data files '(poi,loc,region,other)text.idx'";
    }


    bool TextIndexGenerator::Import(ImportParameter const &parameter,
                                    Progress &progress,
                                    TypeConfig const &typeConfig)
    {
        // TODO we can set this dynamically
        // based on max FileOffset and max TextId
        uint8_t const sz_idx_bytes=4;
        uint8_t const sz_offset_bytes=8;

        // TODO desc
        uint32_t textid_mask=(std::numeric_limits<uint32_t>::max() >> 2);

        // To figure out which type of object an offset
        // in the index is (node, way or area), we set
        // the two MSB in the offset as follows:

        // node: From MSB: 0 1
        FileOffset setbits_node=0;
        setbits_node |= (1 << ((sizeof(FileOffset)*8)-2));

        // way: From MSB: 1 0
        FileOffset setbits_way=0;
        setbits_way  |= (1 << ((sizeof(FileOffset)*8)-1));

        // area: From MSB: 1 1
        FileOffset setbits_area=0;
        setbits_area |= (1 << ((sizeof(FileOffset)*8)-1));
        setbits_area |= (1 << ((sizeof(FileOffset)*8)-2));


        // file paths
        std::string const destdir=parameter.GetDestinationDirectory();

        std::vector<std::string> list_text_datfiles;
        list_text_datfiles.push_back(AppendFileToDir(destdir,"poitext.dat"));
        list_text_datfiles.push_back(AppendFileToDir(destdir,"loctext.dat"));
        list_text_datfiles.push_back(AppendFileToDir(destdir,"regiontext.dat"));
        list_text_datfiles.push_back(AppendFileToDir(destdir,"othertext.dat"));

        std::vector<std::string> list_text_idxfiles;
        list_text_idxfiles.push_back(AppendFileToDir(destdir,"poitext.idx"));
        list_text_idxfiles.push_back(AppendFileToDir(destdir,"loctext.idx"));
        list_text_idxfiles.push_back(AppendFileToDir(destdir,"regiontext.idx"));
        list_text_idxfiles.push_back(AppendFileToDir(destdir,"othertext.idx"));

        // tries
        marisa::Trie trie_poi;
        marisa::Trie trie_loc;
        marisa::Trie trie_region;
        marisa::Trie trie_other;

        std::vector<marisa::Trie*> list_tries;
        list_tries.push_back(&trie_poi);
        list_tries.push_back(&trie_loc);
        list_tries.push_back(&trie_region);
        list_tries.push_back(&trie_other);

        // list of offsets by textid for each trie
        std::vector<std::vector<std::set<FileOffset> > > list_list_offsets_textid;
        list_list_offsets_textid.resize(4);

        progress.Info("Reading in text dat files...");
        for(size_t i=0; i < list_tries.size(); i++) {
            // open/load the index file
            try {
                list_tries[i]->load(list_text_datfiles[i].c_str());
            }
            catch(marisa::Exception const &ex) {
                std::string err_msg=
                        "Error opening "+list_text_datfiles[i]+": "+
                        std::string(ex.what());
                progress.Error(err_msg);
                return false;
            }

            std::size_t textid_count=list_tries[i]->num_keys();
            std::string msg=list_text_datfiles[i]+" has "+
                            NumberToString(textid_count)+" keys";
            progress.Info(msg);

            // should never, ever *ever* get here
            if(textid_count >= std::numeric_limits<uint32_t>::max()) {
                std::string err_msg=
                        "Too many unique text strings: "+
                        NumberToString(textid_count)+
                        " (exceeds 32-bit index), aborting";
                progress.Error(err_msg);
                return false;
            }

            // resize list_offsets_textid to assign
            // TextIds since we use index==TextId
            std::vector<std::set<FileOffset> > & list_offsets_text_id =
                    list_list_offsets_textid[i];

            list_offsets_text_id.resize(textid_count);
        }

        // (nodes)

        // Read in all nodes one by one and create
        // a look up for TextId->list of FileOffsets
        std::string const file_nodes =
            AppendFileToDir(parameter.GetDestinationDirectory(),"nodes.dat");

        FileScanner scanner;
        if(!scanner.Open(file_nodes,FileScanner::Sequential,false) ||
           !scanner.GotoBegin()) {
            progress.Error("Cannot open 'nodes.dat'");
            return false;
        }

        // Get number of nodes in 'nodes.dat'
        uint32_t node_dat_count;
        if(!scanner.Read(node_dat_count)) {
            progress.Error("Could not read node count in 'nodes.dat'");
            return false;
        }

        // for each node
        progress.Info("Scanning in nodes...");
        FileOffset nodes_dat_offset;
        for(uint32_t i=0; i < node_dat_count; i++) {
            scanner.GetPos(nodes_dat_offset);

            Node node;
            if(!node.Read(scanner)) {
                std::string err_msg=
                        "Failed to read in nodes.dat offset "+
                        NumberToString(nodes_dat_offset);
                progress.Error(err_msg);
                return false;
            }

            // modify the offset with so that we know its a node
            FileOffset node_mod_offset = nodes_dat_offset | setbits_node;

            TypeInfo typeInfo=typeConfig.GetTypeInfo(node.GetType());
            std::vector<bool> list_type_match(4,false);
            list_type_match[0]=typeInfo.GetIndexAsPOI();
            list_type_match[1]=typeInfo.GetIndexAsLocation();
            list_type_match[2]=typeInfo.GetIndexAsRegion();
            list_type_match[3]=!(list_type_match[0] || list_type_match[1] || list_type_match[2]);

            for(size_t j=0; j < list_type_match.size(); j++) {
                if(list_type_match[j]) {
                    // save this node's FileOffset by its TextId
                    NodeAttributes const &nodeAttr = node.GetAttributes();
                    if(nodeAttr.HasName()) {
                        uint32_t textid = (uint32_t(nodeAttr.GetNameId()) & textid_mask);
                        list_list_offsets_textid[j][textid].insert(node_mod_offset);
                    }
                    if(nodeAttr.HasNameAlt()) {
                        uint32_t textid = (uint32_t(nodeAttr.GetNameAltId()) & textid_mask);
                        list_list_offsets_textid[j][textid].insert(node_mod_offset);
                    }
                }
            }
        }
        scanner.Close();


        // (poi,loc,region,other)text.idx file format desc
        //
        // (starting from byte 0)
        // size                     // desc
        //
        // uint32_t                 textid_count
        //                          Number of TextIds in the index.
        //
        // uint32_t *               list_textid_lookup_offsets
        // textid_count             List of local file offsets that point to the list
        //                          of object data file offsets for the given TextId.
        //
        // uint32_t                 tail_textid_lookup_offsets
        //                          Special entry at the end of local file offsets that
        //                          points to the byte after the last object data file
        //                          offset. This lets us know the size of the last list
        //                          of object data file offsets by subtracting the tail
        //                          offset value from the offset value immediately
        //                          preceding it.
        //
        // sizeof(FileOffset) *     list_object_offsets
        // num. node offsets        List of object data file offsets.

        // Write the index files for each data file
        for(size_t i=0; i < list_text_idxfiles.size(); i++) {
            progress.Info("Writing index: "+list_text_idxfiles[i]);

            FileWriter writer;
            if(!writer.Open(list_text_idxfiles[i])) {
                progress.Error("Cannot create "+list_text_idxfiles[i]);
                return false;
            }

            // write the total number of TextIds
            std::vector<std::set<FileOffset> > & list_offsets_text_id =
                    list_list_offsets_textid[i];

            uint32_t textid_count=list_offsets_text_id.size();
            writer.Write(textid_count);

            // write the local file offsets

            // the first object offset will start at the current position
            // plus the size of all list_textid_lookup_offsets (including
            // the tail)
            FileOffset offset_start_textids;
            writer.GetPos(offset_start_textids);            // current pos

            FileOffset offset_obj_offsets =
                    offset_start_textids+
                    (sizeof(uint32_t)*(textid_count+1));    // +1 for tail

            // write list_textid_lookup_offsets
            for(uint32_t j=0; j < textid_count; j++) {
                writer.Write(uint32_t(offset_obj_offsets));
                offset_obj_offsets+=(list_offsets_text_id[j].size()*sizeof(FileOffset));
            }
            // tail_textid_lookup_offsets
            writer.Write(uint32_t(offset_obj_offsets));

            // write list_node_offsets
            for(size_t j=0; j < list_offsets_text_id.size(); j++) {
                std::set<FileOffset>::iterator it;
                for(it=list_offsets_text_id[j].begin();
                    it!=list_offsets_text_id[j].end(); ++it) {
                    writer.WriteFileOffset(*it);
                }
            }
            writer.Close();
        }

        return true;
    }

//    void TextIndexGenerator::testNodeTextIndex(ImportParameter const &parameter,
//                                                   Progress &progress)
//    {
//        // TODO
//        // check what happens when we try to save/open an
//        // empty trie with libmarisa

//        // Open up the node text trie
//        std::string const file_nodetextdict =
//            AppendFileToDir(parameter.GetDestinationDirectory(),"nodetext.dict");

//        marisa::Trie trie;
//        try {
//            trie.load(file_nodetextdict.c_str());
//        }
//        catch(marisa::Exception const &ex) {
//            std::string err_msg=
//                    "TEST /Cannot open 'nodetext.dict': "+
//                    std::string(ex.what());
//            progress.Error(err_msg);
//            return;
//        }

//        if(trie.empty()) {
//            progress.Info("TEST / Note: nodetext trie empty!");
//            return;
//        }

//        // Open the node text index
//        std::string const file_nodetextidx =
//            AppendFileToDir(parameter.GetDestinationDirectory(),"nodetext.idx");

//        FileScanner scanner;
//        if(!scanner.Open(file_nodetextidx,FileScanner::Sequential,true)) {
//            progress.Error("TEST / Cannot open 'nodetext.idx'");
//        }


//        // read in number of TextIds
//        uint32_t textid_count;
//        scanner.Read(textid_count);

//        // read in offsets:
//        // vector index: TextId
//        // uint32_t: local file lookup offset
//        // uint32_t: num node data file offsets for TextId
//        // Note
//        // The last index is the tail
//        std::vector<std::pair<uint32_t,uint32_t> > list_offsets;

//        for(uint32_t i=0; i < textid_count; i++) {
//            std::pair<uint32_t,uint32_t> insData;
//            scanner.Read(insData.first);
//            insData.second=0;
//            list_offsets.push_back(insData);
//        }
//        //progress.Info("TEST / COUNT "+NumberToString(textid_count));

//        // Get the number of offsets for each TextId by
//        // finding the difference between adjacent offsets
//        for(uint32_t i=0; i < textid_count; i++) {
//            list_offsets[i].second=(list_offsets[i+1].first-list_offsets[i].first)/sizeof(FileOffset);
//        }

//        // get some textids:
//        std::vector<TextId> list_lookup_textids;
//        marisa::Agent agent;
//        agent.set_query("T");

//        while(trie.predictive_search(agent)) {
//            TextId textid = static_cast<TextId>(agent.key().id());
//            std::string count_objs = NumberToString(list_offsets[textid].first);
//            //std::string count_objs = NumberToString(textid);
//            std::string msg=
//                    "DEBUG / "+
//                    std::string(agent.key().ptr(),agent.key().length())+
//                    " --- "+
//                    count_objs;
//            progress.Info(msg);
//        }
//    }
}
