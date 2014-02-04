#ifndef OSMSCOUT_TEXTINDEX_H
#define OSMSCOUT_TEXTINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Preet Desai

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
    class OSMSCOUT_API TextIndex
    {

    public:
        TextIndex();
        void Close();
        bool Load(std::string const &path);

        bool Search(std::string const &query,
                    bool const searchPOIs,
                    bool const searchLocations,
                    bool const searchRegions,
                    bool const searchOther,
                    std::vector<TextId> &listResultIds,
                    std::vector<std::string> &listResultText) const;

        bool GetText(TextId const textId,
                     std::string &text) const;

        bool GetText(std::vector<TextId> const &listTextIds,
                     std::vector<std::string> listText);

        bool GetOffsets(std::vector<TextId> const &listIds,
                        std::vector<RefType> &listRefTypes,
                        std::vector<FileOffset> &listOffsets);

    private:
        std::string m_filepart;
        std::string m_datafilename;
        mutable FileScanner m_scanner;

        marisa::Trie m_trie_poi;
        marisa::Trie m_trie_loc;
        marisa::Trie m_trie_region;
        marisa::Trie m_trie_other;
    };
}




#endif // OSMSCOUT_TEXTINDEX_H
