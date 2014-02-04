#ifndef OSMSCOUT_IMPORT_GENNODEDAT_H
#define OSMSCOUT_IMPORT_GENNODEDAT_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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
#include <osmscout/Node.h>
#include <marisa.h>

namespace osmscout {

  class NodeDataGenerator : public ImportModule
  {
  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);

  private:
    bool loadTextDataTries(ImportParameter const &parameter,
                           Progress &progress,
                           marisa::Trie &trie_poi,
                           marisa::Trie &trie_loc,
                           marisa::Trie &trie_region,
                           marisa::Trie &trie_other);

    bool saveNodeTextIds(Progress &progress,
                         TypeConfig const &typeConfig,
                         std::string const &name,
                         std::string const &name_alt,
                         Node &node,
                         marisa::Trie &trie_poi,
                         marisa::Trie &trie_loc,
                         marisa::Trie &trie_region,
                         marisa::Trie &trie_other);

    void cutNameDataFromTags(TypeConfig const &typeConfig,
                             std::vector<Tag> &tags,
                             std::string &name,
                             std::string &name_alt);
  };
}

#endif
