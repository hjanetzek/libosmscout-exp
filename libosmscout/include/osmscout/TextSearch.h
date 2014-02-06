#ifndef OSMSCOUT_IMPORT_TEXTSEARCH_H
#define OSMSCOUT_IMPORT_TEXTSEARCH_H

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

#include <osmscout/TypeSet.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/util/FileScanner.h>

#include <marisa.h>

namespace osmscout
{
    class TextSearch
    {
    public:
//        typedef std::vector<std::pair<std::string,std::vector<ObjectFileRef> > > ResultsList;
        typedef OSMSCOUT_HASHMAP<std::string,std::vector<ObjectFileRef> > ResultsMap;

        TextSearch();

        bool Load(std::string const &path);

        bool Search(std::string const &query,
                    bool const searchPOIs,
                    bool const searchLocations,
                    bool const searchRegions,
                    bool const searchOther,
                    ResultsMap &results) const;

    private:
        void splitSearchResult(std::string const &result,
                               std::string &text,
                               ObjectFileRef &ref) const;

        marisa::Trie m_trie_poi;
        marisa::Trie m_trie_loc;
        marisa::Trie m_trie_region;
        marisa::Trie m_trie_other;

        std::vector<marisa::Trie*> m_list_tries;
    };
}


#endif // OSMSCOUT_IMPORT_TEXTSEARCH_H
