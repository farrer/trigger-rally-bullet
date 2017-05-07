// bulletlink.cpp

// Copyright Farrer farrer@dnteam.org
// License: GPL version 2 (see included gpl.txt)


#include "bulletlink.h"
#include "bulletdebugdraw.h"
#include "config.h"
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
#if RENDER_DEBUG_BULLET
  bulletDebugDraw = new BulletDebugDraw(); 
  bulletDebugDraw->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
  dynamicsWorld->setDebugDrawer(bulletDebugDraw);
#else
  bulletDebugDraw = NULL;
#endif
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
  if(bulletDebugDraw) {
    delete bulletDebugDraw;
    bulletDebugDraw = NULL;
  }
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
  dynamicsWorld->stepSimulation(timeStep, maxSubSteps, 
        TRIGGER_BULLET_FREQUENCY);
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
#if RENDER_DEBUG_BULLET
  dynamicsWorld->debugDrawWorld();
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
BulletDebugDraw* BulletLink::bulletDebugDraw = NULL;

