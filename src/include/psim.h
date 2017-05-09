
// psim.h [psim]

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)


#include <pengine.h>

/*
#include <tnl/tnl.h>
#include <tnl/tnlNetBase.h>
#include <tnl/tnlGhostConnection.h>
#include <tnl/tnlNetInterface.h>
#include <tnl/tnlNetObject.h>
*/

class PSim;

class PMotor;
class PGearbox;

class ClipSet;
class ClipNode;
class ClipMesh;

class PVehicleType;
class PVehicleTypePart;
class PVehicle;



class PReferenceFrame {
public:
  vec3f pos;
  quatf ori;

  mat44f ori_mat, ori_mat_inv;

public:
  PReferenceFrame() : pos(vec3f::zero()), ori(QUATERNION_IDENTITY) {
    updateMatrices();
  }

  void updateMatrices() {
    ori.normalize();
    btTransform trans(ori);
    btScalar mat[16];
    trans.getOpenGLMatrix(mat);
    /* Note: as it will be only an orientation matrix, ignoring representation
     * of the position. */
    ori_mat.assemble(vec3f(mat[0], mat[4], mat[8]),
                     vec3f(mat[1], mat[5], mat[9]), 
                     vec3f(mat[2], mat[6], mat[10]));
    ori_mat_inv = ori_mat.transpose();
  }

  void setPosition(const vec3f &_pos) { pos = _pos; }
  vec3f getPosition() { return pos; }

  void setOrientation(const quatf &_ori) { ori = _ori; }
  quatf getOrientation() { return ori; }
  mat44f getOrientationMatrix() { return ori_mat; }
  mat44f getInverseOrientationMatrix() { return ori_mat_inv; }

  vec3f getLocToWorldVector(const vec3f &pt) {
    return ori_mat.transform1(pt);
  }
  vec3f getWorldToLocVector(const vec3f &pt) {
    return ori_mat.transform2(pt);
  }
  vec3f getLocToWorldPoint(const vec3f &pt) {
    return pos + ori_mat.transform1(pt);
  }
  vec3f getWorldToLocPoint(const vec3f &pt) {
    return ori_mat.transform2(pt - pos);
  }
};

#include "vehicle.h"

class PSim {
private:

  PTerrain *terrain;

  PResourceList<PVehicleType> vtypelist;

  std::vector<PVehicle *> vehicle;

  vec3f gravity;

public:
  PSim();
  ~PSim();

public:
  void setTerrain(PTerrain *_terrain) { terrain = _terrain; }

  void setGravity(const vec3f &_gravity) { gravity = _gravity; }

  PVehicleType *loadVehicleType(const std::string &filename, PSSModel &ssModel);

  PVehicle *createVehicle(XMLElement *element, const std::string &filepath, PSSModel &ssModel);
  PVehicle *createVehicle(const std::string &type, const vec3f &pos, const quatf &ori, const std::string &filepath, PSSModel &ssModel);
  PVehicle *createVehicle(PVehicleType *type, const vec3f &pos, const quatf &ori, PSSModel &ssModel);

  // Remove all bodies and vehicles
  void clear();

  // Step the simulation delta seconds
  void tick(float delta);


  PTerrain *getTerrain() { return terrain; }

public:

  friend class PRigidBody;
  friend class PVehicle;
};
