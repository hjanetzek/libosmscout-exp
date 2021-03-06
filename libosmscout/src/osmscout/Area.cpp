/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/Area.h>

#include <limits>

#include <osmscout/util/String.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  bool Area::Ring::GetCenter(double& lat, double& lon) const
  {
    double minLat=0.0;
    double minLon=0.0;
    double maxLat=0.0;
    double maxLon=0.0;

    bool start=true;

    for (size_t j=0; j<nodes.size(); j++) {
      if (start) {
        minLat=nodes[j].GetLat();
        minLon=nodes[j].GetLon();
        maxLat=nodes[j].GetLat();
        maxLon=nodes[j].GetLon();

        start=false;
      }
      else {
        minLat=std::min(minLat,nodes[j].GetLat());
        minLon=std::min(minLon,nodes[j].GetLon());
        maxLat=std::max(maxLat,nodes[j].GetLat());
        maxLon=std::max(maxLon,nodes[j].GetLon());
      }
    }

    if (start) {
      return false;
    }

    lat=minLat+(maxLat-minLat)/2;
    lon=minLon+(maxLon-minLon)/2;

    return true;
  }

  void Area::Ring::GetBoundingBox(double& minLon,
                                  double& maxLon,
                                  double& minLat,
                                  double& maxLat) const
  {
    assert(!nodes.empty());

    minLon=nodes[0].GetLon();
    maxLon=minLon;
    minLat=nodes[0].GetLat();
    maxLat=minLat;

    for (size_t i=1; i<nodes.size(); i++) {
      minLon=std::min(minLon,nodes[i].GetLon());
      maxLon=std::max(maxLon,nodes[i].GetLon());
      minLat=std::min(minLat,nodes[i].GetLat());
      maxLat=std::max(maxLat,nodes[i].GetLat());
    }
  }

  bool Area::GetCenter(double& lat, double& lon) const
  {
    assert(!rings.empty());

    double minLat=0.0;
    double minLon=0.0;
    double maxLat=0.0;
    double maxLon=0.0;

    bool start=true;

    for (size_t i=0; i<rings.size(); i++) {
      if (rings[i].ring==Area::outerRingId) {
        for (size_t j=0; j<rings[i].nodes.size(); j++) {
          if (start) {
            minLat=rings[i].nodes[j].GetLat();
            maxLat=minLat;
            minLon=rings[i].nodes[j].GetLon();
            maxLon=minLon;

            start=false;
          }
          else {
            minLat=std::min(minLat,rings[i].nodes[j].GetLat());
            minLon=std::min(minLon,rings[i].nodes[j].GetLon());
            maxLat=std::max(maxLat,rings[i].nodes[j].GetLat());
            maxLon=std::max(maxLon,rings[i].nodes[j].GetLon());
          }
        }
      }
    }

    assert(!start);

    if (start) {
      return false;
    }

    lat=minLat+(maxLat-minLat)/2;
    lon=minLon+(maxLon-minLon)/2;

    return true;
  }

  void Area::GetBoundingBox(double& minLon,
                            double& maxLon,
                            double& minLat,
                            double& maxLat) const
  {
    bool firstOuterRing=true;
    assert(!rings.empty());

    for (std::vector<Area::Ring>::const_iterator role=rings.begin();
         role!=rings.end();
         ++role) {
      if (role->ring==Area::outerRingId) {
        if (firstOuterRing) {
          role->GetBoundingBox(minLon,
                               maxLon,
                               minLat,
                               maxLat);

          firstOuterRing=false;
        }
        else {
          double ringMinLon;
          double ringMaxLon;
          double ringMinLat;
          double ringMaxLat;

          role->GetBoundingBox(ringMinLon,
                               ringMaxLon,
                               ringMinLat,
                               ringMaxLat);

          minLon=std::min(minLon,ringMinLon);
          minLat=std::min(minLat,ringMinLat);
          maxLon=std::max(maxLon,ringMaxLon);
          maxLat=std::max(maxLat,ringMaxLat);
        }
      }
    }
  }

  bool Area::ReadIds(FileScanner& scanner,
                     uint32_t nodesCount,
                     std::vector<Id>& ids)
  {
    ids.resize(nodesCount);

    Id minId;

    scanner.ReadNumber(minId);

    if (minId>0) {
      size_t idCurrent=0;

      while (idCurrent<ids.size()) {
        uint8_t bitset;
        size_t  bitmask=1;

        scanner.Read(bitset);

        for (size_t i=0; i<8 && idCurrent<ids.size(); i++) {
          if (bitset & bitmask) {
            scanner.ReadNumber(ids[idCurrent]);

            ids[idCurrent]+=minId;
          }
          else {
            ids[idCurrent]=0;
          }

          bitmask*=2;
          idCurrent++;
        }
      }
    }

    return !scanner.HasError();
  }

  bool Area::Read(const TypeConfig& typeConfig,
                  FileScanner& scanner)
  {
    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    TypeId   outerType;
    uint32_t ringCount=1;
    uint32_t nodesCount;

    if (!scanner.ReadNumber(outerType)) {
      return false;
    }

    if (outerType>typeConfig.GetMaxTypeId()) {
      outerType=outerType-typeConfig.GetMaxTypeId()-1;

      if (!scanner.ReadNumber(ringCount)) {
        return false;
      }

      ringCount++;
    }

    rings.resize(ringCount);

    TypeInfoRef type=typeConfig.GetTypeInfo((TypeId)outerType);

    rings[0].SetType(type);

    if (!rings[0].featureValueBuffer.GetType()->GetIgnore()) {
      if (!rings[0].featureValueBuffer.Read(scanner)) {
        return false;
      }
    }

    if (ringCount>1) {
      rings[0].ring=masterRingId;
    }
    else {
      rings[0].ring=outerRingId;
    }

    scanner.ReadNumber(nodesCount);

    if (nodesCount>0) {
      if (!ReadIds(scanner,
                   nodesCount,
                   rings[0].ids)) {
        return false;
      }

      if (!scanner.Read(rings[0].nodes,
                        nodesCount)) {
        return false;
      }
    }

    for (size_t i=1; i<ringCount; i++) {
      TypeId ringType;

      scanner.ReadNumber(ringType);

      type=typeConfig.GetTypeInfo(ringType);

      rings[i].SetType(type);

      if (!rings[i].featureValueBuffer.GetType()->GetIgnore()) {
        if (!rings[i].featureValueBuffer.Read(scanner)) {
          return false;
        }
      }

      scanner.Read(rings[i].ring);

      scanner.ReadNumber(nodesCount);

      if (nodesCount>0 &&
          !rings[i].GetType()->GetIgnore()) {
        if (!ReadIds(scanner,
                     nodesCount,
                     rings[i].ids)) {
          return false;
        }
      }

      if (nodesCount>0) {
        if (!scanner.Read(rings[i].nodes,
                          nodesCount)) {
          return false;
        }
      }
    }

    return !scanner.HasError();
  }

  bool Area::ReadOptimized(const TypeConfig& typeConfig,
                           FileScanner& scanner)
  {
    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    TypeId   outerType;
    uint32_t ringCount=1;
    uint32_t nodesCount;

    if (!scanner.ReadNumber(outerType)) {
      return false;
    }

    if (outerType>typeConfig.GetMaxTypeId()) {
      outerType=outerType-typeConfig.GetMaxTypeId()-1;

      if (!scanner.ReadNumber(ringCount)) {
        return false;
      }

      ringCount++;
    }

    rings.resize(ringCount);

    TypeInfoRef type=typeConfig.GetTypeInfo((TypeId)outerType);

    rings[0].SetType(type);

    if (!rings[0].featureValueBuffer.GetType()->GetIgnore()) {
      if (!rings[0].featureValueBuffer.Read(scanner)) {
        return false;
      }
    }

    if (ringCount>1) {
      rings[0].ring=masterRingId;
    }
    else {
      rings[0].ring=outerRingId;
    }

    scanner.ReadNumber(nodesCount);

    if (nodesCount>0) {
      if (!scanner.Read(rings[0].nodes,
                        nodesCount)) {
        return false;
      }
    }

    for (size_t i=1; i<ringCount; i++) {
      TypeId ringType;

      scanner.ReadNumber(ringType);

      type=typeConfig.GetTypeInfo(ringType);

      rings[i].SetType(type);

      if (!rings[i].featureValueBuffer.GetType()->GetIgnore()) {
        if (!rings[i].featureValueBuffer.Read(scanner)) {
          return false;
        }
      }

      scanner.Read(rings[i].ring);

      scanner.ReadNumber(nodesCount);

      if (nodesCount>0) {
        if (!scanner.Read(rings[i].nodes,
                          nodesCount)) {
          return false;
        }
      }
    }

    return !scanner.HasError();
  }

  bool Area::WriteIds(FileWriter& writer,
                      const std::vector<Id>& ids) const
  {
    Id minId=0;

    for (size_t i=0; i<ids.size(); i++) {
      if (ids[i]!=0) {
        if (minId==0) {
          minId=ids[i];
        }
        else {
          minId=std::min(minId,ids[i]);
        }
      }
    }

    writer.WriteNumber(minId);

    if (minId>0) {
      size_t idCurrent=0;

      while (idCurrent<ids.size()) {
        uint8_t bitset=0;
        size_t  bitMask=1;
        size_t  idEnd=std::min(idCurrent+8,ids.size());

        for (size_t i=idCurrent; i<idEnd; i++) {
          if (ids[i]!=0) {
            bitset=bitset | bitMask;
          }

          bitMask*=2;
        }

        writer.Write(bitset);

        for (size_t i=idCurrent; i<idEnd; i++) {
          if (ids[i]!=0) {
            writer.WriteNumber(ids[i]-minId);
          }

          bitMask=bitMask*2;
        }

        idCurrent+=8;
      }
    }

    return !writer.HasError();
  }

  bool Area::Write(const TypeConfig& typeConfig,
                   FileWriter& writer) const
  {
    std::vector<Ring>::const_iterator ring=rings.begin();

    // Outer ring

    if (rings.size()>1) {
      TypeId type=typeConfig.GetMaxTypeId()+1+
                  ring->GetType()->GetId();
      writer.WriteNumber(type);
      writer.WriteNumber((uint32_t)rings.size()-1);
    }
    else {
      writer.WriteNumber(ring->GetType()->GetId());
    }

    if (!ring->featureValueBuffer.GetType()->GetIgnore()) {
      if (!ring->featureValueBuffer.Write(writer)) {
        return false;
      }
    }

    writer.WriteNumber((uint32_t)ring->nodes.size());

    if (!ring->nodes.empty()) {
      if (!WriteIds(writer,
                    ring->ids)) {
        return false;
      }

      if (!writer.Write(ring->nodes,
                        ring->nodes.size())) {
        return false;
      }
    }

    ++ring;

    // Potential additional rings

    while (ring!=rings.end()) {
      writer.WriteNumber(ring->GetType()->GetId());

      if (!ring->GetType()->GetIgnore()) {
        if (!ring->featureValueBuffer.Write(writer)) {
          return false;
        }
      }

      writer.Write(ring->ring);

      writer.WriteNumber((uint32_t)ring->nodes.size());

      if (!ring->nodes.empty() &&
          !ring->featureValueBuffer.GetType()->GetIgnore()) {
        if (!WriteIds(writer,
                      ring->ids)) {
          return false;
        }
      }

      if (!ring->nodes.empty()) {
        if (!writer.Write(ring->nodes,
                          ring->nodes.size())) {
          return false;
        }
      }

      ++ring;
    }

    return !writer.HasError();
  }

  bool Area::WriteOptimized(const TypeConfig& typeConfig,
                            FileWriter& writer) const
  {
    std::vector<Ring>::const_iterator ring=rings.begin();

    // Outer ring

    if (rings.size()>1) {
      TypeId type=typeConfig.GetMaxTypeId()+1+
                  ring->GetType()->GetId();
      writer.WriteNumber(type);
      writer.WriteNumber((uint32_t)rings.size()-1);
    }
    else {
      writer.WriteNumber(ring->GetType()->GetId());
    }

    if (!ring->featureValueBuffer.GetType()->GetIgnore()) {
      if (!ring->featureValueBuffer.Write(writer)) {
        return false;
      }
    }

    writer.WriteNumber((uint32_t)ring->nodes.size());

    if (!ring->nodes.empty()) {
      if (!writer.Write(ring->nodes,
                        ring->nodes.size())) {
        return false;
      }
    }

    ++ring;

    // Potential additional rings

    while (ring!=rings.end()) {
      writer.WriteNumber(ring->GetType()->GetId());

      if (!ring->featureValueBuffer.GetType()->GetIgnore()) {
        if (!ring->featureValueBuffer.Write(writer)) {
          return false;
        }
      }

      writer.Write(ring->ring);

      writer.WriteNumber((uint32_t)ring->nodes.size());

      if (!ring->nodes.empty()) {
        if (!writer.Write(ring->nodes,
                          ring->nodes.size())) {
          return false;
        }
      }

      ++ring;
    }

    return !writer.HasError();
  }
}

