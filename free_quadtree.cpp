#include <iostream>
#include <vector>
#include "quadtree.h"
#include "free_quadtree.h"
#include <atomic>
#include <memory>

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
//  cout << "insert()" << endl;
  if(!boundary.Contains(p))
  {
//    cout << p.String() << " outside " << boundary.String() << endl;
    return false;
  }

  while(true)
  {
    PointList* oldPoints = points.load();
    if(oldPoints == nullptr || oldPoints->Length >= oldPoints->Capacity)
      break;
    PointList* newPoints = new PointList(oldPoints->Capacity);
    newPoints->First = new PointListNode(p, newPoints->First);
    newPoints->Length = oldPoints->Length + 1;
    newPoints->Capacity = oldPoints->Capacity;
    const bool ok = points.compare_exchange_strong(oldPoints, newPoints);
    if(ok)
    {
      while(points.load() == oldPoints)
	cout << "insert ll" << endl;
//      delete oldPoints;
      return true;
    }
  }

//  cout << "sdv" << endl;

  PointList* oldPoints = points.load();
  if(oldPoints != nullptr && oldPoints->Capacity == 0)
    cout << "subdividing with 0 capacity" << endl;

  PointList* localPoints = points.load();
  if(localPoints != nullptr)
    subdivide();

  while(Nw.load() == nullptr)
    cout << "insert nwll" << endl;
  while(Ne.load() == nullptr)
    cout << "insert nell" << endl;
  while(Sw.load() == nullptr)
    cout << "insert swll" << endl;
  while(Se.load() == nullptr)
    cout << "insert sell" << endl;

  LockfreeQuadtree* nw = Nw.load();
  LockfreeQuadtree* ne = Ne.load();
  LockfreeQuadtree* sw = Sw.load();
  LockfreeQuadtree* se = Se.load();
  const bool ok = nw->Insert(p) || ne->Insert(p) || sw->Insert(p) || se->Insert(p);
  if(!ok)
  {
    cout << "insert failed" << endl;
    cout << p.String() << " inside " << boundary.String() << " but outside " << endl;
    cout << nw->Boundary().String() << " and " << endl;
    cout << ne->Boundary().String() << " and " << endl;
    cout << sw->Boundary().String() << " and " << endl;
    cout << se->Boundary().String() << endl << endl;
  }
  return ok;

//  return true;
}

void LockfreeQuadtree::subdivide()
{

  PointList* oldPoints = points.load();
  if(oldPoints == nullptr)
    return;

  const auto capacity = oldPoints->Capacity;
  if(capacity == 0)
    cout << "in subdivide with 0 capacity" << endl;

  const Point newHalf = {boundary.HalfDimension.X / 2.0, boundary.HalfDimension.Y / 2.0}; 
  LockfreeQuadtree* lval = nullptr;

  Point newCenter = {boundary.Center.X - boundary.HalfDimension.X/2.0, boundary.Center.Y - boundary.HalfDimension.Y/2.0};
  BoundingBox newBoundary = {newCenter, newHalf};
  LockfreeQuadtree* q = new LockfreeQuadtree(newBoundary, capacity);
  const bool nwOk = Nw.compare_exchange_strong(lval, q);
  if(!nwOk)
    delete q;

  newCenter = {boundary.Center.X + boundary.HalfDimension.X/2.0, boundary.Center.Y - boundary.HalfDimension.Y/2.0};
  newBoundary = {newCenter, newHalf};
  q = new LockfreeQuadtree(newBoundary, capacity);
  const bool neOk = Ne.compare_exchange_strong(lval, q);
  if(!neOk)
    delete q;

  newCenter = {boundary.Center.X + boundary.HalfDimension.X/2.0, boundary.Center.Y + boundary.HalfDimension.Y/2.0};
  newBoundary = {newCenter, newHalf};
  q = new LockfreeQuadtree(newBoundary, capacity);
  const bool seOk = Se.compare_exchange_strong(lval, q);
  if(!seOk)
    delete q;

  newCenter = {boundary.Center.X - boundary.HalfDimension.X/2.0, boundary.Center.Y + boundary.HalfDimension.Y/2.0};
  newBoundary = {newCenter, newHalf};
  q = new LockfreeQuadtree(newBoundary, capacity);
  const bool swOk = Sw.compare_exchange_strong(lval, q);
  if(!swOk)
    delete q;

  // Another thread may have been in the middle of setting it, so our set failed, but the other thread isn't done yet.
  while(Nw.load() == nullptr);
//    cout << "subdivide nwll" << endl;
  while(Ne.load() == nullptr);
//    cout << "subdivide nell" << endl;
  while(Sw.load() == nullptr);
//    cout << "subdivide swll" << endl;
  while(Se.load() == nullptr);
//    cout << "subdivide sell" << endl;

//  disperse();
}

void LockfreeQuadtree::disperse()
{
  PointList* oldPoints;
  while(true)
  {
    oldPoints = points.load();
    if(oldPoints == nullptr || oldPoints->Length == 0)
      break;
    PointList* newPoints = new PointList(oldPoints->Capacity);
    Point p = newPoints->First->NodePoint;
    newPoints->First = newPoints->First->Next;
    --newPoints->Length;

    /// @todo we must atomically swap the new points, and insert the point into the child.
    ///       we can do this by making Query() help in the dispersal
    bool ok = points.compare_exchange_strong(oldPoints, newPoints);
    if(!ok)
      continue;
//    else
//      delete oldPoints;

    LockfreeQuadtree* nw = Nw.load();
    LockfreeQuadtree* ne = Ne.load();
    LockfreeQuadtree* se = Se.load();
    LockfreeQuadtree* sw = Sw.load();
    if(nw == nullptr || ne == nullptr || sw == nullptr || se == nullptr)
      cout << "subtree null" << endl;
    ok = nw->Insert(p) || ne->Insert(p) || sw->Insert(p) || se->Insert(p);
  }
  if(oldPoints != nullptr)
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
