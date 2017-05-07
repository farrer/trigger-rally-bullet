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

  unsigned int i = lines.size();
  lines.resize(lines.size() + 1);
  lines[i].from = from;
  lines[i].to = to;
  lines[i].color = color;
}

/***********************************************************************
 *                           drawTriangle                              *
 ***********************************************************************/
void BulletDebugDraw::drawTriangle(const btVector3 &v0, const btVector3 &v1,
      const btVector3 &v2, const btVector3 &color, btScalar alpha) {

  unsigned int i = triangles.size();
  triangles.resize(triangles.size() + 1);
  triangles[i].color = color;
  triangles[i].alpha = alpha;
  triangles[i].v0 = v0;
  triangles[i].v1 = v1;
  triangles[i].v2 = v2;
}

/***********************************************************************
 *                               render                                *
 ***********************************************************************/
void BulletDebugDraw::render()
{
  /* Render our current lines */
  if(lines.size() > 0) {
    glBegin(GL_LINES);
  }
  for(unsigned int i = 0; i < lines.size(); i++) {
    glColor4f(lines[i].color[0], lines[i].color[1], lines[i].color[2], 1.0f);
    glVertex3f(lines[i].from[0], lines[i].from[1], lines[i].from[2]);
    glVertex3f(lines[i].to[0], lines[i].to[1], lines[i].to[2]);
  }
  if(lines.size() > 0) {
    glEnd();
    lines.clear();
  }

  /* Render our current triangles */
  if(triangles.size() > 0) {
    glBegin(GL_TRIANGLES);
  }
  for(unsigned int i = 0; i < triangles.size(); i++) {
     glColor4f(triangles[i].color[0], triangles[i].color[1], 
               triangles[i].color[2], triangles[i].alpha);
     glVertex3f(triangles[i].v0[0], triangles[i].v0[1], triangles[i].v0[2]);
     glVertex3f(triangles[i].v1[0], triangles[i].v1[1], triangles[i].v1[2]);
     glVertex3f(triangles[i].v2[0], triangles[i].v2[1], triangles[i].v2[2]);
  }
  if(triangles.size() > 0) {
    glEnd();
    triangles.clear();
  }
}

/***********************************************************************
 *                           setDebugMode                              *
 ***********************************************************************/
void BulletDebugDraw::setDebugMode(int debugMode) {
   curDebugMode = (DebugDrawModes) debugMode;
}

/***********************************************************************
 *                           setDebugMode                              *
 ***********************************************************************/
int BulletDebugDraw::getDebugMode() const {
   return curDebugMode;
}

/***********************************************************************
 *                        reportErrorWarning                           *
 ***********************************************************************/
void BulletDebugDraw::reportErrorWarning(const char *warningString) {
   PUtil::outLog() << warningString << std::endl;
}

/***********************************************************************
 *                        Unused Functions                             *
 ***********************************************************************/
void BulletDebugDraw::drawContactPoint(const btVector3 &PointOnB, 
      const btVector3 &normalOnB, btScalar distance, int lifeTime, 
      const btVector3 &color ) {
}
void BulletDebugDraw::draw3dText( const btVector3 &location, 
      const char *textString) {
}


