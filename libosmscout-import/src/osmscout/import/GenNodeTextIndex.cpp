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

        FileOffset nodes_dat_offset;
        scanner.GetPos(nodes_dat_offset);

        // for each node
        while(!scanner.IsEOF()) {
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

            scanner.GetPos(nodes_dat_offset);
        }
        scanner.Close();

        // 'nodetext.idx' format:
        // [ [sz idx file offsets],   // offsetStartIdxOffsets
        //   [idx file offsets],      // offsetStartNodeOffsets
        //   [linear node dat file offsets] ]

        // The first byte is an unsigned 32-bit int denoting
        // how many idx file offsets there are.

        // The beginning of the file consists of offsets to
        // the part of the file where the corresponding map
        // data file offsets are. So Byte 1 of nodetext.idx
        // will be a 32-bit long unsigned number that will
        // point to where the sequential list of FileOffsets
        // that correspond to TextId 1 begin in the file.

        // To determine how many FileOffsets are in the list,
        // subtract the adjacent byte's value and divide by
        // sizeof(FileOffset) bytes] (for the last TextId,
        // read until EOF)

        std::string const file_nodetextidx =
                AppendFileToDir(parameter.GetDestinationDirectory(),"nodetext.idx");

        FileWriter writer;
        if(!writer.Open(file_nodetextidx)) {
            progress.Error("Cannot create 'nodetext.idx'");
        }

        //
        FileOffset offsetStartIdxOffsets=sizeof(uint32_t);
        FileOffset offsetStartNodeOffsets=
                offsetStartIdxOffsets+
                (sizeof(TextId)*listOffsetsByTextId.size());

        // Write the total number of node text ids
        uint32_t num_text_ids = static_cast<uint32_t>(listOffsetsByTextId.size());
        writer.WriteNumber(num_text_ids);

        // Write the local file offsets
        // TODO: we can use 32-bit for offsets when they are
        //       small enough to save space
        FileOffset offsetNodeOffsets=offsetStartNodeOffsets;
        for(size_t i=0; i < listOffsetsByTextId.size(); i++) {
            // write the start of the set of node offsets
            writer.WriteNumber(uint32_t(offsetNodeOffsets));
            offsetNodeOffsets+=(listOffsetsByTextId[i].size()*sizeof(FileOffset));
        }

        // Write the node file offsets
        for(size_t i=0; i < listOffsetsByTextId.size(); i++) {
            std::set<FileOffset>::iterator it;
            for(it=listOffsetsByTextId[i].begin();
                it!=listOffsetsByTextId[i].end(); ++it) {
                writer.WriteFileOffset(*it);
            }
        }

        //
        writer.Close();

        testNodeTextIndex(parameter,progress);

        return true;
    }

    void NodeTextIndexGenerator::testNodeTextIndex(ImportParameter const &parameter,
                                                   Progress &progress)
    {
        // TODO
        // check what happens when we try to save an
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

        // Read in number of total text ids
        uint32_t num_text_ids = 0;
        scanner.ReadNumber(num_text_ids);

        // Set index start
        FileOffset offsetStartIdxOffsets;
        scanner.GetPos(offsetStartIdxOffsets);

        // read in local offsets index
        // FileOffset, count
        std::vector<std::pair<FileOffset,uint32_t> > listLocalOffsets;

        for(size_t i=0; i < num_text_ids; i++) {
            std::pair<FileOffset,uint32_t> insData;
            scanner.ReadNumber(insData.first);
            insData.second=0;
            listLocalOffsets.push_back(insData);
        }

        for(size_t i=0; i < num_text_ids-1; i++) {
            listLocalOffsets[i].second = (listLocalOffsets[i+1].first-listLocalOffsets[i].first)/sizeof(FileOffset);
        }
        // get last count
        scanner.SetPos(listLocalOffsets.back().first);
        while(!scanner.IsEOF()) {
            listLocalOffsets.back().second++;
        }


        // get some textids:
        marisa::Agent agent;
        agent.set_query("s");

        while(trie.predictive_search(agent)) {
            std::string msg="DEBUG / "+std::string(agent.key().ptr());
            progress.Info(msg);
        }
    }
}
