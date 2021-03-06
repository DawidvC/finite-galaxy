// BatchDrawList.h

#ifndef BATCH_DRAW_LIST_H_
#define BATCH_DRAW_LIST_H_

#include "Point.h"

#include <map>
#include <vector>

class Body;
class Sprite;



// This class collects a set of OpenGL draw commands to issue and groups them by
// sprite, so all instances of each sprite can be drawn with a single command.
class BatchDrawList {
public:
  // Clear the list, also setting the global time step for animation.
  void Clear(int step = 0, double zoom = 1.);
  void SetCentre(const Point &centre);

  // Add an object based on the Body class.
  bool Add(const Body &body, float clip = 1.);

  // Draw all the items in this list.
  void Draw() const;


private:
  bool Cull(const Body &body, const Point &position) const;


private:
  int step = 0;
  double zoom = 1.;
  bool isHighDPI = false;
  Point centre;

  // Each sprite consists of six vertices (four vertices to form a quad and
  // two dummy vertices to mark the break in between them). Each of those
  // vertices has five attributes: (x, y) position in pixels, (s, t) texture
  // coordinates, and the index of the sprite frame.
  std::map<const Sprite *, std::vector<float>> data;
};



#endif
