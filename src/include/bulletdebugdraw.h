// bulletdebugdraw.h

// Copyright Farrer farrer@dnteam.org
// License: GPL version 2 (see included gpl.txt)

#ifndef _trigger_rally_debug_draw_h
#define _trigger_rally_debug_draw_h

#include <btBulletDynamicsCommon.h>

/*! A implementation in GL of the Bullet Debug Draw interface */
class BulletDebugDraw: public btIDebugDraw {
   public:
      BulletDebugDraw();
      ~BulletDebugDraw();
      virtual void drawLine(const btVector3 &from, const btVector3 &to, 
            const btVector3 &color);
      virtual void drawTriangle(const btVector3 &v0, const btVector3 &v1, 
            const btVector3 &v2, const btVector3 &color, btScalar);
      virtual void drawContactPoint(const btVector3 &PointOnB, 
            const btVector3 &normalOnB, btScalar distance, int lifeTime, 
            const btVector3 &color);
      virtual void reportErrorWarning(const char *warningString);
      virtual void draw3dText(const btVector3 &location, 
            const char *textString);
      virtual void setDebugMode(int debugMode);
      virtual int getDebugMode() const;

      void update();
   private:
      DebugDrawModes               mDebugModes;
};


#endif


