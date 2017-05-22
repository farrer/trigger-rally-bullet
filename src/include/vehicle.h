
// vehicle.h [psim]

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)

#include <btBulletDynamicsCommon.h>

// vehicle core types

#define VCTYPE_TANK         1
#define VCTYPE_HELICOPTER   2
#define VCTYPE_PLANE        3
#define VCTYPE_HOVERCRAFT   4
#define VCTYPE_CAR      5


// vehicle clip point types

#define VCLIP_BODY          10
#define VCLIP_DRIVE_LEFT    30
#define VCLIP_DRIVE_RIGHT   31
#define VCLIP_HOVER         40


// RPM = revolutions per minute
// RPS = radians per second

#define RPM_TO_RPS(x) ((x) * (PI / 30.0f))
#define RPS_TO_RPM(x) ((x) * (30.0f / PI))

// MPS = metres per second
// KPH = kilometres per hour
// MPH = miles per hour

#define MPS_TO_MPH(x) ((x) * 2.23693629f) // thanks Google!
#define MPS_TO_KPH(x) ((x) * 3.6f)

#define KPH_TO_MPH(x) ((x) * 0.62f)
#define KPH_TO_MPS(x) ((x) / 3.6f)

// Starting position in degrees, measured counter-clockwise from the x-axis.
#define MPH_ZERO_DEG 210
#define KPH_ZERO_DEG 220

// Degrees to rotate the speedo needle for each unit of speed
#define DEG_PER_MPH 1.5f
#define DEG_PER_KPH 1.0f

// Multiplier for mps to speed in some unit
const float MPS_MPH_SPEED_MULT = 2.23693629f;
const float MPS_KPH_SPEED_MULT = 3.6f;

// Multiplier for mps to degs on the speedo dial
const float MPS_MPH_DEG_MULT = MPS_MPH_SPEED_MULT * DEG_PER_MPH;
const float MPS_KPH_DEG_MULT = MPS_KPH_SPEED_MULT * DEG_PER_KPH;


struct v_control_s {
  // shared
  float throttle;
  float brake1,brake2;
  vec3f turn;
  vec2f aim;

  // helicopter
  float collective;

  // -- utility --

  void setZero() {
    throttle = 0.0f;
    brake1 = 0.0f;
    brake2 = 0.0f;
    turn = vec3f::zero();
    aim = vec2f::zero();
    collective = 0.0f;
  }
  
  void setDefaultRates() {
    throttle = 10.0f;
    brake1 = 10.0f;
    brake2 = 10.0f;
    turn = vec3f(10.0f,10.0f,10.0f);
    aim = vec2f(10.0f,10.0f);
    collective = 10.0f;
  }
  
  void clamp() {
    CLAMP(throttle, -1.0f, 1.0f);
    CLAMP(brake1, 0.0f, 1.0f);
    CLAMP(brake2, 0.0f, 1.0f);
    CLAMP(turn.x, -1.0f, 1.0f);
    CLAMP(turn.y, -1.0f, 1.0f);
    CLAMP(turn.z, -1.0f, 1.0f);
    CLAMP(aim.x, -1.0f, 1.0f);
    CLAMP(aim.y, -1.0f, 1.0f);
    CLAMP(collective, -1.0f, 1.0f);
  }
};

typedef v_control_s v_state_s;


class PDriveSystem {
private:
  std::vector<vec2f> powercurve;
  
  std::vector<vec2f> gear;
  
  float gearch_first, gearch_repeat;
  
  float minRPS, maxRPS;
  
protected:
  float getPowerAtRPS(float rps);
  
public:
  PDriveSystem() :
    gearch_first(0.4f),
    gearch_repeat(0.15f),
    minRPS(10000000.0f),
    maxRPS(0.0f) { }
  
  void addPowerCurvePoint(float rpm, float power) {
    if (rpm <= 0.0f) return;
    
    float rps = RPM_TO_RPS(rpm);
    
    powercurve.push_back(vec2f(rps, power));
    
    if (minRPS > rps) minRPS = rps;
    if (maxRPS < rps) maxRPS = rps;
  }
  
  void addGear(float ratio) {
    if (hasGears()) {
      if (ratio <= getLastGearRatio()) return;
    } else {
      if (ratio <= 0.0f) return;
    }
    gear.push_back(vec2f(ratio, 1.0f / ratio));
  }
  
  bool hasGears() { return !gear.empty(); }
  float getLastGearRatio() { return gear.back().x; }
  
  friend class PDriveSystemInstance;
};

class PDriveSystemInstance {
private:
  
  PDriveSystem *dsys;
  
  float rps;
  
  int currentgear, targetgear_rel;
  float gearch;
  
  bool reverse;
  
  float out_torque;
  
  bool flag_gearchange;
  
public:
  PDriveSystemInstance(PDriveSystem *system) :
    dsys(system),
    currentgear(0),
    targetgear_rel(0),
    gearch(0.0f),
    reverse(false),
    out_torque(0.0f),
    flag_gearchange(false) { }
  
  void tick(float delta, float throttle, float wheel_rps);
  
  float getOutputTorque() { return out_torque; }
  
  float getEngineRPS() { return rps; }
  float getEngineRPM() { return RPS_TO_RPM(rps); }
  
  int getCurrentGear() { return reverse ? -1 : currentgear; }
  
  bool getFlagGearChange() {
    bool ret = flag_gearchange;
    flag_gearchange = false;
    return ret;
  }
  
  void doReset() {
    rps = dsys->minRPS;
    currentgear = 0;
    targetgear_rel = 0;
    gearch = 0.0f;
    out_torque = 0.0f;
  }
};


struct vehicle_clip_s {
  vec3f pt;
  int type;
  float force, dampening;
};


struct PVehicleTypeWheel {
  vec3f pt;
  float radius;
  float drive, steer, brake1, brake2;
  float force, dampening;
};


struct PVehicleTypePart {
  std::string name, parentname;
  int parent;
  
  PReferenceFrame ref_local;
  
  std::vector<vehicle_clip_s> clip;
  
  std::vector<PVehicleTypeWheel> wheel;
  
  std::vector<PReferenceFrame> flame;
  
  float scale;
  PModel *model;
};


class PVehicleType : public PResource {
public:
  std::string proper_name;
  std::string proper_class;
  
  // Pseudo Statistics
  std::string pstat_weightkg;
  std::string pstat_enginebhp;
  std::string pstat_wheeldrive;
  std::string pstat_handling;

  /*! Each possible wheel drive type */
  enum WheelDriveType
  {
     WHEEL_DRIVE_TYPE_FWD,
     WHEEL_DRIVE_TYPE_RWD,
     WHEEL_DRIVE_TYPE_4WD
  };

  WheelDriveType wheeldrive; /**< Vehicle's wheel drive type */
  
  int coretype;
  
  float mass; /**< Vehicle's mass */
  float normalbrake; /**< Normal (foot) brake value */
  float handbrake;   /**< Hand brake value */

  /*! Suspension parameters. See function Vehicle::addWheels for 
   * explanation of each ones. */
  struct SuspensionInfo {
     float stiffness;
     float compressionk;
     float relaxationk;
     float restlength;
  };

  SuspensionInfo suspension;
  
  std::vector<PVehicleTypePart> part;
  
  float wheelscale;
  PModel *wheelmodel;
  
  PDriveSystem dsys;
  
  float inverse_drive_total;
  
public:
  struct {
    // shared
    float speed;
    vec3f turnspeed;
    float turnspeed_a, turnspeed_b; // turnspeed = a + b * speed
    vec3f drag;
    float angdrag;
    vec2f lift; // x = fin lift (hz), y = wing lift (vt)
    vec2f fineffect; // x = rudder/fin (hz), y = tail (vt)
  } param;

  v_control_s ctrlrate;

public:
  PVehicleType() { }
  ~PVehicleType() { unload(); }

public:
  bool load(const std::string &filename, PSSModel &ssModel);
  void unload();
};



struct PVehicleWheel {
  float steering;  /**< Direction angle in degrees */
  float rotation;  /**< Spinning angle in degrees */
  vec3f pos;       /**< Wheel position */
  
  float skidding, dirtthrow;
  vec3f dirtthrowpos, dirtthrowvec;
  
  float bumplast, bumpnext, bumptravel;
  
  PVehicleWheel() {
    reset();
  }

  void reset() {
    rotation = 0.0f;
    steering = 0.0f;
    pos = vec3f(0.0f, 0.0f, 0.0f);
    bumplast = 0.0f;
    bumpnext = 0.0f;
    bumptravel = 0.0f;
  }
};


struct PVehiclePart {

  // ref_local is initted from vehicle type, but may change per-vehicle

  PReferenceFrame ref_local, ref_world;
  
  std::vector<PVehicleWheel> wheel;
};



class PVehicle {
//class PVehicle : public NetObject {
//typedef NetObject Parent;
  
public:
  PSim &sim;
  
  PVehicleType *type;

  std::vector<PVehiclePart> part;
  
  v_state_s state;
  
  PDriveSystemInstance dsysi;
  
  // helicopter-specific
  float blade_ang1;
  
  int nextcp;
  int nextcdcp; ///< Next codriver checkpoint.
  int currentlap; ///< Current lap, counted from 1.

#if 0
  // for vehicle resetting, after being flipped
  float reset_trigger_time;
  vec3f reset_pos;
  quatf reset_ori;
  float reset_time;
#endif
  
  // for body crash/impact noises
  float crunch_level, crunch_level_prev;
  
public:
  v_control_s ctrl;
  
  // info
  float skid_level;
  
public:
/*  
  enum StateBits {
    InitialBit = BIT(0),
    KinematicsBit = BIT(1),
    ControlBit = BIT(2),
  };
*/
public:
  PVehicle(PSim &sim_parent, PVehicleType *_type);
  ~PVehicle();

  /*! Force set current vehicle position and orientation
   * \param ps vector with new position
   * \param ori quaternion with new rotation */
  void setPositionAndOrientation(const vec3f& pos, const quatf& ori);

  /*! \return vector with current vehicle position */
  const vec3f& getPosition() const { return curReference.pos;};

  /*! \return quaternion with current vehicle rotations */
  const quatf& getOrientation() const { return curReference.ori; };

  /*! \return current reference frame */
  PReferenceFrame getReferenceFrame() { return curReference; };

  /*! Force a the debug draw of bullet's representation of this vehicle */
  void debugDraw();

public:
  /*
  // NetObject stuff
  void performScopeQuery(GhostConnection *connection);
  U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
  void unpackUpdate(GhostConnection *connection, BitStream *stream);
  */
  
  
  void tick(float delta);
  
  bool canHaveDustTrail();
  
  void updateParts();
  
  void doReset();
  void doReset2(const vec3f &pos, const quatf &ori);
  
  float getEngineRPM() { return dsysi.getEngineRPM(); }
  int getCurrentGear() { return dsysi.getCurrentGear(); }
  bool getFlagGearChange() { return dsysi.getFlagGearChange(); }
  float getCrashNoiseLevel() {
    if (crunch_level > crunch_level_prev) {
      float tmp = crunch_level - crunch_level_prev;
      crunch_level_prev = crunch_level;
      return tmp;
    } else {
      return 0.0f;
    }
  }
  /*! \return current vehicle speed, in km/h */
  const float getSpeed() const { return vehicle->getCurrentSpeedKmHour(); }
  float getSkidLevel() { return skid_level; }

private:

  /*! \return angular speed of a wheel which receives engine 
   * transmission power */
  float getWheelAngularSpeed(float delta) const;

  /* note: bullet physics for vehicle in Trigger was based on Bullet's
   * CarHandlingDemo as a starting point (also using Kester Maddock's
   * Vehicle Simulation with Bullet documentation). */

  /*! Create the vehicle implementation for Bullet Physics integration */
  void createBulletVehicle();

  /*! Create the rigid body for the chassis.
   * \param the chassis shape (in fact, the compound used as center of gravity
   *        for better vehicle handle). */
  void createChassisRigidBodyFromShape(btCollisionShape* chassisShape);

  /*! Add the wheels to bullet's vehicle.
   * \param halfExtends chassis' half sizes
   * \param tuning the vehicle tuning used */
  void addWheels(btVector3* halfExtents,
        btRaycastVehicle::btVehicleTuning tuning);

  
  btRaycastVehicle* vehicle; /**< The bullet vehicle approximation */
  btVehicleRaycaster* vehicleRayCaster; /**< Raycaster used */
  btRigidBody* chassisRigidBody; /**< Rigid body for vehicle's chassis */
  /*! Collision shapes of the vehicle (ie: chassis, wheels) */
  btAlignedObjectArray<btCollisionShape*> collisionShapes;
  btDefaultMotionState* chassisMotionState; /**< Motion state for chassis */
  btScalar wheelWidth;     /**< Current wheel width */
  btScalar wheelRadius;    /**< Current wheel radius */
  btScalar wheelPerimeter; /**< wheelRadius * 2 * M_PI */

  /*! Current position and orientation. Note: kept at a PReferenceFrame
   * for compatibility with the way it was implemented for camera retrieve
   * informations about vehicle. */
  PReferenceFrame curReference;

};
