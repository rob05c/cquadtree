#ifndef quadtreeH
#define quadtreeH

#include <vector>
#include <string>

namespace quadtree 
{
struct Point
{
  Point(const double& x, const double& y)
  : X(x)
  , Y(y)
  {}
  std::string String() const
  {
    return std::string() + "[" + std::to_string(X) + "," + std::to_string(Y) + "]";
  }
  double X;
  double Y;
};

class PointListNode
{
public:
  PointListNode(const Point& p, PointListNode* next) : NodePoint(p), Next(next) {}
  Point NodePoint;
  PointListNode* Next;
};

/// @todo change this to use std iterators
class PointList
{
public:
  PointList(int capacity) : Capacity(capacity), First(nullptr), Length(0) {}
  int Capacity;
  PointListNode* First;
  int Length; // cache for speed; we could calculate it in O(n) by iterating thru the nodes
};

class BoundingBox
{
public:
  Point Center;
  Point HalfDimension;
  bool Contains(const Point& p) const
  {
    return p.X >= Center.X - HalfDimension.X
      && p.X <= Center.X + HalfDimension.X
      && p.Y >= Center.Y - HalfDimension.Y
      && p.Y <= Center.Y + HalfDimension.Y;
  }
  bool Intersects(const BoundingBox& other) const
  {
    return Center.X + HalfDimension.X > other.Center.X - other.HalfDimension.X
      && Center.X - HalfDimension.X < other.Center.X + other.HalfDimension.X
      && Center.Y + HalfDimension.Y > other.Center.Y - other.HalfDimension.Y
      && Center.Y - HalfDimension.Y < other.Center.Y + other.HalfDimension.Y;
  }
  std::string String()
  {
    return std::string() + "[" + Center.String() + "," + HalfDimension.String() + "]";
  }
};

/// interface
class Quadtree
{
public:
  virtual ~Quadtree() {}
  virtual bool Insert(const Point& p) = 0;
  virtual std::vector<Point> Query(const BoundingBox&) = 0;
  virtual BoundingBox Boundary() = 0;
};

}

#endif // quadtreeH
