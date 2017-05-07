// bulletdebugdraw.cpp

// Copyright Farrer farrer@dnteam.org
// License: GPL version 2 (see included gpl.txt)

#include "bulletdebugdraw.h"
#include <SDL2/SDL.h>
#include <pengine.h>

/***********************************************************************
 *                             Constructor                             *
 ***********************************************************************/
BulletDebugDraw::BulletDebugDraw() {
}

/***********************************************************************
 *                              Destructor                             *
 ***********************************************************************/
BulletDebugDraw::~BulletDebugDraw() {
}

/***********************************************************************
 *                              drawLine                               *
 ***********************************************************************/
void BulletDebugDraw::drawLine(const btVector3 &from, const btVector3 &to, 
      const btVector3 &color) {

   glColor4f(color[0], color[1], color[2], 1.0f);
   glBegin(GL_LINES);
      glVertex3f(from[0], from[1], from[2]);
      glVertex3f(to[0], to[1], to[2]);
   glEnd();
}

/***********************************************************************
 *                           drawTriangle                              *
 ***********************************************************************/
void BulletDebugDraw::drawTriangle(const btVector3 &v0, const btVector3 &v1,
      const btVector3 &v2, const btVector3 &color, btScalar alpha) {
   
   glColor4f(color[0], color[1], color[2], alpha);
   glBegin(GL_TRIANGLES);
      glVertex3f(v0[0], v0[1], v0[2]);
      glVertex3f(v1[0], v1[1], v1[2]);
      glVertex3f(v2[0], v2[1], v2[2]);
   glEnd();
}

/***********************************************************************
 *                        Unused Functions                             *
 ***********************************************************************/
void BulletDebugDraw::drawContactPoint(const btVector3 &PointOnB, 
      const btVector3 &normalOnB, btScalar distance, int lifeTime, 
      const btVector3 &color ) {
}
void BulletDebugDraw::reportErrorWarning(const char *warningString) {
   PUtil::outLog() << warningString << std::endl;
}
void BulletDebugDraw::draw3dText( const btVector3 &location, 
      const char *textString) {
}
void BulletDebugDraw::setDebugMode(int debugMode) {
   mDebugModes = (DebugDrawModes) debugMode;
}
int BulletDebugDraw::getDebugMode() const {
   return mDebugModes;
}

