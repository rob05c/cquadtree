#include <iostream>
#include <vector>
#include "quadtree.h"
#include "free_quadtree.h"
#include <atomic>
#include <memory>
#include <functional> //debug
#include <thread> //debug
#include <algorithm>

namespace
{
using std::vector;
using std::cout;
using std::endl;
using std::atomic;

/// Each thread has lists of pointers to delete.
/// These are compared to the non-thread-local hazard pointer list.
/// If no hazard pointer contains the pointer, it is safe for deletion.
thread_local std::vector<quadtree::PointList*> deleteList;
thread_local std::vector<quadtree::PointList*> deleteWithNodeList;
}

namespace quadtree
{
std::atomic<LockfreeQuadtree::HazardPointer*> LockfreeQuadtree::HazardPointer::head;

LockfreeQuadtree::LockfreeQuadtree(BoundingBox boundary_, size_t capacity_)
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
    return false;
  // MIN means the subdivision has reached the minimum double precision. We can't subdivide anymore.
  HazardPointer* hazardPointer = HazardPointer::Acquire(); // @todo create a finaliser/whileinscope class for HazardPointers.
  while(true)
  {
    hazardPointer->Hazard.store(points.load());
    PointList* oldPoints = hazardPointer->Hazard.load();
    if(oldPoints == nullptr || oldPoints->Length >= oldPoints->Capacity)
      break;
    PointList* newPoints = new PointList(oldPoints->Capacity);
    newPoints->First = new PointListNode(p, oldPoints->First);
    newPoints->Length = oldPoints->Length + 1;
    const bool ok = points.compare_exchange_strong(oldPoints, newPoints);
//    while(points.load() == oldPoints); // necesary?
    hazardPointer->Hazard.store(nullptr);

    if(ok)
    {
      deleteList.push_back(oldPoints);
      gc();
      HazardPointer::Release(hazardPointer); /// @todo create a finaliser. This is dangerous. I don't like it. Not one bit.
      return true;
    }
    else
    {
      delete newPoints->First;
      delete newPoints;
    }
  }
  HazardPointer::Release(hazardPointer);

  PointList* localPoints = points.load(); // we don't need to set the Hazard Pointer because we never dereference the pointer
  if(localPoints != nullptr)
    subdivide();

//  while(Nw.load() == nullptr);
//  while(Ne.load() == nullptr);
//  while(Sw.load() == nullptr);
//  while(Se.load() == nullptr);

  // these will each need Hazard Pointers if it's ever possible for a subtree to be deleted
  LockfreeQuadtree* nw = Nw.load();
  LockfreeQuadtree* ne = Ne.load();
  LockfreeQuadtree* sw = Sw.load();
  LockfreeQuadtree* se = Se.load();
  const bool ok = nw->Insert(p) || ne->Insert(p) || sw->Insert(p) || se->Insert(p);

  if(!ok) // debug
  {
    cout << "insert failed" << endl;
    cout << p.String() << " inside " << boundary.String() << " but outside " << endl;
    cout << nw->Boundary().String() << " and " << endl;
    cout << ne->Boundary().String() << " and " << endl;
    cout << sw->Boundary().String() << " and " << endl;
    cout << se->Boundary().String() << endl << endl;
    
    cout << this << " " << nw << " " << ne << " " << sw << " " << se << endl;
  }

  return ok;
}

void LockfreeQuadtree::subdivide()
{
  HazardPointer* hazardPointer = HazardPointer::Acquire(); // @todo pass this rather than expensively reacquiring
  hazardPointer->Hazard.store(points.load());
  PointList* oldPoints = hazardPointer->Hazard.load();
  if(oldPoints == nullptr)
    return;

  size_t capacity = oldPoints->Capacity;

  HazardPointer::Release(hazardPointer);


  if(capacity == 0) // if cap=0 someone beat us here. Skip to dispersing.
  {
    disperse(); //debug
    return;
  }

  const double dx = 0.000001;
  // don't subdivide further if we reach the limits of double precision
  if(fabs(boundary.HalfDimension.X/2.0) < dx || fabs(boundary.HalfDimension.Y/2.0) < dx)
    capacity = std::numeric_limits<size_t>::max();

  const Point newHalf = {boundary.HalfDimension.X / 2.0, boundary.HalfDimension.Y / 2.0}; 
  LockfreeQuadtree* lval = nullptr;

  Point newCenter = {boundary.Center.X - boundary.HalfDimension.X/2.0, boundary.Center.Y - boundary.HalfDimension.Y/2.0};
  BoundingBox newBoundary = {newCenter, newHalf};
  LockfreeQuadtree* q = new LockfreeQuadtree(newBoundary, capacity);
  const bool nwOk = Nw.compare_exchange_strong(lval, q);
  if(!nwOk)
  {
    delete q;
    while(Nw.load() == nullptr);
  }

  newCenter = {boundary.Center.X + boundary.HalfDimension.X/2.0, boundary.Center.Y - boundary.HalfDimension.Y/2.0};
  newBoundary = {newCenter, newHalf};
  q = new LockfreeQuadtree(newBoundary, capacity);
  const bool neOk = Ne.compare_exchange_strong(lval, q);
  if(!neOk)
  {
    delete q;
    while(Ne.load() == nullptr);
  }

  newCenter = {boundary.Center.X + boundary.HalfDimension.X/2.0, boundary.Center.Y + boundary.HalfDimension.Y/2.0};
  newBoundary = {newCenter, newHalf};
  q = new LockfreeQuadtree(newBoundary, capacity);
  const bool seOk = Se.compare_exchange_strong(lval, q);
  if(!seOk)
  {
    delete q;
    while(Se.load() == nullptr);
  }

  newCenter = {boundary.Center.X - boundary.HalfDimension.X/2.0, boundary.Center.Y + boundary.HalfDimension.Y/2.0};
  newBoundary = {newCenter, newHalf};
  q = new LockfreeQuadtree(newBoundary, capacity);
  const bool swOk = Sw.compare_exchange_strong(lval, q);
  if(!swOk)
  {
    delete q;
    while(Sw.load() == nullptr);
  }

//  while(Nw.load() == nullptr);
//  while(Ne.load() == nullptr);
//  while(Sw.load() == nullptr);
//  while(Se.load() == nullptr);

  disperse();
}


void LockfreeQuadtree::disperse()
{
//  const auto tid = std::to_string(std::hash<std::thread::id>()(std::this_thread::get_id()));
  PointList* oldPoints;
  HazardPointer* hazardPointer = HazardPointer::Acquire(); // @todo pass this rather than expensively reacquiring
  while(true)
  {
    hazardPointer->Hazard.store(points.load());
    oldPoints = hazardPointer->Hazard.load();

    if(oldPoints == nullptr || oldPoints->Length == 0)
      break;
    
    PointList* newPoints = new PointList(0); // set the capacity to 0, so no one else tries to add

    //debug
    if(oldPoints->Length == 0)
      cout << "op0\n";

    // debug
    if(oldPoints->First == nullptr)
      cout << "opf0 " << oldPoints->Length << endl;

    Point p = oldPoints->First->NodePoint;
    newPoints->First = oldPoints->First->Next;
    newPoints->Length = oldPoints->Length - 1;

    /// @todo we must atomically swap the new points, and insert the point into the child.
    ///       we can do this by making Query() help in the dispersal
    bool ok = points.compare_exchange_strong(oldPoints, newPoints);
//    while(points.load() == oldPoints); //necessary?
    hazardPointer->Hazard.store(nullptr);

    if(!ok)
    {
      delete newPoints;
//      hazardPointer->Hazard.store(nullptr);
      continue;
    }

    deleteWithNodeList.push_back(oldPoints);
    gc();


    // shouldn't be necessary.
//    while(Nw.load() == nullptr);
//    while(Ne.load() == nullptr);
//    while(Se.load() == nullptr);
//    while(Sw.load() == nullptr);

    LockfreeQuadtree* nw = Nw.load(); // @todo merge this with while, to avoid calling load twice
    LockfreeQuadtree* ne = Ne.load();
    LockfreeQuadtree* se = Se.load();
    LockfreeQuadtree* sw = Sw.load();

    ok = nw->Insert(p) || ne->Insert(p) || sw->Insert(p) || se->Insert(p);
  }
  HazardPointer::Release(hazardPointer); /// @todo create a finaliser. This is dangerous. I don't like it. Not one bit.

  if(oldPoints != nullptr)
    points.store(nullptr);
}

vector<Point> LockfreeQuadtree::Query(const BoundingBox& b)
{
  vector<Point> found;

  if(!boundary.Intersects(b))
    return found;

  HazardPointer* hazardPointer = HazardPointer::Acquire();
  hazardPointer->Hazard.store(points.load());
  PointList* localPoints = hazardPointer->Hazard.load();
  if(localPoints != nullptr)
  {
    for(auto node = localPoints->First; node != nullptr; node = node->Next)
    {
      if(b.Contains(node->NodePoint))
	found.push_back(node->NodePoint);
    }
  }
  HazardPointer::Release(hazardPointer);

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

/// tries to delete everything in this thread's delete lists.
/// This is part of the hazard pointer implementation
void LockfreeQuadtree::gc()
{
  if(deleteList.empty() && deleteWithNodeList.empty())
    return;

  vector<PointList*> hazards;
  for(HazardPointer* head = HazardPointer::Head(); head != nullptr; head = head->Next) 
  {
    PointList* p = head->Hazard.load();
    if(p != nullptr)
      hazards.push_back(p);
  }
  std::sort(hazards.begin(), hazards.end(), std::less<PointList*>());
  for(auto i = deleteList.begin(); i != deleteList.end();)
  {
    if(!std::binary_search(hazards.begin(), hazards.end(), *i))
    {
      delete (*i);
//      cout << "deleted\n";
//      if(&*i != &deleteList.back())
//	*i = deleteList.back();
//      deleteList.pop_back();
      i = deleteList.erase(i);
    }
    else
      ++i;
  }
  for(auto i = deleteWithNodeList.begin(); i != deleteWithNodeList.end();)
  {
    if(!std::binary_search(hazards.begin(), hazards.end(), *i))
    {
      delete (*i)->First;
      delete (*i);
//      cout << "deletedW\n";
      i = deleteWithNodeList.erase(i);
//      if(&*i != &deleteList.back())
//	*i = deleteList.back();
//      deleteList.pop_back();
    }
    else
      ++i;
  }
//  return deleteList.empty() && deleteWithNodeList.empty();
}

/// @return whether there is nothing more for this thread to delete, i.e. whether this thread's delete lists are empty.
bool LockfreeQuadtree::ThreadCanComplete()
{
  return deleteWithNodeList.empty() && deleteList.empty();
}

LockfreeQuadtree::HazardPointer* LockfreeQuadtree::HazardPointer::Acquire()
{
  // try to reuse a released HazardPointer
  for(HazardPointer* p = LockfreeQuadtree::HazardPointer::head.load(); p != nullptr; p = p->Next)
  {
    if(p->active.test_and_set())
      continue;
    return p;
  }

  // no old released HazardPointers. Allocate a new one
//  ++length; // std::atomic overloads the ++ operator

  HazardPointer* p = new HazardPointer();
  p->active.test_and_set();
  p->Hazard.store(nullptr);

      
  /// @todo change this to a for loop. Because I like for.
  HazardPointer* old;
  do {
    old = LockfreeQuadtree::HazardPointer::head.load();
    p->Next = old;
  } while(!LockfreeQuadtree::HazardPointer::head.compare_exchange_strong(old, p));
  return p;
}

}

