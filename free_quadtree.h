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
  bool ThreadCanComplete(); ///< @todo not sure I like this. Make gc() call until success on each func?

  BoundingBox boundary; ///< @todo change to shared_ptr ?

private:
  LockfreeQuadtree();

  std::atomic<PointList*> points;
  std::atomic<LockfreeQuadtree*> Nw;
  std::atomic<LockfreeQuadtree*> Ne;
  std::atomic<LockfreeQuadtree*> Sw;
  std::atomic<LockfreeQuadtree*> Se;


  void subdivide();
  void disperse();
  static void gc(); ///< this function is thread-specific. It collects garbage specific to the thread, not the tree.

  class HazardPointer
  {
  public:
    std::atomic<PointList*> Hazard; // this MUST be atomic. It could be half-changed then referenced // the hazardous pointer. Change to PointList?
    HazardPointer* Next;
  private:
    std::atomic_flag active;
  private:
    static std::atomic<HazardPointer*> head;
//    static std::atomic_size_t length;
  public:
    static HazardPointer* Head() {return head.load();}
    static HazardPointer* Acquire();
    static void Release(HazardPointer* p) {p->Hazard.store(nullptr);p->active.clear();}
  };
};
}
#endif // quadtreeH
