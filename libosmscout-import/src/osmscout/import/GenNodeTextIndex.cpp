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
#include <osmscout/import/GenNodeTextIndex.h>

#include <marisa.h>

namespace osmscout
{

    std::string NodeTextIndexGenerator::GetDescription() const
    {
        return "Write TextId lookup file for node text data";
    }


    bool NodeTextIndexGenerator::Import(ImportParameter const &parameter,
                                 Progress &progress,
                                 TypeConfig const &typeConfig)
    {


        std::string const file_nodes =
            AppendFileToDir(parameter.GetDestinationDirectory(),"nodes.dat");

        // Read in all of the nodes one by one and create
        // a look up for TextId->list of FileOffsets
        std::vector<std::set<FileOffset> > listOffsetsByTextId;

        // get the number of textids in the tree
        {
            // Open up the node text trie
            std::string const file_nodetextdict =
                AppendFileToDir(parameter.GetDestinationDirectory(),"nodetext.dict");

            marisa::Trie trie;
            try {
                trie.load(file_nodetextdict.c_str());
            }
            catch(marisa::Exception const &ex) {
                std::string err_msg=
                        "Cannot open 'nodetext.dict': "+
                        std::string(ex.what());
                progress.Error(err_msg);
                return false;
            }

            if(trie.empty()) {
                progress.Info("Note: nodetext trie empty!");
                return true;
            }
            listOffsetsByTextId.resize(trie.num_keys());
        }

        FileScanner scanner;
        if(!scanner.Open(file_nodes,
                         FileScanner::Sequential,
                         false)) {
            progress.Error("Cannot open 'nodes.dat'");
            return false;
        }

        if(!scanner.GotoBegin()) {
            progress.Error("Cannot seek to beginning of 'nodes.dat'");
            return false;
        }

        // Get number of nodes in 'nodes.dat'
        uint32_t node_dat_count;
        if(!scanner.Read(node_dat_count)) {
            progress.Error("Could not read node count in 'nodes.dat'");
            return false;
        }

        // for each node
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

            // save this node's FileOffset by its TextId
            NodeAttributes const &nodeAttr = node.GetAttributes();
            if(nodeAttr.HasName()) {
                size_t name_id = static_cast<size_t>(nodeAttr.GetNameId());
                listOffsetsByTextId[name_id].insert(nodes_dat_offset);
            }
            if(nodeAttr.HasNameAlt()) {
                size_t name_alt_id = static_cast<size_t>(nodeAttr.GetNameAltId());
                listOffsetsByTextId[name_alt_id].insert(nodes_dat_offset);
            }

//            std::string smsg=(node.GetAttributes().HasName()) ? "Yes" : "No";
//            progress.Info("DEBUG:"+smsg);
        }
        scanner.Close();




        // nodetext.idx file format desc
        //
        // (starting from byte 0)
        // size                     // desc
        //
        // uint32_t                 textid_count
        //                          Number of TextIds in the index.
        //
        // uint32_t *               list_textid_lookup_offsets
        // textid_count             List of local file offsets that point to the list
        //                          of node data file offsets for the given TextId.
        //
        // uint32_t                 tail_textid_lookup_offsets
        //                          Special entry at the end of local file offsets that
        //                          points to the byte after the last node data file
        //                          offset. This lets us know the size of the last list
        //                          of node data file offsets by subtracting the tail
        //                          offset value from the offset value immediately
        //                          preceding it.
        //
        // sizeof(FileOffset) *     list_node_offsets
        // num. node offsets        List of node data file offsets.


        std::string const file_nodetextidx =
                AppendFileToDir(parameter.GetDestinationDirectory(),"nodetext.idx");

        FileWriter writer;
        if(!writer.Open(file_nodetextidx)) {
            progress.Error("Cannot create 'nodetext.idx'");
        }

        // write the total number of TextIds
        uint32_t textid_count=listOffsetsByTextId.size();
        writer.Write(textid_count);

        // write the local file offsets

        // the first node offset will start at the current position
        // plus the size of all list_textid_lookup_offsets (including
        // the tail)
        FileOffset offset_start_textids;
        writer.GetPos(offset_start_textids);            // current pos

        FileOffset offset_node_offsets =
                offset_start_textids+
                (sizeof(uint32_t)*(textid_count+1));    // +1 for tail

        // write list_textid_lookup_offsets
        for(uint32_t i=0; i < textid_count; i++) {
            writer.Write(uint32_t(offset_node_offsets));
            offset_node_offsets+=(listOffsetsByTextId[i].size()*sizeof(FileOffset));
        }
        // tail_textid_lookup_offsets
        writer.Write(uint32_t(offset_node_offsets));

        // write list_node_offsets
        for(size_t i=0; i < listOffsetsByTextId.size(); i++) {
            std::set<FileOffset>::iterator it;
            for(it=listOffsetsByTextId[i].begin();
                it!=listOffsetsByTextId[i].end(); ++it) {
                writer.WriteFileOffset(*it);
            }
        }
        writer.Close();

        testNodeTextIndex(parameter,progress);

        return true;
    }

    void NodeTextIndexGenerator::testNodeTextIndex(ImportParameter const &parameter,
                                                   Progress &progress)
    {
        // TODO
        // check what happens when we try to save/open an
        // empty trie with libmarisa

        // Open up the node text trie
        std::string const file_nodetextdict =
            AppendFileToDir(parameter.GetDestinationDirectory(),"nodetext.dict");

        marisa::Trie trie;
        try {
            trie.load(file_nodetextdict.c_str());
        }
        catch(marisa::Exception const &ex) {
            std::string err_msg=
                    "TEST /Cannot open 'nodetext.dict': "+
                    std::string(ex.what());
            progress.Error(err_msg);
            return;
        }

        if(trie.empty()) {
            progress.Info("TEST / Note: nodetext trie empty!");
            return;
        }

        // Open the node text index
        std::string const file_nodetextidx =
            AppendFileToDir(parameter.GetDestinationDirectory(),"nodetext.idx");

        FileScanner scanner;
        if(!scanner.Open(file_nodetextidx,FileScanner::Sequential,true)) {
            progress.Error("TEST / Cannot open 'nodetext.idx'");
        }


        // read in number of TextIds
        uint32_t textid_count;
        scanner.Read(textid_count);

        // read in offsets:
        // vector index: TextId
        // uint32_t: local file lookup offset
        // uint32_t: num node data file offsets for TextId
        // Note
        // The last index is the tail
        std::vector<std::pair<uint32_t,uint32_t> > list_offsets;

        for(uint32_t i=0; i < textid_count; i++) {
            std::pair<uint32_t,uint32_t> insData;
            scanner.Read(insData.first);
            insData.second=0;
            list_offsets.push_back(insData);
        }
        //progress.Info("TEST / COUNT "+NumberToString(textid_count));

        // Get the number of offsets for each TextId by
        // finding the difference between adjacent offsets
        for(uint32_t i=0; i < textid_count; i++) {
            list_offsets[i].second=(list_offsets[i+1].first-list_offsets[i].first)/sizeof(FileOffset);
        }

        // get some textids:
        std::vector<TextId> list_lookup_textids;
        marisa::Agent agent;
        agent.set_query("T");

        while(trie.predictive_search(agent)) {
            TextId textid = static_cast<TextId>(agent.key().id());
            std::string count_objs = NumberToString(list_offsets[textid].first);
            //std::string count_objs = NumberToString(textid);
            std::string msg=
                    "DEBUG / "+
                    std::string(agent.key().ptr(),agent.key().length())+
                    " --- "+
                    count_objs;
            progress.Info(msg);
        }
    }
}
