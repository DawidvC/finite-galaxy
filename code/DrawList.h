// DrawList.h

#ifndef DRAW_LIST_H_
#define DRAW_LIST_H_

#include "Point.h"
#include "SpriteShader.h"

#include <cstdint>
#include <vector>

class Body;
class Sprite;



// Class for storing a list of textures to blit to the screen. This allows the
// work of calculating the transformation matrices to be done in a separate
// thread from the graphics thread. However, the SpriteShader class is also
// available for drawing individual sprites in contexts where putting them into
// a DrawList first does not make sense.
class DrawList {
public:
  // Clear the list, also setting the global time step for animation.
  void Clear(int step = 0, double zoom = 1.);
  void SetCentre(const Point &centre, const Point &centreVelocity = Point());

  // Add an object based on the Body class.
  bool Add(const Body &body, double cloak = 0.);
  bool Add(const Body &body, Point position);
  bool AddUnblurred(const Body &body);
  bool AddProjectile(const Body &body, const Point &adjustedVelocity, double clip);
  bool AddSwizzled(const Body &body, int swizzle);

  // Draw all the items in this list.
  void Draw() const;


private:
  bool Cull(const Body &body, const Point &position, const Point &blur) const;

  void Push(const Body &body, Point pos, Point blur, double cloak, double clip, int swizzle);


private:
  int step = 0;
  double zoom = 1.;
  bool isHighDPI = false;
  std::vector<SpriteShader::Item> items;

  Point centre;
  Point centreVelocity;
};



#endif
