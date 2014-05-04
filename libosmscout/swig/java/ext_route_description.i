%extend osmscout::RouteDescription::Node {
 NameDescription* getNameDescription(){
   return dynamic_cast<osmscout::RouteDescription::NameDescription*>($self->GetDescription(osmscout::RouteDescription::WAY_NAME_DESC));
  }
 StartDescription* getStartDescription(){
   return dynamic_cast<osmscout::RouteDescription::StartDescription*>($self->GetDescription(osmscout::RouteDescription::NODE_START_DESC));
  }
 TargetDescription* getTargetDescription(){
   return dynamic_cast<osmscout::RouteDescription::TargetDescription*>($self->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC));
  }
 NameChangedDescription* getNameChangedDescription(){
   return dynamic_cast<osmscout::RouteDescription::NameChangedDescription*>($self->GetDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC));
  }
 CrossingWaysDescription* getCrossingWaysDescription(){
   return dynamic_cast<osmscout::RouteDescription::CrossingWaysDescription*>($self->GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC));
  }
 DirectionDescription* getDirectionDescription(){
   return dynamic_cast<osmscout::RouteDescription::DirectionDescription*>($self->GetDescription(osmscout::RouteDescription::DIRECTION_DESC));
  }
 TurnDescription* getTurnDescription(){
   return dynamic_cast<osmscout::RouteDescription::TurnDescription*>($self->GetDescription(osmscout::RouteDescription::TURN_DESC));
  }
 RoundaboutEnterDescription* getRoundaboutEnterDescription(){
   return dynamic_cast<osmscout::RouteDescription::RoundaboutEnterDescription*>($self->GetDescription(osmscout::RouteDescription::ROUNDABOUT_ENTER_DESC));
  }
 RoundaboutLeaveDescription* getRoundaboutLeaveDescription(){
   return dynamic_cast<osmscout::RouteDescription::RoundaboutLeaveDescription*>($self->GetDescription(osmscout::RouteDescription::ROUNDABOUT_LEAVE_DESC));
  }
 MotorwayEnterDescription* getMotorwayEnterDescription(){
   return dynamic_cast<osmscout::RouteDescription::MotorwayEnterDescription*>($self->GetDescription(osmscout::RouteDescription::MOTORWAY_ENTER_DESC));
  }
 MotorwayChangeDescription* getMotorwayChangeDescription(){
   return dynamic_cast<osmscout::RouteDescription::MotorwayChangeDescription*>($self->GetDescription(osmscout::RouteDescription::MOTORWAY_CHANGE_DESC));
  }
 MotorwayLeaveDescription* getMotorwayLeaveDescription(){
   return dynamic_cast<osmscout::RouteDescription::MotorwayLeaveDescription*>($self->GetDescription(osmscout::RouteDescription::MOTORWAY_LEAVE_DESC));
  }
}
