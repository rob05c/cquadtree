#include <iostream>
#include <vector>
#include "quadtree.h"
#include "lock_quadtree.h"
#include <atomic>
#include <memory>
#include <functional> //debug
#include <thread> //debug
#include <algorithm>
#include <mutex>
namespace
{
using std::vector;
using std::cout;
using std::endl;
}

namespace quadtree
{
LockQuadtree::LockQuadtree(BoundingBox boundary_, size_t capacity_)
  : boundary(boundary_)
  , capacity(capacity_)
  , Nw(nullptr)
  , Ne(nullptr)
  , Sw(nullptr)
  , Se(nullptr)
{}

bool LockQuadtree::Insert(const Point& p)
{
  if(!boundary.Contains(p))
    return false;

  pointsMutex.lock();
  if(points.size() < capacity)
  {
    points.push_back(p);
    pointsMutex.unlock();
    return true;
  }

  if(!points.empty())
    subdivide();

  const bool ok = Nw->Insert(p) || Ne->Insert(p) || Sw->Insert(p) || Se->Insert(p);
  pointsMutex.unlock();
  return ok;
}

void LockQuadtree::subdivide()
{
  const double dx = 0.000001;
  // don't subdivide further if we reach the limits of double precision
  if(fabs(boundary.HalfDimension.X/2.0) < dx || fabs(boundary.HalfDimension.Y/2.0) < dx)
    capacity = std::numeric_limits<size_t>::max();

  const Point newHalf = {boundary.HalfDimension.X / 2.0, boundary.HalfDimension.Y / 2.0}; 
  Point newCenter = {boundary.Center.X - boundary.HalfDimension.X/2.0, boundary.Center.Y - boundary.HalfDimension.Y/2.0};
  BoundingBox newBoundary = {newCenter, newHalf};
  Nw = new LockQuadtree(newBoundary, capacity);;

  newCenter = {boundary.Center.X + boundary.HalfDimension.X/2.0, boundary.Center.Y - boundary.HalfDimension.Y/2.0};
  newBoundary = {newCenter, newHalf};
  Ne = new LockQuadtree(newBoundary, capacity);

  newCenter = {boundary.Center.X + boundary.HalfDimension.X/2.0, boundary.Center.Y + boundary.HalfDimension.Y/2.0};
  newBoundary = {newCenter, newHalf};
  Se = new LockQuadtree(newBoundary, capacity);

  newCenter = {boundary.Center.X - boundary.HalfDimension.X/2.0, boundary.Center.Y + boundary.HalfDimension.Y/2.0};
  newBoundary = {newCenter, newHalf};
  Sw = new LockQuadtree(newBoundary, capacity);

  disperse();
  capacity = 0;
}

void LockQuadtree::disperse()
{
  for(auto i = points.begin(), end = points.end(); i != end; ++i)
  {
    Point p = *i;
    const bool ok = Nw->Insert(p) || Ne->Insert(p) || Sw->Insert(p) || Se->Insert(p);
    if(!ok)
      cout << "disperse insert failed.";
  }
  points.clear();
}

vector<Point> LockQuadtree::Query(const BoundingBox& b)
{
  pointsMutex.lock();
  vector<Point> found;

  if(!boundary.Intersects(b))
    return found;

  for(auto i = points.begin(), end = points.end(); i != end; ++i)
  {
    if(b.Contains(*i))
      found.push_back(*i);
  }
  pointsMutex.unlock();

  if(Nw != nullptr)
  {
    vector<Point> f = Nw->Query(b);
    found.insert(found.end(), f.begin(), f.end());
  }
  if(Ne != nullptr)
  {
    vector<Point> f = Ne->Query(b);
    found.insert(found.end(), f.begin(), f.end());
  }
  if(Sw != nullptr)
  {
    vector<Point> f = Sw->Query(b);
    found.insert(found.end(), f.begin(), f.end());
  }
  if(Se != nullptr)
  {
    vector<Point> f = Se->Query(b);
    found.insert(found.end(), f.begin(), f.end());
  }
  return found;
}
}
