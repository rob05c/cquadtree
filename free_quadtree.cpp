#include <iostream>
#include <vector>
#include "quadtree.h"
#include "free_quadtree.h"
#include <atomic>
#include <memory>

/*
/// this should be in stdatomic.h. Defining here because clang doesn't have stdatomic.h
enum memory_order {
  memory_order_relaxed,
  memory_order_consume,
  memory_order_acquire,
  memory_order_release,
  memory_order_acq_rel,
  memory_order_seq_cst
};
*/

namespace
{
using std::vector;
using std::cout;
using std::endl;
using std::atomic;
}

namespace quadtree
{
LockfreeQuadtree::LockfreeQuadtree(BoundingBox boundary_, int capacity_)
  : boundary(boundary_)
  , points(new PointList(capacity_))
  , Nw(nullptr)
  , Ne(nullptr)
  , Sw(nullptr)
  , Se(nullptr)
{}

bool LockfreeQuadtree::Insert(const Point& p)
{
  if(!boundary.Contains(p))
  {
//    cout << "insert outside boundary" << endl;
    return false;
  }

  while(true)
  {
    PointList* oldPoints = points.load();
    if(oldPoints == nullptr || oldPoints->Length >= oldPoints->Capacity)
      break;
    PointList* newPoints = new PointList(*oldPoints);
    newPoints->First = new PointListNode(p, newPoints->First);
    ++newPoints->Length;

    const bool ok = points.compare_exchange_weak(oldPoints, newPoints);
    if(ok)
      return true;
  }

    PointList* localPoints = points.load();
    if(localPoints == nullptr)
      subdivide();

    LockfreeQuadtree* nw = Nw.load();
    LockfreeQuadtree* ne = Ne.load();
    LockfreeQuadtree* sw = Sw.load();
    LockfreeQuadtree* se = Se.load();
    const bool ok = nw->Insert(p) || ne->Insert(p) || sw->Insert(p) || se->Insert(p);
    if(!ok)
      cout << "insert failed" << endl;
    return ok;
}

void LockfreeQuadtree::subdivide()
{
  PointList* oldPoints = points.load();
  if(oldPoints == nullptr)
    return;
 
  LockfreeQuadtree* lval = nullptr;
  LockfreeQuadtree* q = new LockfreeQuadtree(boundary, oldPoints->Capacity);
  bool ok = Nw.compare_exchange_weak(lval, q);
  if(ok)
    q = new LockfreeQuadtree(boundary, oldPoints->Capacity);
  else
    lval = nullptr;

  ok = Ne.compare_exchange_weak(lval, q);
  if(ok)
    q = new LockfreeQuadtree(boundary, oldPoints->Capacity);
  else
    lval = nullptr;

  ok = Sw.compare_exchange_weak(lval, q);
  if(ok)
    q = new LockfreeQuadtree(boundary, oldPoints->Capacity);
  else
    lval = nullptr;

  ok = Se.compare_exchange_weak(lval, q);
  if(!ok)
    delete q;

  disperse();
}

void LockfreeQuadtree::disperse()
{
  while(true)
  {
    PointList* oldPoints = points.load();
    if(oldPoints == nullptr || oldPoints->Length == 0)
      break;

    PointList* newPoints = new PointList(*oldPoints);

    Point p = newPoints->First->NodePoint;
    newPoints->First = newPoints->First->Next;
    --newPoints->Length;

    /// @todo we must atomically swap the new points, and insert the point into the child.
    ///       we can do this by making Query() help in the dispersal
    bool ok = points.compare_exchange_weak(oldPoints, newPoints);
    if(!ok)
      continue;

    LockfreeQuadtree* nw = Nw.load();
    LockfreeQuadtree* ne = Ne.load();
    LockfreeQuadtree* sw = Sw.load();
    LockfreeQuadtree* se = Se.load();
    ok = nw->Insert(p) || ne->Insert(p) || sw->Insert(p) || se->Insert(p);
    if(!ok)
      cout << "disperse insert failed" << endl;
  }
  points.store(nullptr);
}


vector<Point> LockfreeQuadtree::Query(const BoundingBox& b)
{
  vector<Point> found;

  if(!boundary.Intersects(b))
    return found;

  PointList* localPoints = points.load();
  if(localPoints != nullptr)
  {
    for(auto node = localPoints->First; node != nullptr; node = node->Next)
    {
      if(b.Contains(node->NodePoint))
	found.push_back(node->NodePoint);
    }
  }
  if(Nw != nullptr)
  {
    LockfreeQuadtree* q = Nw.load();
    vector<Point> f = q->Query(b);
    found.insert(found.end(), f.begin(), f.end());
  }
  if(Ne != nullptr)
  {
    LockfreeQuadtree* q = Ne.load();
    vector<Point> f = q->Query(b);
    found.insert(found.end(), f.begin(), f.end());
  }
  if(Sw != nullptr)
  {
    LockfreeQuadtree* q = Sw.load();
    vector<Point> f = q->Query(b);
    found.insert(found.end(), f.begin(), f.end());
  }
  if(Se != nullptr)
  {
    LockfreeQuadtree* q = Se.load();
    vector<Point> f = q->Query(b);
    found.insert(found.end(), f.begin(), f.end());
  }
  return found;
}
}
