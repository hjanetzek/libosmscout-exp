// useful links:
// http://www.swig.org/Doc2.0/SWIGDocumentation.html
// http://stackoverflow.com/questions/10126531/swig-and-c-shared-library
// http://sourceforge.net/p/swig/patches/259/ (std::list)

// swig -v  -DOSMSCOUT_API -c++  -outdir out/org/osmscout -package org.osmscout -java test.i
// g++  -std=c++11 -shared -fPIC -DOSMSCOUT_API= -DSWIG=1 -c test_wrap.cxx -I/usr/lib/jvm/java-7-openjdk-amd64/include/  -I/usr/lib/jvm/java-7-openjdk-amd64/include/linux ../libosmscout/include/osmscout
// g++  -shared -Wl,--no-undefined test_wrap.o  -o libtest.so -losmscout

%module(directors="1") osmscout

%{
  /* Includes the header in the wrapper code */
#include "osmscout/util/Reference.h"
#include "osmscout/util/Breaker.h"
#include "osmscout/util/Magnification.h"
#include "osmscout/util/Projection.h"
#include "osmscout/util/Parser.h"
#include "osmscout/TypeConfig.h"
#include "osmscout/Types.h"
#include "osmscout/TypeSet.h"
#include "osmscout/Coord.h"
#include "osmscout/GeoCoord.h"
#include "osmscout/Tag.h"
#include "osmscout/Node.h"
#include "osmscout/Way.h"
#include "osmscout/Area.h"
#include "osmscout/GroundTile.h"
#include "osmscout/AttributeAccess.h"
#include "osmscout/Database.h"
#include "osmscout/MapService.h"
#include "osmscout/Location.h"
#include "osmscout/LocationService.h"
#include "osmscout/POIService.h"

#include "osmscout/Point.h"
#include "osmscout/Route.h"
#include "osmscout/RouteNode.h"
#include "osmscout/RouteData.h"
#include "osmscout/RoutingProfile.h"
#include "osmscout/RoutingService.h"
#include "osmscout/RoutePostprocessor.h"
#include "osmscout/util/Breaker.h"
#include "osmscout/TypeConfigLoader.h"
%}

/* might be needed out of source */
//%include "inttypes.i"

// FIXME// FIXME ?
//%import "osmscout/system/Types.h"
typedef int size_t;
typedef unsigned char uint8_t;
typedef unsigned char int8_t;
typedef int uint16_t;
typedef int uint32_t;

// knows about things like int *OUTPUT:
%include "typemaps.i"
%include "cpointer.i"

//%include "stl.i"
%include "std_common.i"
//%include "std_vector.i"
%include "std_string.i"
%include "std_map.i"

%include "vector.i"
%include "list.i"

/* Wrap a class interface around an "int *" */
//%pointer_class(int, intp);
//%pointer_class(double, doublep);

/* Force the generated Java code to use the C constant values rather than making a JNI call */
%javaconst(1);

/* Renme operators */
//%rename (opAdd) *::operator+=;
%rename (assign) *::operator=;
%rename (equals) *::operator==;
//%rename (opLEqualThan) *::operator<=;
//%rename (opGEqualThan) *::operator>=;
//%rename (lessThan) *::operator<;
//%rename (greaterThan) *::operator>;
%rename (subtract) *::operator-;
//%rename (notEquals) *::operator!=;
%rename (add) *::operator+;
//%rename (opSubtract) *::operator-=;

%ignore *::operator!=;
%ignore *::operator>;
%ignore *::operator<;
%ignore *::operator<=;
%ingore *::operator>=;
%ignore *::operator-=;
%ignore *::operator+=;
%ignore *::operator&;
%ignore *::operator*;
%ignore *::operator();
%ignore *::operator osmscout::Breaker*;
%ignore *::operator osmscout::Breaker&;
//... etc ignored anyway

//%rename(pointer) *::operator->;

%ignore *::operator*;

//%ignore *::operator->;

/* Ignore API functions used for database creation */
%ignore osmscout::AreaAttributes::SetTags(Progress& progress, const TypeConfig& typeConfig, std::vector<Tag>& tags);
%ignore osmscout::Node::SetTags(Progress& progress, const TypeConfig& typeConfig, std::vector<Tag>& tags);
%ignore osmscout::Way::SetTags(Progress& progress, const TypeConfig& typeConfig, std::vector<Tag>& tags);

%ignore osmscout::Way::GetCoordinates(size_t nodeIndex, double& lat, double& lon) const;
%ignore osmscout::RouteData::RouteEntry::SetObjects(const std::vector<ObjectFileRef> objects);
%ignore osmscout::RouteData::Entries();

%nodefaultctor osmscout::RouteData::RouteEntry;
%nodefaultdtor osmscout::RouteData::RouteEntry;

%ignore osmscout::RouteData::RouteEntry(Id currentNodeId,
					size_t currentNodeIndex,
					const ObjectFileRef& pathObject,
					size_t targetNodeIndex);

/* can be accessed by getTarget/SourceDescription() - avoiding one list wrapper */
%ignore osmscout::RouteDescription::CrossingWaysDescription::GetDescriptions() const;
%ignore osmscout::RouteDescription::Node::GetDescriptions() const;

//%ignore osmscout::LocationSearchResult::results;

// OVERRIDDEN
%ignore osmscout::LocationSearchResult::Entry::adminRegion;
%ignore osmscout::LocationSearchResult::Entry::location;
%ignore osmscout::LocationSearchResult::Entry::poi;
%ignore osmscout::LocationSearchResult::Entry::address;

//Remove automatically created accessors for public fields
%ignore osmscout::Area::type;
%ignore osmscout::Area::attributes;
%ignore osmscout::Area::Ring::type;
%ignore osmscout::Area::Ring::attributes;
%ignore osmscout::Area::Ring::SetType(const TypeId& type);
%ignore osmscout::AreaAttributes::name;
%ignore osmscout::GeoCoord::lat;
%ignore osmscout::GeoCoord::lon;
%ignore osmscout::WayAttributes::type;
%ignore osmscout::WayAttributes::name;
%ignore osmscout::ObjectFileRef::type;
%ignore osmscout::ObjectOSMRef::id;
%ignore osmscout::ObjectOSMRef::type;
%ignore osmscout::RouteNode::id;
%ignore osmscout::RouteNodeRef::id;
%ignore osmscout::Point::id;
%ignore osmscout::Point::coords;

%ignore osmscout::RoutingService::FILENAME_INTERSECTIONS_DAT;
%ignore osmscout::RoutingService::FILENAME_INTERSECTIONS_IDX;
%ignore osmscout::RoutingService::FILENAME_FOOT_DAT;
%ignore osmscout::RoutingService::FILENAME_FOOT_IDX;
%ignore osmscout::RoutingService::FILENAME_BICYCLE_DAT;
%ignore osmscout::RoutingService::FILENAME_BICYCLE_IDX;
%ignore osmscout::RoutingService::FILENAME_CAR_DAT;
%ignore osmscout::RoutingService::FILENAME_CAR_IDX;

%ignore *::dataOffset;
%ignore *::regionOffset;
%ignore *::addressOffset;
%ignore *::locationOffset;
%ignore *::parentRegionOffset;

/* Dont create setter */
%immutable *::object;

/* Create std::list<T> classes and iterators that implement java Iterator<T> */
%define STD_LIST_WRAP(NAME, CLAZZ, T...)
%newobject std::list<T>::iterator() const;

%typemap(javainterfaces) Iterator_t<T> "java.util.Iterator<CLAZZ>"
%template( ## NAME ## Iterator) Iterator_t<T>;

%typemap(javainterfaces) std::list<T> "Iterable<CLAZZ>"
%template( ## NAME ## List) std::list<T>;
%enddef

STD_LIST_WRAP(LocationSearchResultEntry, LocationSearchResult.Entry, osmscout::LocationSearchResult::Entry)
STD_LIST_WRAP(LocationSearchEntry, LocationSearch.Entry, osmscout::LocationSearch::Entry)
STD_LIST_WRAP(GroundTile, GroundTile, osmscout::GroundTile)
STD_LIST_WRAP(ReverseLookupResult, LocationService.ReverseLookupResult, osmscout::LocationService::ReverseLookupResult)
STD_LIST_WRAP(AddressResult, AddressListVisitor.AddressResult, osmscout::AddressListVisitor::AddressResult)
 //STD_LIST_WRAP(TypeCondition, TypeInfo.TypeCondition, osmscout::TypeInfo::TypeCondition)
STD_LIST_WRAP(Point, Point, osmscout::Point)
STD_LIST_WRAP(RouteEntry, RouteData.RouteEntry, osmscout::RouteData::RouteEntry)
STD_LIST_WRAP(RouteDescription, RouteDescription.Node, osmscout::RouteDescription::Node)
STD_LIST_WRAP(RoutePostprocessor, RoutePostprocessorRef, osmscout::Ref<osmscout::RoutePostprocessor::Postprocessor>)
STD_LIST_WRAP(Referencable, Referencable, osmscout::Referencable)

/* Modify function names to java conventions */
%rename("%(lowercamelcase)s", %$isfunction) "";

%include "enums.swg"

%include "osmscout/Types.h"

 /* Additional class functions */
%include "ext_location_search.i"
%include "ext_route_description.i"

// FIXME - not sure if needed or harmful
//%refobject   osmscout::Referencable "$this->AddReference();"
//%unrefobject osmscout::Referencable "$this->RemoveReference();"

//%typemap(javabase) SWIGTYPE, SWIGTYPE *, SWIGTYPE &, SWIGTYPE [], SWIGTYPE (CLASS::*) "Referencable"


/*** Parse the header file to generate wrappers ***/
%include "osmscout/util/Reference.h"


/* 'break' is Java keyword */
%rename(abortExecution) osmscout::Breaker::Break;
%include "osmscout/util/Breaker.h"

%include "osmscout/util/Magnification.h"
%include "osmscout/util/Parser.h"
%include "osmscout/Tag.h"

/*** TypeConfig.h */
/*
%ignore osmscout::TypeConfig::ResolveTags(const std::map<TagId,std::string>& map, std::vector<Tag>& tags) const;
%ignore osmscout::TypeConfig::IsNameTag(TagId tag, uint32_t& priority) const
%ignore osmscout::TypeConfig::IsNameAltTag(TagId tag, uint32_t& priority) const
%ignore osmscout::TypeConfig::GetNodeTypeId(const std::map<TagId,std::string>& tagMap, TypeId &typeId) const
%apply int *OUTPUT { short&  typeId };
%apply int *OUTPUT { short&  wayType };
%apply int *OUTPUT { short&  areaType };
*/
%ignore osmscout::TagCondition;
%ignore osmscout::TypeInfo::TypeCondition;
%ignore osmscout::TypeInfo::GetConditions() const;
%ignore osmscout::TypeInfo::AddCondition(unsigned char types, TagCondition* condition);
%include "osmscout/TypeConfig.h"

//%ignore osmscout::TypeSet::TypeSet(const TypeConfig& typeConfig);
%include "osmscout/TypeSet.h"
/* %extend osmscout::TypeSet{
 *   static osmscout::TypeSet *reateTypeSet(int maxTypeId) {
 *     osmscout::TypeSet *t = new osmscout::TypeSet();
 *     t->types.resize(maxTypeId+1,false);
 *     return t;
 *   }
 * } */

%include "osmscout/GeoCoord.h"
%include "osmscout/AttributeAccess.h"
%include "osmscout/ObjectRef.h"

%apply double *OUTPUT { double& };

%apply int *OUTPUT { int& start };

%include "osmscout/util/Projection.h"


// Way getNodeIndexByNodeId
%apply int *OUTPUT { int& index };
%include "osmscout/Way.h"

 /* %extend osmscout::Way {
 *   long getNumNodes() {
 *     return $self->nodes.size();
 *   }
 *   void copyNodes(double nodes[]){
 *     for (int i = 0, n = $self->nodes.size(); i < n; i++){
 *       nodes[i*2+0] =  $self->nodes[i].lat;
 *       nodes[i*2+1] =  $self->nodes[i].lon;
 *     }
 *   }
 * }; */

%include "osmscout/Node.h"
%extend osmscout::Node {
    size_t GetTagCount() const
    {
      return $self->GetAttributes().GetTags().size();
    }

    TagId GetTagKey(size_t idx) const
    {
      return $self->GetAttributes().GetTags()[idx].key;
    }

    const std::string& GetTagValue(size_t idx) const
    {
      return $self->GetAttributes().GetTags()[idx].value;
    }
}

%ignore osmscout::Area::rings;
%include "osmscout/Area.h"
%extend osmscout::Area {
    size_t GetRingCount() const
    {
      return $self->rings.size();
    }

    const Ring *GetRing(int i) const
    {
      return &($self->rings[i]);
    }
}
%extend osmscout::Area::Ring {
    size_t GetTagCount() const
    {
      return $self->GetAttributes().GetTags().size();
    }

    TagId GetTagKey(size_t idx) const
    {
      return $self->GetAttributes().GetTags()[idx].key;
    }

    const std::string& GetTagValue(size_t idx) const
    {
      return $self->GetAttributes().GetTags()[idx].value;
    }
}

%include "osmscout/GroundTile.h"

%include "osmscout/Database.h"
%extend osmscout::Database {
  static osmscout::Ref<osmscout::Database> createDatabase(osmscout::DatabaseParameter param){
    return new osmscout::Database(param);
  }
}

%include "osmscout/MapService.h"

 //%feature("director") osmscout::AdminRegionVisitor;
%feature("director") osmscout::AddressVisitor;
%feature("director") osmscout::LocationVisitor;
%include "osmscout/Location.h"

%{
#include "Visitors.h"
%}
%feature("director") osmscout::MyVisitor;
//%ignore  osmscout::MyVisitor::Visit(const AdminRegion& region);
%include "Visitors.h"


%ignore osmscout::LocationService::ReverseLookupObjects(const std::list<ObjectFileRef>& objects,
							std::list<ReverseLookupResult>& result) const;
%include "osmscout/LocationService.h"


%include "osmscout/POIService.h"

%include "osmscout/Point.h"

/*** Routing ***/
%include "osmscout/Route.h"

%ignore osmscout::RouteNode::Path;
%ignore osmscout::RouteNode::Exclude;
%ignore osmscout::RouteNode::paths;
%ignore osmscout::RouteNode::excludes;
%include "osmscout/RouteNode.h"
%include "osmscout/RouteData.h"
%include "osmscout/RoutingProfile.h"

/// RoutePostProcessor
//%ignore *::Postprocessor;
%ignore *::StartPostprocessor;
%ignore *::TargetPostprocessor;
%ignore *::DirectionPostprocessor;
%ignore *::DistanceAndTimePostprocessor;
%ignore *::WayNamePostprocessor;
%ignore *::CrossingWaysPostprocessor;

%include "osmscout/RoutePostprocessor.h"

%template(RoutePostprocessorRef) osmscout::Ref<osmscout::RoutePostprocessor::Postprocessor>;

%extend osmscout::RoutePostprocessor {
  static osmscout::Ref<osmscout::RoutePostprocessor::Postprocessor> createRouteStartProcessor(std::string startDescription){
    return new osmscout::RoutePostprocessor::StartPostprocessor(startDescription);
  }

  static osmscout::Ref<osmscout::RoutePostprocessor::Postprocessor> createRouteTargetProcessor(std::string endDescription){
    return new osmscout::RoutePostprocessor::TargetPostprocessor(endDescription);
  }

  static osmscout::Ref<osmscout::RoutePostprocessor::Postprocessor> createRouteDirectionProcessor(){
    return new osmscout::RoutePostprocessor::DirectionPostprocessor();
  }

  static osmscout::Ref<osmscout::RoutePostprocessor::Postprocessor> createRouteDistanceAndTimeProcessor(){
    return new osmscout::RoutePostprocessor::DistanceAndTimePostprocessor();
  }

  static osmscout::Ref<osmscout::RoutePostprocessor::Postprocessor> createRouteWayNameProcessor(){
    return new osmscout::RoutePostprocessor::WayNamePostprocessor();
  }

  static osmscout::Ref<osmscout::RoutePostprocessor::Postprocessor> createRouteCrossingWaysProcessor(){
    return new osmscout::RoutePostprocessor::CrossingWaysPostprocessor();
  }
}


/// RoutingService
%apply int *OUTPUT { int& nodeIndex };
%include "osmscout/RoutingService.h"

%include "osmscout/TypeConfigLoader.h"
%include "osmscout/Coord.h"


%template(BreakerRef) osmscout::Ref<osmscout::Breaker>;
%template(NodeRef) osmscout::Ref<osmscout::Node>;
%template(WayRef) osmscout::Ref<osmscout::Way>;
%template(AreaRef) osmscout::Ref<osmscout::Area>;
%template(DatabaseRef) osmscout::Ref<osmscout::Database>;
%template(TypeConfigRef) osmscout::Ref<osmscout::TypeConfig>;
//%template(TagConditionRef) osmscout::Ref<osmscout::TagCondition>;
%template(RouteNameDescriptionRef) osmscout::Ref<osmscout::RouteDescription::NameDescription>;

%template(LocationRef) osmscout::Ref<osmscout::Location>;
%template(AdminRegionRef) osmscout::Ref<osmscout::AdminRegion>;
%template(POIRef) osmscout::Ref<osmscout::POI>;
%template(AddressRef) osmscout::Ref<osmscout::Address>;
%template(RouteNodeRef) osmscout::Ref<osmscout::RouteNode>;

/* Re-use Ref<T> instances when iterating vectors */
%define REF_VECTOR(NAME, CLAZZ)
%extend std::vector< ## CLAZZ ## > {
       void get(int i,  ## CLAZZ ## & ref) throw (std::out_of_range) {
	 int size = int(self->size());
	 if (i>=0 && i<size)
	   ref = (*self)[i];
	 else
	   throw std::out_of_range("vector index out of range");
       }
}
%template( ## NAME ##) std::vector< ## CLAZZ ## >;
%enddef

REF_VECTOR(Nodes, osmscout::NodeRef)
REF_VECTOR(Ways, osmscout::WayRef)
REF_VECTOR(Areas, osmscout::AreaRef)


%template(Tags) std::vector<osmscout::Tag>;
%template(TypeSets) std::vector<osmscout::TypeSet>;
%template(GeoCoords) std::vector<osmscout::GeoCoord>;
%template(TagInfos) std::vector<osmscout::TagInfo>;
%template(TypeInfos) std::vector<osmscout::TypeInfo>;
%template(GroundTileCoords) std::vector<osmscout::GroundTile::Coord>;

%template(TagMap) std::map<unsigned short, std::string>;

/* Used by Database get*ByOffset() and TextSearchResults */
%template(ObjectFileRefs)  std::vector<osmscout::ObjectFileRef>;


%{
#include "osmscout/Pixel.h"
#include "osmscout/util/Transformation.h"
#include "Transformer.h"
%}

// TransBuffer, Transformer
%apply int *OUTPUT { int& start };
%apply int *OUTPUT { int& end };

%include "osmscout/Pixel.h"
%ignore osmscout::CoordBuffer;
%ignore osmscout::CoordBufferImpl;
%include "osmscout/util/Transformation.h"


%include "array_nocpy.i"
%include "Transformer.h"


%{
#include "osmscout/TextSearchIndex.h"
#include "osmscout/util/HashMap.h"
%}
%import "osmscout/util/HashMap.h"
%include "osmscout/TextSearchIndex.h"

// from http://stackoverflow.com/questions/9465856/no-iterator-for-java-when-using-swig-with-cs-stdmap
%typemap(javainterfaces) MapIterator "java.util.Iterator<String>"
%typemap(javacode) MapIterator %{
  public void remove() throws UnsupportedOperationException {
    throw new UnsupportedOperationException();
  }

  public String next() throws java.util.NoSuchElementException {
    if (!hasNext()) {
      throw new java.util.NoSuchElementException();
    }

    return nextImpl();
  }
%}

%javamethodmodifiers MapIterator::nextImpl "private";
%inline %{
  struct MapIterator {
    typedef std::map<std::string,std::vector<osmscout::ObjectFileRef>> map_t;
    MapIterator(const map_t& m) : it(m.begin()), map(m) {}
    bool hasNext() const {
      return it != map.end();
    }

    const std::string& nextImpl() {
      //const std::pair<int,std::string>& ret = *it++;
      //return ret.second;
      return (*it++).first;
    }
  private:
    map_t::const_iterator it;
    const map_t& map;
  };
%}

%typemap(javainterfaces) std::map<std::string, std::vector<osmscout::ObjectFileRef>> "Iterable<String>"

%newobject std::map<std::string,  std::vector<osmscout::ObjectFileRef>>::iterator() const;
%extend std::map<std::string, std::vector<osmscout::ObjectFileRef>> {
  MapIterator *iterator() const {
    return new MapIterator(*$self);
  }
}

%template(TextSearchResults) std::map<std::string, std::vector<osmscout::ObjectFileRef>>;
