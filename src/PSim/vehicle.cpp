
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
  /* Note: the old trigger physics simulation was defining wrong values
   * for the wheel revolutions per second (very high ones). Thus, to not
   * break the PDriveInstance or have to rewrite it from scratch, this
   * correction_multiplier is used. */
  const float correction_multiplier = 12.0f;
  wheel_rps *= correction_multiplier;

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

  out_torque *= throttle * correction_multiplier * 0.5f;
  
  if (reverse) {
    out_torque *= -1.0;
  }
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
  
  wheeldrive = WHEEL_DRIVE_TYPE_FWD;
  mass = 1.0;
  normalbrake = 20.0f;
  handbrake = 80.0f;
  maxsteering = 1.0f;

  suspension.stiffness = 35.0f;
  suspension.compressionk = 0.3f;
  suspension.relaxationk = 0.5f;
  suspension.restlength = 0.7f;
  suspension.maxtravel = 500;

  drift.threshold = 4.0f;
  drift.torquelevel = 1.0f;
  
  wheelmodel = nullptr;
  
  ctrlrate.setDefaultRates();
  
  float allscale = 1.0;
  
  float drive_total = 0.0f;
  
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

        val = walk->Attribute("enginebhp");
        if (val != nullptr) pstat_enginebhp = val;

        val = walk->Attribute("wheeldrive");
        if (val != nullptr) {
           pstat_wheeldrive = val;
           if(pstat_wheeldrive == "RWD") {
             wheeldrive = PVehicleType::WHEEL_DRIVE_TYPE_RWD;
           }
           else if(pstat_wheeldrive == "4WD") {
             wheeldrive = PVehicleType::WHEEL_DRIVE_TYPE_4WD;
           }
           else {
             wheeldrive = PVehicleType::WHEEL_DRIVE_TYPE_FWD;
           }
        }

        val = walk->Attribute("handling");
        if (val != nullptr) pstat_handling = val;
    } else if (!strcmp(walk->Value(), "brake")) {
      val = walk->Attribute("normal");
      if(val != nullptr) {
        normalbrake = atof(val);
      }
      val = walk->Attribute("handbrake");
      if(val != nullptr) {
        handbrake = atof(val);
      }
    } else if (!strcmp(walk->Value(), "suspension")) {
      val = walk->Attribute("stiffness");
      if(val != nullptr) {
        suspension.stiffness = atof(val);
      }
      val = walk->Attribute("compressionk");
      if(val != nullptr) {
        suspension.compressionk = atof(val);
      }
      val = walk->Attribute("relaxationk");
      if(val != nullptr) {
        suspension.relaxationk = atof(val);
      }
      val = walk->Attribute("restlength");
      if(val != nullptr) {
        suspension.restlength = atof(val);
      }
      val = walk->Attribute("maxtravel");
      if(val != nullptr) {
        suspension.maxtravel = atof(val);
      }
    } else if (!strcmp(walk->Value(), "drift")) {
      val = walk->Attribute("threshold");
      if(val != nullptr) {
        drift.threshold = atof(val);
      }
      val = walk->Attribute("torquelevel");
      if(val != nullptr) {
        drift.torquelevel = atof(val);
      }
    } else if (!strcmp(walk->Value(), "genparams")) {

      val = walk->Attribute("mass");
      if (val != nullptr) {
         pstat_weightkg = val;
         mass = atof(val);
      }

      val = walk->Attribute("steering");
      if (val) maxsteering = atof(val);

      val = walk->Attribute("wheelmodel");
      if (val) wheelmodel = ssModel.loadModel(PUtil::assemblePath(val, filename));
      
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
          
          vtw.scale = 1.0f;
          vtw.rollinfluence = 1.0f;
          vtw.friction = 0.8f;
          
          val = walk2->Attribute("pos");
          if (!val) {
            if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
              PUtil::outLog() << "Warning: <wheel> has no pos attribute\n";
            continue;
          }
          sscanf(val, "%f , %f , %f", &vtw.pt.x, &vtw.pt.y, &vtw.pt.z);
          vtw.pt *= allscale;
          
          val = walk2->Attribute("scale");
          if (val) vtw.scale = atof(val);

          val = walk2->Attribute("friction");
          if (val) vtw.friction = atof(val);

          val = walk2->Attribute("rollinfluence");
          if (val) vtw.rollinfluence = atof(val);
          
          vtp->wheel.push_back(vtw);
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
  state.setZero();
  ctrl.setZero();
  
  blade_ang1 = 0.0;
  
  nextcp = 0;
  nextcdcp = 0;
  currentlap = 1;
  
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
  }
  
  updateParts();

  createBulletVehicle();
  
  //mNetFlags.set(Ghostable);
}

void PVehicle::debugDraw()
{
   vehicle->debugDraw(BulletLink::getDebugDraw());
   BulletLink::getDebugDraw()->render();
}

void PVehicle::createBulletVehicle()
{
  /* Define body sizes */
  vec3f ext = type->part[0].model->getHalfExtents() * type->part[0].scale;
  btVector3 halfExtends(ext[0], ext[1], ext[2]);

  /* define wheel sizes */
  //FIXME should be from wheel definition and one per wheel.
  ext = type->wheelmodel->getHalfExtents() * getWheelScaleFactor(0);
  wheelWidth = ext[0];
  wheelRadius = ext[2];
  wheelPerimeter = 2 * M_PI * wheelRadius;

  btCollisionShape* chassisShape = new btBoxShape(halfExtends);
  collisionShapes.push_back(chassisShape);

  /* A compound shape is used so we can easily shift the center of gravity 
   * of our vehicle to its bottom.
   * This is needed to make our vehicle more stable. */
  btCompoundShape* compound = new btCompoundShape();
  collisionShapes.push_back(compound);

  /* The center of gravity of the compound shape is the origin. When we add a
   * rigidbody to the compound shape it's center of gravity does not change.
   * This way we can add the chassis rigidbody one unit above our center of
   * gravity keeping it under our chassis, and not in the middle of it */
  chassisDiff = 1.0f;
  btTransform localTransform;
  localTransform.setIdentity();
  localTransform.setOrigin(btVector3(0.0f, 0.0f, getChassisDiff()));
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
  chassisTransform.setOrigin(btVector3(0.0f, 0.0f, getChassisDiff()));

  /* Calculate its local inertia */
  btVector3 localInertia(0, 0, 0);
  chassisShape->calculateLocalInertia(type->mass, localInertia);

  /* Create our chassis motion state */
  chassisMotionState = new btDefaultMotionState(chassisTransform);

  /* Finally, the chassis rigid body */
  btRigidBody::btRigidBodyConstructionInfo rbInfo(type->mass, 
        chassisMotionState, chassisShape, localInertia);
  chassisRigidBody = new btRigidBody(rbInfo); 
  BulletLink::addRigidBody(chassisRigidBody);
}

void PVehicle::addWheels(btVector3* halfExtents,
      btRaycastVehicle::btVehicleTuning tuning)
{
  /* The direction of the raycast, the btRaycastVehicle uses raycasts instead
   * of simulating the wheels with rigid bodies */
  btVector3 wheelDirectionCS0(0, 0, -1);

  /* The axis which the wheel rotates arround */
  btVector3 wheelAxleCS(1, 0, 0);

  /* The maximum length of the suspension (metres) */
  btScalar suspensionRestLength(type->suspension.restlength);

  /* Adds the front wheels */
  vec3f pos = type->part[0].wheel[0].pt; 
  vehicle->addWheel(btVector3(pos[0], pos[1], pos[2]), 
        wheelDirectionCS0, wheelAxleCS, 
        suspensionRestLength, wheelRadius, tuning, true);

  pos = type->part[0].wheel[1].pt; 
  vehicle->addWheel(btVector3(pos[0], pos[1], pos[2]), 
        wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius,
        tuning, true);

  /* Adds the rear wheels */
  pos = type->part[0].wheel[2].pt; 
  vehicle->addWheel(btVector3(pos[0], pos[1], pos[2]), 
        wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius,
        tuning, false);

  pos = type->part[0].wheel[3].pt; 
  vehicle->addWheel(btVector3(pos[0], pos[1], pos[2]), 
        wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius,
        tuning, false); 

  /* Configures each wheel of our vehicle, setting its friction, damping
   * compression, etc. */
  for(int i = 0; i < vehicle->getNumWheels(); i++)
  {
    btWheelInfo& wheel = vehicle->getWheelInfo(i);
    /* The stiffness constant for the suspension. 10.0 - Offroad buggy,
     * 50.0 - Sports car, 200.0 - F1 Car */
    wheel.m_suspensionStiffness = type->suspension.stiffness;
    /* The maximum distance the suspension can be compressed (centimetres) */
    wheel.m_maxSuspensionTravelCm = type->suspension.maxtravel;
    /* The damping coefficient for when the suspension is compressed. Set
     * to k * 2.0 * btSqrt(m_suspensionStiffness) so k is proportional to 
     * critical damping.
     * k = 0.0 undamped & bouncy, k = 1.0 critical damping
     * k = 0.1 to 0.3 are good values */
    wheel.m_wheelsDampingCompression = 
       btScalar(type->suspension.compressionk) * 2 *
       btSqrt(wheel.m_suspensionStiffness);
    /* The damping coefficient for when the suspension is expanding. See
     * the comments for m_wheelsDampingCompression for how to set k.
     * m_wheelsDampingRelaxation should be slightly larger than
     * m_wheelsDampingCompression, eg k = 0.2 to 0.5 */
    wheel.m_wheelsDampingRelaxation = btScalar(type->suspension.relaxationk) * 
       2 * btSqrt(wheel.m_suspensionStiffness);

    /* The coefficient of friction between the tyre and the ground.
     * Should be about 0.8 for realistic cars, but can increased for better
     * handling. */
    wheel.m_frictionSlip = type->part[0].wheel[i].friction;
    /* Reduces the rolling torque applied from the wheels that cause the
     * vehicle to roll over.
     * This is a bit of a hack, but it's quite effective. 0.0 = no roll,
     * 1.0 = physical behaviour.
     * If m_frictionSlip is too high, you'll need to reduce this to stop
     * the vehicle rolling over.
     * You should also try lowering the vehicle's centre of mass */
    wheel.m_rollInfluence = type->part[0].wheel[i].rollinfluence;
  }
}

float PVehicle::getWheelAngularSpeed(float delta) const
{
  /* Calculating by vehicle velocity and how many revolutions the wheel
   * will do at that velocity */
  return KPH_TO_MPS(getSpeed()) / wheelPerimeter;
}

const float PVehicle::getWheelScaleFactor(int i) const
{
   assert((i >= 0) && (i <=3));
   return type->part[0].wheel[i].scale;
}

const float PVehicle::getWheelRadius(int i) const
{
   //FIXME
   assert((i >= 0) && (i <=3));
   return wheelRadius;
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
#endif
  
  for (unsigned int i=0; i<part.size(); i++) {
    for (unsigned int j=0; j<part[i].wheel.size(); j++) {
      part[i].wheel[j].reset();
    }
  }  

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

#endif
  for (unsigned int i=0; i<part.size(); i++) {
    for (unsigned int j=0; j<part[i].wheel.size(); j++) {
      part[i].wheel[j].reset();
    }
  }

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

void PVehicle::applyZTorque(float mult, bool alwaysApply) 
{
   btVector3 dir(0.0f, 0.0f, chassisRigidBody->getAngularVelocity()[2]);
   if((alwaysApply) || (fabs(dir[2]) < 1.0f)) {
     /* Note: the fabs check is to avoid infinite spinning when applying
      *       at the same direction. */
     chassisRigidBody->applyTorqueImpulse(dir * mult); 
  }
}

void PVehicle::tick(float delta)
{
  /* Retrieve current position and orientation from bullet */
  btVector3 vPos(vehicle->getRigidBody()->getCenterOfMassPosition());
  curReference.pos = vec3f(vPos[0], vPos[1], 
        vPos[2] + getChassisDiff() - getWheelRadius(0));
  
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
  dsysi.tick(delta, state.throttle, getWheelAngularSpeed(delta));
  
  float drivetorque = dsysi.getOutputTorque();
  
  float turnfactor = state.turn.z;// /

  bool isBrakeing = (state.brake1 >= 0.01f) || (state.brake2 >= 0.01f);
  if(isBrakeing) {
     drivetorque = 0.0f;
  }

  /* Applying engine force and normal brake to the wheels, 
   * based on transmission type */
  if((type->wheeldrive == PVehicleType::WHEEL_DRIVE_TYPE_RWD) || 
     (type->wheeldrive == PVehicleType::WHEEL_DRIVE_TYPE_4WD)) {

     /* Rear wheels */
     vehicle->applyEngineForce(drivetorque, 2);
     vehicle->applyEngineForce(drivetorque, 3);
  }
  if((type->wheeldrive == PVehicleType::WHEEL_DRIVE_TYPE_FWD) || 
     (type->wheeldrive == PVehicleType::WHEEL_DRIVE_TYPE_4WD)) {
     
     /* Front wheels */
     vehicle->applyEngineForce(drivetorque, 0);
     vehicle->applyEngineForce(drivetorque, 1);
  }

  /* Apply Brakes */
  if(isBrakeing) {
    if(isHandBrakeActive()) {
      /* Hand brake is always applied on rear wheels */
      vehicle->setBrake(type->handbrake * state.brake2, 2);
      vehicle->setBrake(type->handbrake * state.brake2, 3);
      /* Should apply a torque to the chassis (due to wheel lock) */
      applyZTorque(type->handbrake * 0.5f, false); 
    } else {
      /* Normal brake is applied on transmission wheels */
      if((type->wheeldrive == PVehicleType::WHEEL_DRIVE_TYPE_RWD) || 
         (type->wheeldrive == PVehicleType::WHEEL_DRIVE_TYPE_4WD)) {
        vehicle->setBrake(type->normalbrake * state.brake1, 0);
        vehicle->setBrake(type->normalbrake * state.brake1, 1);
      }
      if((type->wheeldrive == PVehicleType::WHEEL_DRIVE_TYPE_FWD) || 
         (type->wheeldrive == PVehicleType::WHEEL_DRIVE_TYPE_4WD)) {
        vehicle->setBrake(type->normalbrake * state.brake1, 0);
        vehicle->setBrake(type->normalbrake * state.brake1, 1);
      }
    }
  } else {
    /* No brake applied */
    vehicle->setBrake(0, 0);
    vehicle->setBrake(0, 1);
    vehicle->setBrake(0, 2);
    vehicle->setBrake(0, 3);
  }

  /* Apply turn factor to front wheels */
  vehicle->setSteeringValue(-1*turnfactor * type->maxsteering, 0);
  vehicle->setSteeringValue(-1*turnfactor * type->maxsteering, 1);

  /* Set current skid_level.
   * Note: for compatibility with the old value, we kept it inverse of
   *       what is defined from Bullet. */
  skid_level = 4.0f;
  for(int i=0; i < 4; i++) {
     skid_level -= vehicle->getWheelInfo(i).m_skidInfo;
  }

  /* Apply torque for automatic drift */
  if(skid_level > type->drift.threshold) {
    applyZTorque((skid_level / 4.0f) * (type->drift.torquelevel), 
          type->drift.torquelevel > 0.0f);
  }

    //(1.0f + fabsf(wheel_angvel) / 70.0f);
#if 0
  wheel_angvel = 0.0f;
 
  
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
      
    }
  }
  
  wheel_angvel *= type->inverse_drive_total;
  
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
            //PVehicleTypeWheel &typewheel = type->part[i].wheel[j];
            vec3f wclip(wheel.worldtrans.getOrigin());

            // TODO: calc wclip along wheel plane instead of just straight down
            //wclip.z -= typewheel.radius;
            wclip.z -= wheelRadius;

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
  if(!vehicle) {
    return;
  }
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
      part[i].wheel[j].worldtrans = vehicle->getWheelInfo(j).m_worldTransform;
    }
  }
}
