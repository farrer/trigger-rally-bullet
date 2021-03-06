
// psim.cpp [psim]

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)

#include "psim.h"



PSim::PSim() : terrain(nullptr), gravity(vec3f::zero())
{
}


PSim::~PSim()
{
  clear();
}



PVehicleType *PSim::loadVehicleType(const std::string &filename, PSSModel &ssModel)
{
  PVehicleType *vtype = vtypelist.find(filename);
  if (!vtype) {
    vtype = new PVehicleType();
    if (!vtype->load(filename, ssModel)) {
      if (PUtil::isDebugLevel(DEBUGLEVEL_ENDUSER))
        PUtil::outLog() << "Failed to load " << filename << "\n";
      return nullptr;
    }
    vtypelist.add(vtype);
  }
  return vtype;
}

PVehicle *PSim::createVehicle(XMLElement *element, const std::string &filepath, PSSModel &ssModel)
{
  const char *val;

  const char *type = element->Attribute("type");
  if (!type) {
    PUtil::outLog() << "Vehicle has no type\n";
    return nullptr;
  }

  vec3f pos = vec3f::zero();
  quatf ori = QUATERNION_IDENTITY;

  val = element->Attribute("pos");
  if (val) sscanf(val, "%f , %f , %f", &pos.x, &pos.y, &pos.z);

  //XXX if you look at vehicles' files, the attribute is called orientation,
  //not ori. but this function seems to never be called. Shouldn't we remove
  //those never used functions?
  val = element->Attribute("ori");
  if (val) {
    float w, x, y, z;
    sscanf(val, "%f , %f , %f , %f", &w, &x, &y, &z);
    ori = quatf(x, y, z, w);
  }

  return createVehicle(type, pos, ori, filepath, ssModel);
}

PVehicle *PSim::createVehicle(const std::string &type, const vec3f &pos, const quatf &ori, const std::string &filepath, PSSModel &ssModel)
{
  PVehicleType *vtype = loadVehicleType(PUtil::assemblePath(type, filepath), ssModel);

  return createVehicle(vtype, pos, ori, ssModel);
}

PVehicle *PSim::createVehicle(PVehicleType *type, const vec3f &pos, const quatf &ori, PSSModel &ssModel)
{
  PSSModel *unused = &ssModel; unused = unused;

  if (!type) return nullptr;

  PVehicle *newvehicle = new PVehicle(*this, type);

  vec3f vpos = pos;
  if (terrain) vpos.z += terrain->getHeight(vpos.x, vpos.y);
  
  newvehicle->setPositionAndOrientation(vpos, ori);

  newvehicle->updateParts();

  vehicle.push_back(newvehicle);
  return newvehicle;
}

void PSim::clear()
{
  for (unsigned int i=0; i<vehicle.size(); ++i)
    delete vehicle[i];
  vehicle.clear();

  vtypelist.clear();
}


void PSim::tick(float delta)
{
  if (delta <= 0.0) return;

  //lta *= 0.1;

  float timeslice = 0.005;
  int num = (int)(delta / timeslice) + 1;
  timeslice = delta / (float)num;

  for (int timestep=0; timestep<num; ++timestep) {
    for (unsigned int i=0; i<vehicle.size(); ++i) {
      vehicle[i]->tick(timeslice);
    }

    for (unsigned int i=0; i<vehicle.size(); ++i) {
      vehicle[i]->updateParts();
    }
  }
}



