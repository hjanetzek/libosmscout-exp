#ifndef OSMSCOUT_MYVISIT_H
#define OSMSCOUT_MYVISIT_H

#include "osmscout/Location.h"

namespace osmscout{
  class MyVisitor : public AdminRegionVisitor{
  private:
    AdminRegionRef *curRegion;

  public:
    MyVisitor(){
    }

    ~MyVisitor(){
    }
    
    void setAdminRegion(AdminRegionRef *ref){
      curRegion = ref;
    }
    
    virtual Action Visit() = 0;

    Action Visit(const AdminRegion& region){
      *curRegion = new AdminRegion(region);
      return Visit();
    }
    
    //virtual ~MyVisitor();

  };
}
#endif
