bin_PROGRAMS = LocationLookup \
               ReverseLocationLookup \
               Routing \
               LookupPOI \
               LookupText \
               Srtm


if OSMSCOUT_HAVE_LIB_MARISA
bin_PROGRAMS += LookupText
endif

LocationLookup_SOURCES = LocationLookup.cpp
LocationLookup_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
LocationLookup_LDADD = $(LIBOSMSCOUT_LIBS)

ReverseLocationLookup_SOURCES = ReverseLocationLookup.cpp
ReverseLocationLookup_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
ReverseLocationLookup_LDADD = $(LIBOSMSCOUT_LIBS)

LookupPOI_SOURCES = LookupPOI.cpp
LookupPOI_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
LookupPOI_LDADD = $(LIBOSMSCOUT_LIBS)

Routing_SOURCES = Routing.cpp
Routing_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
Routing_LDADD = $(LIBOSMSCOUT_LIBS)

Srtm_SOURCES = Srtm.cpp
Srtm_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
Srtm_LDADD = $(LIBOSMSCOUT_LIBS)

LookupText_SOURCES = LookupText.cpp
LookupText_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS) $(MARISA_CFLAGS)
LookupText_LDADD = $(LIBOSMSCOUT_LIBS) $(MARISA_LIBS)
