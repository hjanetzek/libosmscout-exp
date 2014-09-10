LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

MARISA_SRC := marisa/lib/marisa

LOCAL_MODULE    := marisa 
LOCAL_C_INCLUDES := $(MARISA_SRC) $(MARISA_SRC)/grimoire

LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -Weffc++ -std=c++11 \
	-Wno-sign-compare -Wno-psabi -frtti -fexceptions

LOCAL_SRC_FILES := \
  $(MARISA_SRC)/keyset.cc \
  $(MARISA_SRC)/trie.cc \
  $(MARISA_SRC)/agent.cc \
  $(MARISA_SRC)/grimoire/io/mapper.cc \
  $(MARISA_SRC)/grimoire/io/writer.cc \
  $(MARISA_SRC)/grimoire/io/reader.cc \
  $(MARISA_SRC)/grimoire/vector/bit-vector.cc \
  $(MARISA_SRC)/grimoire/trie/tail.cc \
  $(MARISA_SRC)/grimoire/trie/louds-trie.cc

LOCAL_EXPORT_C_INCLUDES := marisa/lib
#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := osmscout
LOCAL_C_INCLUDES := ../config include

LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -D__ANDROID__ \
	-Wno-sign-compare -Wno-psabi -frtti -std=c++11

#LOCAL_CFLAGS += -fopenmp

LOCAL_STATIC_LIBRARIES := marisa
#LOCAL_SHARED_LIBRARIES := marisa#
LOCAL_SRC_FILES := \
		osmscout/util/Breaker.cpp \
		osmscout/util/Cache.cpp \
		osmscout/util/Color.cpp \
		osmscout/util/File.cpp \
		osmscout/util/FileScanner.cpp \
		osmscout/util/FileWriter.cpp \
		osmscout/util/Geometry.cpp \
		osmscout/util/HashMap.cpp \
		osmscout/util/HashSet.cpp \
		osmscout/util/Magnification.cpp \
		osmscout/util/NodeUseMap.cpp \
		osmscout/util/Number.cpp \
		osmscout/util/NumberSet.cpp \
		osmscout/util/Parser.cpp \
		osmscout/util/Progress.cpp \
		osmscout/util/Projection.cpp \
		osmscout/util/Reference.cpp \
		osmscout/util/StopClock.cpp \
		osmscout/util/String.cpp \
		osmscout/util/Transformation.cpp \
		osmscout/Types.cpp \
		osmscout/TypeConfig.cpp \
		osmscout/TypeSet.cpp \
		osmscout/ost/Scanner.cpp \
		osmscout/ost/Parser.cpp \
		osmscout/TypeConfigLoader.cpp \
		osmscout/GroundTile.cpp \
		osmscout/Intersection.cpp \
		osmscout/Location.cpp \
		osmscout/Coord.cpp \
		osmscout/GeoCoord.cpp \
		osmscout/Pixel.cpp \
		osmscout/Area.cpp \
		osmscout/Node.cpp \
		osmscout/Path.cpp \
		osmscout/Point.cpp \
		osmscout/Tag.cpp \
		osmscout/AttributeAccess.cpp \
		osmscout/TurnRestriction.cpp \
		osmscout/Way.cpp \
		osmscout/ObjectRef.cpp \
		osmscout/NumericIndex.cpp \
		osmscout/CoordDataFile.cpp \
		osmscout/AreaAreaIndex.cpp \
		osmscout/AreaNodeIndex.cpp \
		osmscout/AreaWayIndex.cpp \
		osmscout/LocationIndex.cpp \
		osmscout/OptimizeAreasLowZoom.cpp \
		osmscout/OptimizeWaysLowZoom.cpp \
		osmscout/WaterIndex.cpp \
		osmscout/Route.cpp \
		osmscout/RouteData.cpp \
		osmscout/RouteNode.cpp \
		osmscout/RoutePostprocessor.cpp \
		osmscout/RoutingProfile.cpp \
		osmscout/Database.cpp \
		osmscout/DebugDatabase.cpp \
		osmscout/SRTM.cpp \
		osmscout/LocationService.cpp \
		osmscout/POIService.cpp \
		osmscout/MapService.cpp \
		osmscout/RoutingService.cpp \
		osmscout/TextSearchIndex.cpp \
		osmscout/util/utf8.cpp
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE    := osmscout_jni
LOCAL_C_INCLUDES := ../config include
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 \
	-Wno-cast-qual -fno-strict-aliasing -D__ANDROID__ \
	-std=c++11 -Wno-sign-compare -Wno-psabi -frtti  -fexceptions

LOCAL_LDLIBS := -lm -llog  -lstdc++

#LOCAL_LDFLAGS += -fopenmp

LOCAL_ARM_MODE  := arm

LOCAL_SRC_FILES := osmscout_wrap.cxx
LOCAL_STATIC_LIBRARIES := osmscout

include $(BUILD_SHARED_LIBRARY)
