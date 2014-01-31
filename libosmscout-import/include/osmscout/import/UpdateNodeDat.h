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

#include <osmscout/import/Import.h>
#include <osmscout/Database.h>

namespace osmscout
{

    class NodeDataUpdater : public ImportModule
    {
    public:
        std::string GetDescription() const;
        bool Import(ImportParameter const &parameter,
                    Progress &progress,
                    TypeConfig const &typeConfig);

    private:
        bool findLocalRegion(double const lon,
                             double const lat,
                             ObjectFileRef &localRegion);

        //
        Database m_database;

        // prioritized list of types used to
        // query for the closest local region
        std::map<TypeId,size_t> m_region_node_prio_types;
        std::map<TypeId,size_t> m_region_area_prio_types;
        TypeSet m_region_node_typeset;
        TypeSet m_region_area_typeset;
    };
}
