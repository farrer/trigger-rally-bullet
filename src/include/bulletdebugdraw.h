// bulletdebugdraw.h

// Copyright Farrer farrer@dnteam.org
// License: GPL version 2 (see included gpl.txt)

#ifndef _trigger_rally_debug_draw_h
#define _trigger_rally_debug_draw_h

#include <btBulletDynamicsCommon.h>
#include <vector>

/*! A implementation in GL of the Bullet Debug Draw interface.
 * \note only implemented line and triangle renderer, as it is what we need
 * to visualize on Trigger right now. */
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

      /*! Render current primitives to our gl scene */
      void render();
   private:

      /*! Struct to keep line information */
      struct Line {
        btVector3 from;
        btVector3 to;
        btVector3 color;
      };

      /*! Struct to keep triangle information */
      struct Triangle {
        btVector3 v0;
        btVector3 v1;
        btVector3 v2;
        btVector3 color;
        btScalar alpha;
      };

      DebugDrawModes curDebugMode; /**< Current debug draw mode */
      std::vector<Line> lines; /**< lines that should be rendered at frame */
      std::vector<Triangle> triangles; /**< triangles to render at frame */
};


#endif


