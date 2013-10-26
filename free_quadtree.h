#ifndef freequadtreeH
#define freequadtreeH

#include <vector>
#include <atomic>
//#include <memory>
#include "quadtree.h"

namespace quadtree 
{
class LockfreeQuadtree : public Quadtree
{
public:
  // @todo create destructor, that deletes all the newed children
  LockfreeQuadtree(BoundingBox boundary, int capacity);
  virtual ~LockfreeQuadtree() {}

  virtual bool               Insert(const Point& p);
  virtual std::vector<Point> Query(const BoundingBox&);
  virtual BoundingBox        Boundary() {return boundary;}
private:
  LockfreeQuadtree();

  void subdivide();
  void disperse();
  
  BoundingBox boundary; ///< @todo change to shared_ptr ?
  std::atomic<PointList*> points;
  std::atomic<LockfreeQuadtree*> Nw;
  std::atomic<LockfreeQuadtree*> Ne;
  std::atomic<LockfreeQuadtree*> Sw;
  std::atomic<LockfreeQuadtree*> Se;
};
}
#endif // quadtreeH
