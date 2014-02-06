#ifndef OSMSCOUT_IMPORT_GENTEXTDAT_H
#define OSMSCOUT_IMPORT_GENTEXTDAT_H

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

#include <osmscout/Types.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/import/Import.h>
#include <marisa.h>

namespace osmscout
{
    class TextDataGenerator : public ImportModule
    {
    public:
        TextDataGenerator();

        std::string GetDescription() const;

        bool Import(ImportParameter const &parameter,
                    Progress &progress,
                    TypeConfig const &typeConfig);

    private:
        bool setFileOffsetSize(ImportParameter const &parameter,
                               Progress &progress);

        bool addNodeTextToKeysets(ImportParameter const &parameter,
                                  Progress &progress,
                                  TypeConfig const &typeConfig);

        bool addWayTextToKeysets(ImportParameter const &parameter,
                                 Progress &progress,
                                 TypeConfig const &typeConfig);

        bool addAreaTextToKeysets(ImportParameter const &parameter,
                                  Progress &progress,
                                  TypeConfig const &typeConfig);

        bool buildKeyStr(std::string const &text,
                         FileOffset const offset,
                         RefType const reftype,
                         std::string &keystr) const;

        uint8_t getMinBytesForValue(uint64_t val) const;

        marisa::Keyset m_keyset_poi;
        marisa::Keyset m_keyset_loc;
        marisa::Keyset m_keyset_region;
        marisa::Keyset m_keyset_other;
        uint8_t m_sz_offset;
    };
}


#endif // OSMSCOUT_IMPORT_GENTEXTDAT_H
