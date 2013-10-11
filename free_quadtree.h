#ifndef freequadtreeH
#define freequadtreeH

#include <vector>
#include "quadtree.h"

namespace quadtree 
{
class LockfreeQuadtree : public Quadtree
{
public:
  LockfreeQuadtree(BoundingBox boundary, int capacity);
  virtual bool Insert(const Point& p);
  virtual std::vector<Point> Query(const BoundingBox&);
  virtual BoundingBox Boundary() {return boundary;}
private:
  LockfreeQuadtree();
  PointList* points;
  BoundingBox boundary;
  LockfreeQuadtree* Nw;
  LockfreeQuadtree* Ne;
  LockfreeQuadtree* Sw;
  LockfreeQuadtree* Se;
};

}

#endif // quadtreeH
