
// vehicle.cpp

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)

#include "psim.h"
#include "bulletlink.h"


// PDriveSystem and PDriveSystemInstance //


float PDriveSystem::getPowerAtRPS(float rps)
{
  unsigned int p;
  float power;
  
  // find which curve points rps lies between
  for (p = 0; p < powercurve.size() && powercurve[p].x < rps; p++);
  
  if (p == 0) {
    // to the left of the graph
    power = powercurve[0].y * (rps / powercurve[0].x);
  } else if (p < powercurve.size()) {
    // on the graph
    power = powercurve[p-1].y + (powercurve[p].y - powercurve[p-1].y) *
      ( (rps - powercurve[p-1].x) / (powercurve[p].x - powercurve[p-1].x) );
  } else {
    // to the right of the graph
    power = powercurve[p-1].y + (0.0f - powercurve[p-1].y) *
      ( (rps - powercurve[p-1].x) / (powercurve.back().x - powercurve[p-1].x) );
  }
  
  return power;
}

void PDriveSystemInstance::tick(float delta, float throttle, float wheel_rps)
{
  rps = wheel_rps * dsys->gear[currentgear].y;
  
  bool wasreverse = reverse;
  reverse = (throttle < 0.0f);
  
  if (wasreverse != reverse) flag_gearchange = true;
  
  if (reverse) {
    rps *= -1.0f;
    throttle *= -1.0f;
  }
  
  CLAMP_UPPER(throttle, 1.0f);
  CLAMP(rps, dsys->minRPS, dsys->maxRPS);
  
  if (reverse) {
    currentgear = 0;
  }
  
  out_torque = dsys->getPowerAtRPS(rps) * dsys->gear[currentgear].y / rps;
  
  if (!reverse) {
    int newtarget_rel = 0;
    
    if (currentgear < (int)dsys->gear.size()-1) {
      float nextrate = rps / dsys->gear[currentgear].y * dsys->gear[currentgear+1].y;
      CLAMP(nextrate, dsys->minRPS, dsys->maxRPS);
      float nexttorque = dsys->getPowerAtRPS(nextrate) * dsys->gear[currentgear+1].y / nextrate;
      if (nexttorque > out_torque)
        newtarget_rel = 1;
    }

    // don't test for down if already decided to go up
    if (currentgear > 0 && newtarget_rel == 0) {
      float nextrate = rps / dsys->gear[currentgear].y * dsys->gear[currentgear-1].y;
      CLAMP(nextrate, dsys->minRPS, dsys->maxRPS);
      float nexttorque = dsys->getPowerAtRPS(nextrate) * dsys->gear[currentgear-1].y / nextrate;
      if (nexttorque > out_torque)
        newtarget_rel = -1;
    }
    
    if (newtarget_rel != 0 && newtarget_rel == targetgear_rel) {
      if ((gearch -= delta) <= 0.0f)
      {
        float nextrate = rps / dsys->gear[currentgear].y * dsys->gear[currentgear + targetgear_rel].y;
        CLAMP(nextrate, dsys->minRPS, dsys->maxRPS);
        out_torque = dsys->getPowerAtRPS(nextrate) * dsys->gear[currentgear + targetgear_rel].y / nextrate;
        currentgear += targetgear_rel;
        gearch = dsys->gearch_repeat;
        flag_gearchange = true;
      }
    } else {
      gearch = dsys->gearch_first;
      targetgear_rel = newtarget_rel;
    }
  }
  
  out_torque *= throttle;
  
  if (reverse) {
    out_torque *= -1.0;
  }
  
  out_torque -= wheel_rps * 0.1f;
}


// PVehicleType //


bool PVehicleType::load(const std::string &filename, PSSModel &ssModel)
{
  if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
    PUtil::outLog() << "Loading vehicle type \"" << filename << "\"\n";

  name = filename;

  unload();

  // defaults

  proper_name = "Vehicle";
  proper_class = "Unknown";
  
    pstat_weightkg = "N/A";
    pstat_enginebhp = "N/A";
    pstat_wheeldrive = "N/A";
    pstat_handling = "N/A";
  
  mass = 1.0;
  dims = vec3f(1.0,1.0,1.0);
  
  wheelscale = 1.0;
  wheelmodel = nullptr;
  
  ctrlrate.setDefaultRates();
  
  param.speed = 0.0;
  param.turnspeed = vec3f::zero();
  param.turnspeed_a = 1.0;
  param.turnspeed_b = 0.0;
  param.drag = vec3f::zero();
  param.angdrag = 0.0;
  param.lift = vec2f::zero();
  param.fineffect = vec2f::zero();
  
  float allscale = 1.0;
  
  float drive_total = 0.0f;
  
  wheel_speed_multiplier = 0.0f;
  
  //
  
  XMLDocument xmlfile;
  XMLElement *rootelem = PUtil::loadRootElement(xmlfile, filename, "vehicle");
  if (!rootelem) {
    PUtil::outLog() << "Load failed: TinyXML error\n";
    return false;
  }
  
  const char *val;
  
  val = rootelem->Attribute("name");
  if (val) proper_name = val;
  
  val = rootelem->Attribute("class");
  
  if (val != nullptr)
    proper_class = val;
  
  val = rootelem->Attribute("allscale");
  if (val) allscale = atof(val);
  
  val = rootelem->Attribute("type");
  if (!val || !strlen(val)) {
    if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
      PUtil::outLog() << "Warning: <vehicle> element without type attribute\n";
    return false;
  }

  // VEHICLE TYPE POINT

  if (false) ;
  else if (!strcmp(val, "tank")) coretype = VCTYPE_TANK;
  else if (!strcmp(val, "helicopter")) coretype = VCTYPE_HELICOPTER;
  else if (!strcmp(val, "plane")) coretype = VCTYPE_PLANE;
  else if (!strcmp(val, "hovercraft")) coretype = VCTYPE_HOVERCRAFT;
  else if (!strcmp(val, "car")) coretype = VCTYPE_CAR;
  else {
    if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
      PUtil::outLog() << "Error: <vehicle> has unrecognised type \"" << val << "\"\n";
    return false;
  }

  for (XMLElement *walk = rootelem->FirstChildElement();
    walk; walk = walk->NextSiblingElement()) {

    if (false) {
    } else if (!strcmp(walk->Value(), "pstats")) {
        val = walk->Attribute("weightkg");
        if (val != nullptr) pstat_weightkg = val;

        val = walk->Attribute("enginebhp");
        if (val != nullptr) pstat_enginebhp = val;

        val = walk->Attribute("wheeldrive");
        if (val != nullptr) pstat_wheeldrive = val;

        val = walk->Attribute("handling");
        if (val != nullptr) pstat_handling = val;
    } else if (!strcmp(walk->Value(), "genparams")) {

      val = walk->Attribute("mass");
      if (val) mass = atof(val);

      val = walk->Attribute("dimensions");
      if (val) {
        sscanf(val, "%f , %f , %f", &dims.x, &dims.y, &dims.z);
        dims *= allscale;
      }
      
      val = walk->Attribute("wheelscale");
      if (val) wheelscale = atof(val);
      
      val = walk->Attribute("wheelmodel");
      if (val) wheelmodel = ssModel.loadModel(PUtil::assemblePath(val, filename));
      
    } else if (!strcmp(walk->Value(), "ctrlparams")) {

      val = walk->Attribute("speed");
      if (val) param.speed = atof(val);

      val = walk->Attribute("turnspeed");
      if (val) sscanf(val, "%f , %f , %f", &param.turnspeed.x, &param.turnspeed.y, &param.turnspeed.z);

      val = walk->Attribute("drag");
      if (val) sscanf(val, "%f , %f , %f", &param.drag.x, &param.drag.y, &param.drag.z);

      val = walk->Attribute("angdrag");
      if (val) param.angdrag = atof(val);

      val = walk->Attribute("lift");
      if (val) sscanf(val, "%f , %f", &param.lift.x, &param.lift.y);


      val = walk->Attribute("speedrate");
      if (val) ctrlrate.throttle = atof(val);

      val = walk->Attribute("turnspeedrate");
      if (val) sscanf(val, "%f , %f , %f", &ctrlrate.turn.x, &ctrlrate.turn.y, &ctrlrate.turn.z);

      val = walk->Attribute("turnspeedcoefficients");
      if (val) sscanf(val, "%f , %f", &param.turnspeed_a, &param.turnspeed_b);

      val = walk->Attribute("fineffect");
      if (val) sscanf(val, "%f , %f", &param.fineffect.x, &param.fineffect.y);

    } else if (!strcmp(walk->Value(), "drivesystem")) {
      
      for (XMLElement *walk2 = walk->FirstChildElement();
        walk2; walk2 = walk2->NextSiblingElement()) {
        if (!strcmp(walk2->Value(), "engine")) {
          
          float powerscale = 1.0f;
          
          val = walk2->Attribute("powerscale");
          if (val) powerscale = atof(val);
          
          for (XMLElement *walk3 = walk2->FirstChildElement();
            walk3; walk3 = walk3->NextSiblingElement()) {
            
            if (!strcmp(walk3->Value(), "powerpoint")) {
              
              float in_rpm, in_power;
              
              val = walk3->Attribute("rpm");
              if (!val) {
                PUtil::outLog() << "Warning: failed to read engine RPM value\n";
                continue;
              }
              in_rpm = atof(val);
              
              val = walk3->Attribute("power");
              if (!val) {
                PUtil::outLog() << "Warning: failed to read engine power value\n";
                continue;
              }
              in_power = atof(val);
              
              dsys.addPowerCurvePoint(in_rpm, in_power * powerscale);
            }
          }
          
        } else if (!strcmp(walk2->Value(), "gearbox")) {
          
          for (XMLElement *walk3 = walk2->FirstChildElement();
            walk3; walk3 = walk3->NextSiblingElement()) {
            
            if (!strcmp(walk3->Value(), "gear")) {
              
              val = walk3->Attribute("absolute");
              if (val) {
                dsys.addGear(atof(val));
              } else {
                val = walk3->Attribute("relative");
                if (!val) {
                  PUtil::outLog() << "Warning: gear has neither absolute nor relative value\n";
                  continue;
                }
                
                if (!dsys.hasGears()) {
                  PUtil::outLog() << "Warning: first gear cannot use relative value\n";
                  continue;
                }
                
                dsys.addGear(dsys.getLastGearRatio() * atof(val));
              }
            }
          }
          
        }
      }
      
    } else if (!strcmp(walk->Value(), "part")) {
      part.push_back(PVehicleTypePart());
      PVehicleTypePart *vtp = &part.back();

      vtp->parent = -1;
      //vtp->ref_local.setPosition(vec3f::zero());
      //vtp->ref_local.setOrientation(vec3f::zero());
      vtp->model = nullptr;
      vtp->scale = 1.0;

      val = walk->Attribute("name");
      if (val) vtp->name = val;

      val = walk->Attribute("parent");
      if (val) vtp->parentname = val;

      val = walk->Attribute("pos");
      if (val) {
        vec3f pos;
        if (sscanf(val, "%f , %f , %f", &pos.x, &pos.y, &pos.z) == 3)
          vtp->ref_local.setPosition(pos * allscale);
      }
      
      val = walk->Attribute("ori");
      if (val) {

        // note: w first, as per usual mathematical notation
        float w, x, y, z;
        if (sscanf(val, "%f , %f , %f , %f", &w, &x, &y, &z) == 4) {

          quatf ori(x, y, z, w);
          vtp->ref_local.setOrientation(ori);
        }
      }

      val = walk->Attribute("scale");
      if (val) vtp->scale = atof(val);

      val = walk->Attribute("model");
      if (val) vtp->model = ssModel.loadModel(PUtil::assemblePath(val, filename));

      for (XMLElement *walk2 = walk->FirstChildElement();
        walk2; walk2 = walk2->NextSiblingElement()) {
        if (!strcmp(walk2->Value(), "clip")) {
          vehicle_clip_s vc;

          vc.force = 0.0f;
          vc.dampening = 0.0f;

          val = walk2->Attribute("type");
          if (!val || !strlen(val)) {
            if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
              PUtil::outLog() << "Warning: <clip> element without type attribute\n";
            continue;
          }

          if (false) ;
          else if (!strcmp(val, "body")) vc.type = VCLIP_BODY;
          else if (!strcmp(val, "drive-left")) vc.type = VCLIP_DRIVE_LEFT;
          else if (!strcmp(val, "drive-right")) vc.type = VCLIP_DRIVE_RIGHT;
          else if (!strcmp(val, "hover")) vc.type = VCLIP_HOVER;
          else {
            if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
              PUtil::outLog() << "Warning: <clip> has unrecognised type \"" << val << "\"\n";
            continue;
          }

          val = walk2->Attribute("pos");
          if (!val) {
            if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
              PUtil::outLog() << "Warning: <clip> has no pos attribute\n";
            continue;
          }
          sscanf(val, "%f , %f , %f", &vc.pt.x, &vc.pt.y, &vc.pt.z);
          vc.pt *= allscale;

          val = walk2->Attribute("force");
          if (val) vc.force = atof(val);

          val = walk2->Attribute("dampening");
          if (val) vc.dampening = atof(val);

          vtp->clip.push_back(vc);
        } else if (!strcmp(walk2->Value(), "wheel")) {
          PVehicleTypeWheel vtw;
          
          vtw.radius = 1.0f;
          vtw.drive = 0.0f;
          vtw.steer = 0.0f;
          vtw.brake1 = 0.0f;
          vtw.brake2 = 0.0f;
          
          vtw.force = 0.0f;
          vtw.dampening = 0.0f;
          
          val = walk2->Attribute("pos");
          if (!val) {
            if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
              PUtil::outLog() << "Warning: <wheel> has no pos attribute\n";
            continue;
          }
          sscanf(val, "%f , %f , %f", &vtw.pt.x, &vtw.pt.y, &vtw.pt.z);
          vtw.pt *= allscale;
          
          val = walk2->Attribute("radius");
          if (val) vtw.radius = atof(val);
          
          val = walk2->Attribute("drive");
          if (val) vtw.drive = atof(val);
          
          val = walk2->Attribute("steer");
          if (val) vtw.steer = atof(val);
          
          val = walk2->Attribute("brake1");
          if (val) vtw.brake1 = atof(val);
          
          val = walk2->Attribute("brake2");
          if (val) vtw.brake2 = atof(val);
          
          val = walk2->Attribute("force");
          if (val) vtw.force = atof(val);
          
          val = walk2->Attribute("dampening");
          if (val) vtw.dampening = atof(val);
          
          vtp->wheel.push_back(vtw);
          drive_total += vtw.drive;
          wheel_speed_multiplier += 1.0f;
        } else if (!strcmp(walk2->Value(), "jetflame")) {
          vtp->flame.push_back(PReferenceFrame());
          
          val = walk2->Attribute("pos");
          if (val) {
            vec3f pos;
            if (sscanf(val, "%f , %f , %f", &pos.x, &pos.y, &pos.z) == 3)
              vtp->flame.back().setPosition(pos * allscale);
          }

          val = walk2->Attribute("ori");
          if (val) {

            float w, x, y, z;
            if (sscanf(val, "%f , %f , %f , %f", &w, &x, &y, &z) == 4) {
              quatf ori(x, y, z, w);
              vtp->flame.back().setOrientation(ori);
            }
          }
        }
      }

      vtp->ref_local.updateMatrices();
    }
  }

  for (unsigned int i=0; i<part.size(); ++i) {
    if (part[i].parentname.length() > 0) {
      unsigned int j;
      for (j=0; j<part.size(); ++j) {
        if (i == j) continue;
        if (part[i].parentname == part[j].name) {
          part[i].parent = j;
          break;
        }
      }
      if (j >= part.size() &&
        PUtil::isDebugLevel(DEBUGLEVEL_TEST))
        PUtil::outLog() << "Warning: part \"" << part[i].name <<
          "\" references non-existant parent \"" << part[i].parentname << "\"\n";
    }
  }
  
  if (drive_total > 0.0f)
    inverse_drive_total = 1.0f / drive_total;
  else
    inverse_drive_total = 0.0f;
  
  if (wheel_speed_multiplier > 0.0f)
    wheel_speed_multiplier = 1.0f / wheel_speed_multiplier;
 
  /* FIXME: use real vehicle values */

  return true;
}

void PVehicleType::unload()
{
  part.clear();
}


// PVehicle //


//TNL_IMPLEMENT_NETOBJECT(PVehicle); 

PVehicle::~PVehicle()
{
   /* Remove our vehicle */
  if(vehicle) {
    BulletLink::removeVehicle(vehicle);
    delete vehicle;
    delete vehicleRayCaster;
  }

  /* Remove our rigid bodies */
  if(chassisRigidBody) {
    BulletLink::removeRigidBody(chassisRigidBody);
    delete chassisRigidBody;
  }

  /* Remove our collision shapes */
  for(int i = 0; i < collisionShapes.size(); i++) {
    btCollisionShape* shape = collisionShapes[i];
    delete shape;
  }
  collisionShapes.clear();

  /* Remove our motion states */
  if(chassisMotionState) {
    delete chassisMotionState;
  }
}

PVehicle::PVehicle(PSim &sim_parent, PVehicleType *_type) :
  sim(sim_parent), type(_type), dsysi(&_type->dsys), vehicle(NULL),
  vehicleRayCaster(NULL), chassisRigidBody(NULL), chassisMotionState(NULL)
{
#if 0
  body = sim.createRigidBody();
  
  body->setMassCuboid(type->mass, type->dims);
#endif  
  state.setZero();
  ctrl.setZero();
  
  forwardspeed = 0.0f;
  
  blade_ang1 = 0.0;
  
  nextcp = 0;
  nextcdcp = 0;
  currentlap = 1;
  
  wheel_angvel = 0.0f;

#if 0
  reset_trigger_time = 0.0f;

  reset_time = 0.0f;
#endif  
  crunch_level = 0.0f;
  crunch_level_prev = 0.0f;
  
  part.resize(type->part.size());
  for (unsigned int i=0; i<part.size(); i++) {
    part[i].ref_local = type->part[i].ref_local;
    
    part[i].wheel.resize(type->part[i].wheel.size());
    
    for (unsigned int j=0; j<part[i].wheel.size(); j++) {
      part[i].wheel[j].ref_world.setPosition(vec3f(0,0,1000000)); // FIXME!!!
    }
  }
  
  updateParts();

  createBulletVehicle();
  
  //mNetFlags.set(Ghostable);
}

void PVehicle::createBulletVehicle()
{
  //TODO: get vehicle halfExtends from its model
  btVector3 halfExtends(1, 2, btScalar(0.5));

  btCollisionShape* chassisShape = new btBoxShape(halfExtends);
  collisionShapes.push_back(chassisShape);

  /* A compound shape is used so we can easily shift the center of gravity 
   * of our vehicle to its bottom.
   * This is needed to make our vehicle more stable. */
  //TODO: Test the vehicle without it
  btCompoundShape* compound = new btCompoundShape();
  collisionShapes.push_back(compound);

  /* The center of gravity of the compound shape is the origin. When we add a
   * rigidbody to the compound shape it's center of gravity does not change.
   * This way we can add the chassis rigidbody one unit above our center of
   * gravity keeping it under our chassis, and not in the middle of it */
  btTransform localTransform;
  localTransform.setIdentity();
  localTransform.setOrigin(btVector3(0, 0, 1));
  compound->addChildShape(localTransform, chassisShape);

  createChassisRigidBodyFromShape(compound);

  /* Let's create the raycast vehicle */
  vehicleRayCaster = BulletLink::createVehicleRaycaster();
  btRaycastVehicle::btVehicleTuning tuning;
  vehicle = new btRaycastVehicle(tuning, chassisRigidBody, vehicleRayCaster);

  /* Never deactivate the vehicle */
  chassisRigidBody->setActivationState(DISABLE_DEACTIVATION);

  /* Add the vehicle to the world */
  BulletLink::addVehicle(vehicle);

  /* Finally, add its wheels */
  addWheels(&halfExtends, tuning);
}


void PVehicle::createChassisRigidBodyFromShape(btCollisionShape* chassisShape)
{
  btTransform chassisTransform;
  chassisTransform.setIdentity();
  chassisTransform.setOrigin(btVector3(0, 0, 1));

  //FIXME: use real car values for mass
  btScalar mass(1200);

  /* Calculate its local inertia */
  btVector3 localInertia(0, 0, 0);
  chassisShape->calculateLocalInertia(mass, localInertia);

  /* Create our chassis motion state */
  chassisMotionState = new btDefaultMotionState(chassisTransform);

  /* Finally, the chassis rigid body */
  btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, chassisMotionState, 
        chassisShape, localInertia);
  chassisRigidBody = new btRigidBody(rbInfo); 
  BulletLink::addRigidBody(chassisRigidBody);
}

void PVehicle::addWheels(btVector3* halfExtents,
      btRaycastVehicle::btVehicleTuning tuning)
{
  //TODO: use vehicle's values from file.

  /* The direction of the raycast, the btRaycastVehicle uses raycasts instead
   * of simulating the wheels with rigid bodies */
  btVector3 wheelDirectionCS0(0, 0, -1);

  /* The axis which the wheel rotates arround */
  btVector3 wheelAxleCS(-1, 0, 0);

  btScalar suspensionRestLength(0.7);

  btScalar wheelWidth(0.4);

  btScalar wheelRadius(0.5);

  /* The height where the wheels are connected to the chassis */
  btScalar connectionHeight(1.2);

  /* All the wheel configuration assumes the vehicle is centered at the 
   * origin and a right handed coordinate system is used */
  btVector3 wheelConnectionPoint(halfExtents->x() - wheelRadius, 
        halfExtents->y() - wheelWidth,  connectionHeight);

  /* Adds the front wheels */
  vehicle->addWheel(wheelConnectionPoint, wheelDirectionCS0, wheelAxleCS, 
        suspensionRestLength, wheelRadius, tuning, true);

  vehicle->addWheel(wheelConnectionPoint * btVector3(-1, 1, 1), 
        wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius,
        tuning, true);

  /* Adds the rear wheels */
  vehicle->addWheel(wheelConnectionPoint* btVector3(1, -1, 1),
        wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius,
        tuning, false);

  vehicle->addWheel(wheelConnectionPoint * btVector3(-1, -1, 1), 
        wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius,
        tuning, false); 

  /* Configures each wheel of our vehicle, setting its friction, damping
   * compression, etc. */
  for(int i = 0; i < vehicle->getNumWheels(); i++)
  {
    btWheelInfo& wheel = vehicle->getWheelInfo(i);
    wheel.m_suspensionStiffness = 50;
    wheel.m_wheelsDampingCompression = btScalar(0.3) * 2 * 
       btSqrt(wheel.m_suspensionStiffness);//btScalar(0.8);
    wheel.m_wheelsDampingRelaxation = btScalar(0.5) * 2 * 
       btSqrt(wheel.m_suspensionStiffness);//1;

    /* Larger friction slips will result in better handling */
    wheel.m_frictionSlip = btScalar(1.2);
    wheel.m_rollInfluence = 1;
  }
}


void PVehicle::doReset()
{
#if 0
  if (reset_time != 0.0f) return;
  
  reset_pos = body->pos + vec3f(0.0f, 0.0f, 2.0f);
  
  vec3f forw = makevec3f(body->getOrientationMatrix().row[0]);
  float forwangle = atan2(forw.y, forw.x);
  
  quatf temp;
  temp.fromZAngle(forwangle);
  
  if (body->ori.dot(temp) < 0.0f) temp = temp * -1.0f;
  
  reset_ori = temp;
  
  reset_time = 3.0f;
  
  crunch_level = 0.0f;
  crunch_level_prev = 0.0f;
  
  for (unsigned int i=0; i<part.size(); i++) {
    for (unsigned int j=0; j<part[i].wheel.size(); j++) {
      part[i].wheel[j].spin_vel = 0.0f;
      part[i].wheel[j].spin_pos = 0.0f;
      part[i].wheel[j].ride_vel = 0.0f;
      part[i].wheel[j].ride_pos = 0.0f;
      part[i].wheel[j].turn_pos = 0.0f;
      part[i].wheel[j].skidding = 0.0f;
      part[i].wheel[j].dirtthrow = 0.0f;
    }
  }  
#endif
  forwardspeed = 0.0f;
  wheel_angvel = 0.0f;
  wheel_speed = 0.0f;
  
  dsysi.doReset();
  
  state.setZero();

}

void PVehicle::doReset2(const vec3f &pos, const quatf &ori)
{
#if 0
  if (reset_time != 0.0f) return;

  reset_pos = pos;

  quatf temp = ori; // FIXME: laziness and fear of breaking copy-pasted code

  if (body->ori.dot(temp) < 0.0f) temp = temp * -1.0f;

  reset_ori = temp;

  reset_time = 3.0f;

  crunch_level = 0.0f;
  crunch_level_prev = 0.0f;

  for (unsigned int i=0; i<part.size(); i++) {
    for (unsigned int j=0; j<part[i].wheel.size(); j++) {
      part[i].wheel[j].spin_vel = 0.0f;
      part[i].wheel[j].spin_pos = 0.0f;
      part[i].wheel[j].ride_vel = 0.0f;
      part[i].wheel[j].ride_pos = 0.0f;
      part[i].wheel[j].turn_pos = 0.0f;
      part[i].wheel[j].skidding = 0.0f;
      part[i].wheel[j].dirtthrow = 0.0f;
    }
  }
#endif

  forwardspeed = 0.0f;
  wheel_angvel = 0.0f;
  wheel_speed = 0.0f;

  dsysi.doReset();

  state.setZero();

  /* Reset bullet vehicle rigid body position and orientation */
  setPositionAndOrientation(pos, ori);
}

void PVehicle::setPositionAndOrientation(const vec3f& pos, const quatf& ori)
{
  /* Update our current 'cached' values  */
  curReference.pos = pos;
  curReference.ori = ori;
  curReference.updateMatrices();

  /* Define bullet physics objects */
  btRigidBody* rigidBody = vehicle->getRigidBody();
  rigidBody->clearForces();
  btTransform transform = rigidBody->getCenterOfMassTransform();
  transform.setOrigin(btVector3(pos.x,pos.y,pos.z));
  transform.setRotation(ori);
  rigidBody->setCenterOfMassTransform(transform);
}

void PVehicle::tick(float delta)
{
  /* Retrieve current position and orientation from bullet */
  btVector3 vPos(vehicle->getRigidBody()->getCenterOfMassPosition());
  curReference.pos = vec3f(vPos[0], vPos[1], vPos[2]);
  
  btQuaternion vOri(
        vehicle->getRigidBody()->getCenterOfMassTransform().getRotation());

  curReference.ori = quatf(vOri[0], vOri[1], vOri[2], vOri[3]);
  curReference.updateMatrices();

  // ensure control values are in valid range
  ctrl.clamp();
  
  // handle crunch noise level
  PULLTOWARD(crunch_level_prev, crunch_level, delta * 5.0f);
  PULLTOWARD(crunch_level, 0.0f, delta * 5.0f);
  
  // smooth out control values
  PULLTOWARD(state.throttle, ctrl.throttle, type->ctrlrate.throttle * delta);
  PULLTOWARD(state.brake1, ctrl.brake1, type->ctrlrate.brake1 * delta);
  PULLTOWARD(state.brake2, ctrl.brake2, type->ctrlrate.brake2 * delta);
  PULLTOWARD(state.turn.x, ctrl.turn.x, type->ctrlrate.turn.x * delta);
  PULLTOWARD(state.turn.y, ctrl.turn.y, type->ctrlrate.turn.y * delta);
  PULLTOWARD(state.turn.z, ctrl.turn.z, type->ctrlrate.turn.z * delta);
  //PULLTOWARD(state.aim.x, ctrl.aim.x, type->ctrlrate.aim.x * delta);
  //PULLTOWARD(state.aim.y, ctrl.aim.y, type->ctrlrate.aim.y * delta);
  PULLTOWARD(state.collective, ctrl.collective, type->ctrlrate.collective * delta);
  
#if 0
  // prepare some useful data
  //vec3f pos = body->getPosition();
  vec3f linvel = body->getLinearVel();
  mat44f orimatt = body->getInverseOrientationMatrix();
  vec3f angvel = body->getAngularVel();
  
  if (orimatt.row[2].z <= 0.1f) {
    reset_trigger_time += delta;
    
    if (reset_trigger_time >= 4.0f)
      doReset();
  } else
    reset_trigger_time = 0.0f;
  
  vec3f loclinvel = body->getWorldToLocVector(linvel);
  vec3f locangvel = body->getWorldToLocVector(angvel);
  //vec3f locangvel = body->getLocToWorldVector(angvel);
  //vec3f locangvel = angvel;
  
  // check for resetting (if the vehicle has been flipped or something)
  if (reset_time != 0.0f) {
    if (reset_time > 0.0f) {
      PULLTOWARD(body->pos, reset_pos, delta * 2.0f);
      PULLTOWARD(body->ori, reset_ori, delta * 2.0f);
      
      body->setLinearVel(vec3f::zero());
      body->setAngularVel(vec3f::zero());
      
      body->updateMatrices();
      
      reset_time -= delta;
      if (reset_time <= 0.0f)
        reset_time = -2.0f;
      
      return;
      
    } else {
      reset_time += delta;
      
      if (reset_time > 0.0f)
        reset_time = 0.0f;
    }
  }
  
  forwardspeed = loclinvel.y;
  
  // body turn control
  vec3f desiredturn = vec3f(
    state.turn.x * type->param.turnspeed.x,
    state.turn.y * type->param.turnspeed.y,
    state.turn.z * type->param.turnspeed.z);
  body->addLocTorque(desiredturn * type->param.turnspeed_a);
  
  body->addLocTorque((desiredturn - locangvel) * (type->param.turnspeed_b * loclinvel.y));
  
  // fin effect (torque due to drag)
  body->addLocTorque(vec3f(-loclinvel.z * type->param.fineffect.y, 0.0, loclinvel.x * type->param.fineffect.x));
  
  // angular drag
  body->addTorque(angvel * -type->param.angdrag);

  // linear drag
  vec3f frc = -vec3f(
    loclinvel.x * type->param.drag.x,
    loclinvel.y * type->param.drag.y,
    loclinvel.z * type->param.drag.z);

  // lift
  frc += -vec3f(
    loclinvel.x * type->param.lift.x * loclinvel.y,
    0.0,
    loclinvel.z * type->param.lift.y * loclinvel.y);


  body->addLocForce(frc);

  vec3f forwarddir = makevec3f(body->getInverseOrientationMatrix().row[1]);
  //vec3f rightdir = makevec3f(body->getInverseOrientationMatrix().row[0]);
#endif  
  dsysi.tick(delta, state.throttle, wheel_angvel);
  
  float drivetorque = dsysi.getOutputTorque();
  //float drivetorque = 0.0f;
  
  float turnfactor = state.turn.z;// /

  /* Applying engine force on rear wheels. */
  //TODO: Vehicle should be configurable on which wheels receive 
  //engine traction
  vehicle->applyEngineForce(-1*drivetorque*10, 2);
  vehicle->applyEngineForce(-1*drivetorque*10, 3);

  /* Apply turn factor to front wheels */
  vehicle->setSteeringValue(-1*turnfactor, 0);
  vehicle->setSteeringValue(-1*turnfactor, 1);


    //(1.0f + fabsf(wheel_angvel) / 70.0f);
#if 0
  wheel_angvel = 0.0f;
  
  wheel_speed = 0.0f;
  
  skid_level = 0.0f;
  
  for (unsigned int i=0; i<part.size(); ++i) {
    for (unsigned int j=0; j<type->part[i].clip.size(); ++j) {
      
      vec3f lclip = type->part[i].clip[j].pt;
      
      vec3f wclip = part[i].ref_world.getLocToWorldPoint(lclip);
      
      PTerrain::ContactInfo tci;
      tci.pos.x = wclip.x;
      tci.pos.y = wclip.y;
      
      sim.getTerrain()->getContactInfo(tci);
      
      if (type->part[i].clip[j].type == VCLIP_HOVER) {
        if (tci.pos.z < 40.3) {
          tci.pos.z = 40.3;
          tci.normal = vec3f(0,0,1);
        }
      }
      
      if (wclip.z <= tci.pos.z) {
      
        float depth = (tci.pos - wclip) * tci.normal;
        vec3f ptvel = body->getLinearVelAtPoint(wclip);
      
        vec3f frc = vec3f::zero();
      
        switch (type->part[i].clip[j].type) {
        default:
        case VCLIP_BODY:
          {
            #if 0
            frc += vec3f(0.0, 0.0, type->part[i].clip[j].force);
            
            frc += ptvel * -type->part[i].clip[j].dampening;
            
            frc *= depth;
            #else
            vec3f rightdir;
            if (tci.normal.x > 0.5f)
              rightdir = vec3f(0.0f, 1.0f, 0.0f);
            else
              rightdir = vec3f(1.0f, 0.0f, 0.0f);
            
            //float testval = tci.normal * rightdir;
            
            vec3f surf_forward = tci.normal ^ rightdir;
            surf_forward.normalize();
            vec3f surf_right = surf_forward ^ tci.normal;
            surf_right.normalize();
            
            vec3f surfvel(
              ptvel * surf_right,
              ptvel * surf_forward,
              ptvel * tci.normal);
            
            float perpforce = depth * type->part[i].clip[j].force -
              surfvel.z * type->part[i].clip[j].dampening;
            
            // check we have positive normal force
            
            if (perpforce > 0.0f) {
              vec2f friction = vec2f(-surfvel.x, -surfvel.y) * 10000.0f;
              
              float maxfriction = perpforce * 0.9f;
              float testfriction = perpforce * 1.2f;
              
              float leng = friction.length();
              
              if (leng > 0.0f && leng > testfriction)
                friction *= (maxfriction / leng);
              
              frc += (tci.normal * perpforce +
                  surf_right * friction.x +
                  surf_forward * friction.y);
              
              CLAMP_LOWER(crunch_level, perpforce * 0.00001f);
            }
            #endif
          } break;

        case VCLIP_DRIVE_LEFT:
          {
            frc += vec3f(0.0, 0.0, type->part[i].clip[j].force);

            vec3f drivevec = forwarddir *
              (state.throttle * type->param.speed +
              state.turn.z * type->param.turnspeed.z);

            vec3f relvel = drivevec - tci.normal * (drivevec * tci.normal);

            frc += (ptvel - relvel) * -type->part[i].clip[j].dampening;

            frc *= depth;
          } break;

        case VCLIP_DRIVE_RIGHT:
          {
            frc += vec3f(0.0, 0.0, type->part[i].clip[j].force);

            vec3f drivevec = forwarddir *
              (state.throttle * type->param.speed -
              state.turn.z * type->param.turnspeed.z);

            vec3f relvel = drivevec - tci.normal * (drivevec * tci.normal);

            frc += (ptvel - relvel) * -type->part[i].clip[j].dampening;

            frc *= depth;
          } break;

        case VCLIP_HOVER:
          {
            float surfvelz = ptvel * tci.normal;

            float perpfrc = type->part[i].clip[j].force;
            if (surfvelz < 0.0) perpfrc += surfvelz * -type->part[i].clip[j].dampening;

            frc += (tci.normal * perpfrc) * depth;
          } break;
        }

        body->addForceAtPoint(frc, wclip);
      }
    }
    
    for (unsigned int j=0; j<type->part[i].wheel.size(); ++j) {
      
      PVehicleWheel &wheel = part[i].wheel[j];
      PVehicleTypeWheel &typewheel = type->part[i].wheel[j];

        const TerrainType mf_tt = sim.getTerrain()->getRoadSurface(wheel.ref_world.getPosition());
        const float mf_coef     = PUtil::decideFrictionCoef(mf_tt);
        const float mf_resis    = PUtil::decideResistance(mf_tt);

      vec3f wclip = wheel.ref_world.getPosition();
      
      //vec3f wclip = vec3f(0,0,2000);
      
      // TODO: calc wclip along wheel plane instead of just straight down
      wclip.z -= typewheel.radius;
      
      wclip.z += INTERP(wheel.bumplast, wheel.bumpnext, wheel.bumptravel);
      
      wheel.spin_vel += drivetorque * typewheel.drive * delta * (1.0f - mf_resis);
      
      float desiredchange = (state.brake1 * typewheel.brake1 +
        state.brake2 * typewheel.brake2) * delta;
      if (wheel.spin_vel > desiredchange)
        wheel.spin_vel -= desiredchange;
      else if (wheel.spin_vel < -desiredchange)
        wheel.spin_vel += desiredchange;
      else
        wheel.spin_vel = 0.0f;
      
      wheel.spin_pos += wheel.spin_vel * delta;
      wheel.spin_vel -= wheel.spin_vel * mf_resis * delta;
      
      wheel.turn_pos = turnfactor * typewheel.steer;
      
      wheel.dirtthrow = 0.0f;
      
      float suspension_force = wheel.ride_pos * typewheel.force;
      
      wheel.ride_vel +=
        (-suspension_force -
        wheel.ride_vel * typewheel.dampening) * 0.02 * delta;
      wheel.ride_pos += wheel.ride_vel * delta;
      
      PTerrain::ContactInfo tci;
      tci.pos.x = wclip.x;
      tci.pos.y = wclip.y;
      
      sim.getTerrain()->getContactInfo(tci);
      
      if (wclip.z <= tci.pos.z) {
        
        wheel.bumptravel += fabsf(wheel.spin_vel) * 0.6f * delta;
        
        if (wheel.bumptravel >= 1.0f) {
          wheel.bumplast = wheel.bumpnext;
          wheel.bumptravel -= (int)wheel.bumptravel;
          
          wheel.bumpnext = randm11 * rand01 * typewheel.radius * 0.1f;
        }
        
        float depth = (tci.pos - wclip) * tci.normal;
        vec3f ptvel = body->getLinearVelAtPoint(wclip);
        
        vec3f frc = vec3f::zero();
        
        vec3f rightdir = makevec3f(wheel.ref_world.getInverseOrientationMatrix().row[0]);
        
        //float testval = tci.normal * rightdir;

        vec3f surf_forward = tci.normal ^ rightdir;
        surf_forward.normalize();
        vec3f surf_right = surf_forward ^ tci.normal;
        surf_right.normalize();
        
        // add wheel rotation speed to ptvel
        ptvel += surf_forward * (-wheel.spin_vel * typewheel.radius) * (1.0f - mf_resis);

        vec3f surfvel(
          ptvel * surf_right,
          ptvel * surf_forward,
          ptvel * tci.normal);
        
        float perpforce = suspension_force;
        if (surfvel.z < 0.0f) perpforce -= surfvel.z * typewheel.dampening;
        
        wheel.ride_pos += depth;
        
        float maxdepth = typewheel.radius * 0.7f;
        
        if (wheel.ride_pos > maxdepth) {
          float overdepth = wheel.ride_pos - maxdepth;
          
          wheel.ride_pos = maxdepth;
          
          perpforce -= overdepth * surfvel.z * typewheel.dampening * 5.0f;
        }
        
        if (wheel.ride_vel < -surfvel.z)
          wheel.ride_vel = -surfvel.z;
        
        // check we have positive normal force
        
        if (perpforce > 0.0f) {
          vec2f friction = vec2f(-surfvel.x, -surfvel.y) * 10000.0f;

          //float maxfriction = perpforce * 1.0f;
          float maxfriction = perpforce * mf_coef;
          float testfriction = perpforce * 1.0f;
          
          float leng = friction.length();
          
          if (leng > 0.0f && leng > testfriction)
            friction *= (maxfriction / leng) + 0.02f;

          frc += (tci.normal * perpforce +
              surf_right * friction.x +
              surf_forward * friction.y);
          
          wheel.spin_vel -= (friction.y * typewheel.radius) * 0.1f * delta;
          
          //wheel.turn_vel -= friction.x * 1.0f * delta;

          body->addForceAtPoint(frc, wclip);
          
          wheel.dirtthrow = leng / maxfriction;
          skid_level += wheel.dirtthrow;
          
          vec3f downward = surf_forward ^ rightdir;
          downward.normalize();
          
          if (wheel.spin_vel > 0.0f)
            downward += surf_forward * -0.3f;
          else
            downward += surf_forward * 0.3f;
          downward.normalize();
          
          wheel.dirtthrowpos = wheel.ref_world.getPosition() +
            downward * typewheel.radius;
          wheel.dirtthrowvec =
            body->getLinearVelAtPoint(wheel.dirtthrowpos) +
            (downward ^ rightdir) * (wheel.spin_vel * typewheel.radius);
        }
      }
      
      //wheel.spin_vel /= 1.0f + delta * 0.6f;
      
      wheel.spin_pos = fmodf(wheel.spin_pos, PI*2.0f);
      
      wheel_angvel += wheel.spin_vel * typewheel.drive;
      
      wheel_speed += wheel.spin_vel * typewheel.radius;
    }
  }
  
  wheel_angvel *= type->inverse_drive_total;
  
  wheel_speed *= type->wheel_speed_multiplier;
  
  skid_level *= type->wheel_speed_multiplier;
#endif
}

///
/// @brief Checks if vehicle can have a dust trail.
/// @todo Use a dynamic height?
///
bool PVehicle::canHaveDustTrail()
{
    for (unsigned int i=0; i<part.size(); ++i)
    {
        for (unsigned int j=0; j<type->part[i].wheel.size(); ++j)
        {
            PVehicleWheel &wheel = part[i].wheel[j];
            PVehicleTypeWheel &typewheel = type->part[i].wheel[j];
            vec3f wclip = wheel.ref_world.getPosition();

            // TODO: calc wclip along wheel plane instead of just straight down
            wclip.z -= typewheel.radius;

            wclip.z += INTERP(wheel.bumplast, wheel.bumpnext, wheel.bumptravel);

            PTerrain::ContactInfo tci;

            tci.pos.x = wclip.x;
            tci.pos.y = wclip.y;
            sim.getTerrain()->getContactInfo(tci);

#define MAX_HEIGHT 1.5f
            if (wclip.z - tci.pos.z <= MAX_HEIGHT)
                return true;
#undef MAX_HEIGHT
        }
    }

    return false;
}

void PVehicle::updateParts()
{
  //TODO: use values from bullet (specially for wheels).
  for (unsigned int i=0; i<part.size(); ++i) {
    PReferenceFrame *parent;
    if (type->part[i].parent > -1)
      parent = &part[type->part[i].parent].ref_world;
    else
      parent = &curReference;

    part[i].ref_world.ori = part[i].ref_local.ori * parent->ori;

    part[i].ref_world.updateMatrices();

    part[i].ref_world.pos = parent->pos +
      parent->getOrientationMatrix().transform1(part[i].ref_local.pos);
    
    for (unsigned int j=0; j<part[i].wheel.size(); j++) {
      vec3f locpos = type->part[i].wheel[j].pt +
            vec3f(0.0f, 0.0f, part[i].wheel[j].ride_pos);
      
      part[i].wheel[j].ref_world.setPosition(part[i].ref_world.getLocToWorldPoint(locpos));
      
      quatf turnang(0.0f, 0.0f, part[i].wheel[j].turn_pos);
      quatf spinang(part[i].wheel[j].spin_pos, 0.0f, 0.0f);
      
      part[i].wheel[j].ref_world.ori = spinang * turnang * part[i].ref_world.ori;
      
      part[i].wheel[j].ref_world.updateMatrices();
    }
  }
}
