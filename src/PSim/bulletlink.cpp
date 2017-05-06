// bulletlink.h

// Copyright Farrer farrer@dnteam.org
// License: GPL version 2 (see included gpl.txt)


#include "bulletlink.h"
#include <assert.h>

/***********************************************************************
 *                     bulletLinkTickCallback                          *
 ***********************************************************************/
void bulletLinkTickCallback(btDynamicsWorld *world, btScalar timeStep) {
   BulletLink::tickCallBack();
}

/***********************************************************************
 *                          createBulletWorld                          *
 ***********************************************************************/
void BulletLink::createBulletWorld() {
  assert(dynamicsWorld == NULL);
  collisionConfiguration = new btDefaultCollisionConfiguration();
  dispatcher = new btCollisionDispatcher(collisionConfiguration);
  broadPhase = new btDbvtBroadphase();
  solver = new btSequentialImpulseConstraintSolver();
  dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadPhase, 
        solver,collisionConfiguration);
  dynamicsWorld->setGravity(btVector3(0, 0, -9.8f));
  //dynamicsWorld->setInternalTickCallback(bulletLinkTickCallback);
}

/***********************************************************************
 *                        deleteBulletWorld                            *
 ***********************************************************************/
void BulletLink::deleteBulletWorld() {
  assert(dynamicsWorld != NULL);
  delete dynamicsWorld;
  dynamicsWorld = NULL;
  delete solver;
  delete dispatcher;
  delete collisionConfiguration;
  delete broadPhase;
}

/***********************************************************************
 *                          addRigidBody                               *
 ***********************************************************************/
void BulletLink::addRigidBody(btRigidBody* rigidBody) {
  dynamicsWorld->addRigidBody(rigidBody);
}

/***********************************************************************
 *                        removeRigidBody                              *
 ***********************************************************************/
void BulletLink::removeRigidBody(btRigidBody* rigidBody) {
  dynamicsWorld->removeRigidBody(rigidBody);
}

/***********************************************************************
 *                              step                                   *
 ***********************************************************************/
void BulletLink::step(btScalar timeStep, int maxSubSteps) {
  /* Convert the step time from ms to bullet seconds. */
  btScalar stepInSeconds = timeStep / 1000.0f;

  /* Finally, do the physics step */
  dynamicsWorld->stepSimulation(stepInSeconds, maxSubSteps, 
        TRIGGER_BULLET_FREQUENCY);

  debugDraw();
}

/***********************************************************************
 *                          tickCallBack                               *
 ***********************************************************************/
void BulletLink::tickCallBack() {
  //TODO: check if this is necessary for trigger or not.
}

/***********************************************************************
 *                            debugDraw                                *
 ***********************************************************************/
void BulletLink::debugDraw() {
  //TODO
#if 0
  if(bulletDebugDraw != NULL)
  {
    dynamicsWorld->debugDrawWorld();
    bulletDebugDraw->update();
  }
#endif
}

/***********************************************************************
 *                           static fields                             *
 ***********************************************************************/
btBroadphaseInterface* BulletLink::broadPhase = NULL; 
btDefaultCollisionConfiguration* BulletLink::collisionConfiguration = NULL; 
btCollisionDispatcher* BulletLink::dispatcher = NULL;
btSequentialImpulseConstraintSolver* BulletLink::solver = NULL;
btDiscreteDynamicsWorld* BulletLink::dynamicsWorld = NULL;
//BulletDebugDraw* BulletLink::bulletDebugDraw = NULL;
//
