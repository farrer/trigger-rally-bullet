// bulletlink.h

// Copyright Farrer farrer@dnteam.org
// License: GPL version 2 (see included gpl.txt)


#ifndef _trigger_bulletlink_h
#define _trigger_bulletlink_h

#include <btBulletDynamicsCommon.h>
#include "bulletdebugdraw.h"

#define TRIGGER_BULLET_FREQUENCY (1.0f / 60.0f)

/*! Class that make the link with bullet, initing it, setting the world, 
 * receiveing callback, etc.
 * \note it's implemented as an static class. */
class BulletLink
{
   public:
      /*! Create the bullet world to use */
      static void createBulletWorld();

      /*! Delete the created bulled world */
      static void deleteBulletWorld();

      /*! The callback for bullet tick */
      static void tickCallBack();

      /*! Add rigid body to the world
       * \param rigidBody -> pointer to the rigid body to add */
      static void addRigidBody(btRigidBody* rigidBody);

      /*! Remove rigid body from the world
       * \param rigidBody -> pointer to the rigid body to remove */
      static void removeRigidBody(btRigidBody* rigidBody);

      /*! Add a vehicle to the world
       * \param vehicle pointer of the vehicle to add */
      static void addVehicle(btRaycastVehicle* vehicle);

      /*! Remove a vehicle from the world
       * \param vehicle pointer to the vehicle to remove */
      static void removeVehicle(btRaycastVehicle* vehicle);

      /*! \return pointer to a new default vehicle raycaster */
      static btVehicleRaycaster* createVehicleRaycaster();

      /*! Do the draw of current physics for debug. */
      static void debugDraw();

      /*! Step the rigid body
       * \param timeStep -> current time of this step (in seconds).
       * \param maxSubSteps -> numer of bullet sub steps */
      static void step(btScalar timeStep, int maxSubSteps);

      /*! \return if the bullet link was already created or not */
      static bool isCreated() { return dynamicsWorld != NULL; };

   private:
      BulletLink(){};

      static btBroadphaseInterface* broadPhase; 
      static btDefaultCollisionConfiguration* collisionConfiguration;
      static btCollisionDispatcher* dispatcher;
      static btSequentialImpulseConstraintSolver* solver;
      static btDiscreteDynamicsWorld* dynamicsWorld;
      static BulletDebugDraw* bulletDebugDraw;
};

#endif

