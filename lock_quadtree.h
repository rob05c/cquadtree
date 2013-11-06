#ifndef lockquadtreeH
#define lockquadtreeH

#include <vector>
#include <mutex>
//#include <memory>
#include "quadtree.h"


namespace quadtree 
{
class LockQuadtree : public Quadtree
{
public:
  // @todo create destructor, that deletes all the newed children
  LockQuadtree(BoundingBox boundary, size_t capacity);
  virtual ~LockQuadtree() {}

  virtual bool               Insert(const Point& p);
  virtual std::vector<Point> Query(const BoundingBox&);
  virtual BoundingBox        Boundary() {return boundary;}
  bool ThreadCanComplete(); ///< @todo not sure I like this. Make gc() call until success on each func?

  BoundingBox boundary; ///< @todo change to shared_ptr ?

  // @todo rename these and vars, swap case
  // this are here so the gui can get their boundaries.
  LockQuadtree* nw() {return Nw;}
  LockQuadtree* ne() {return Ne;}
  LockQuadtree* sw() {return Sw;}
  LockQuadtree* se() {return Se;}

private:
  LockQuadtree();

  std::mutex pointsMutex;
  std::vector<Point> points;
  size_t capacity;
  LockQuadtree* Nw;
  LockQuadtree* Ne;
  LockQuadtree* Sw;
  LockQuadtree* Se;

  void subdivide();
  void disperse();
};
}
#endif // quadtreeH
