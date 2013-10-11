#include <iostream>
#include <vector>
#include "quadtree.h"
#include "free_quadtree.h"
//#include <atomic>

/// this should be in stdatomic.h. Defining here because clang doesn't have stdatomic.h
enum memory_order {
  memory_order_relaxed,
  memory_order_consume,
  memory_order_acquire,
  memory_order_release,
  memory_order_acq_rel,
  memory_order_seq_cst
};

namespace
{
using std::vector;
using std::cout;
using std::endl;
}

namespace quadtree
{
LockfreeQuadtree::LockfreeQuadtree(BoundingBox boundary_, int capacity_)
  : points(new PointList(capacity_))
  , boundary(boundary_)
  , Nw(NULL)
  , Ne(NULL)
  , Sw(NULL)
  , Se(NULL)
{}

bool LockfreeQuadtree::Insert(const Point& p)
{
  return true;
}

vector<Point> LockfreeQuadtree::Query(const BoundingBox& b)
{
  vector<Point> found;

  if(!boundary.Intersects(b))
    return found;

  PointList localPoints = __c11_atomic_load(points, memory_order_relaxed);

  return found;
}
}

using namespace quadtree; ///< @todo remove this

int main() {
  BoundingBox b = {{100, 100}, {50, 50}};
  LockfreeQuadtree q(b, 4);
  Point p;
  cout << q.Insert(p) << endl;
  return 0;
}
