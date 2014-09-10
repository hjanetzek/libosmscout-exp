%extend osmscout::LocationSearchResult::Entry {
  // TODO provide int flags for which items are available
   AdminRegion* getAdminRegion(){
    return $self->adminRegion.Valid() ? $self->adminRegion.Get() : 0;
  }
   Location* getLocation(){
    return $self->location.Valid() ? $self->location.Get() : 0;
  }
   Address* getAddress(){
    return $self->address.Valid() ? $self->address.Get() : 0;
  }
   POI* getPoi(){
    return $self->poi.Valid() ? $self->poi.Get() : 0;
  }
}
