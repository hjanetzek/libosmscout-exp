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

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/import/UpdateNodeDat.h>

namespace osmscout
{

    std::string NodeDataUpdater::GetDescription() const
    {
        return "Update 'nodes.dat'to add city/street data";
    }


    bool NodeDataUpdater::Import(ImportParameter const &parameter,
                                 Progress &progress,
                                 TypeConfig const &typeConfig)
    {
        std::string const file_nodes =
                AppendFileToDir(parameter.GetDestinationDirectory(),"nodes.dat");

        // Read in all of the nodes one by one and
        // overwrite the node data in-place with city
        // and street offsets as required
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

        // for each node
        while(!scanner.IsEOF()) {
            Node node;
            node.Read(scanner);

            if(typeConfig.GetTypeInfo(node.GetType()).GetFindCity()) {
                ObjectFileRef localRegion; // local region/city ref
                if(findLocalRegion(node.GetLon(),
                                   node.GetLat(),
                                   localRegion)) {

                }
                else {

                }
            }
        }

        return true;
    }

    bool NodeDataUpdater::findLocalRegion(double const lon,
                                          double const lat,
                                          ObjectFileRef &localRegion)
    {

    }
}
