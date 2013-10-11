#ifndef quadtreeH
#define quadtreeH

#include <vector>

namespace quadtree 
{
struct Point
{
  double X;
  double Y;
};

struct PointListNode
{
  Point NodePoint;
  PointListNode* Next;
};

class PointList
{
public:
  PointList(int capacity) : Capacity(capacity), First(NULL), Length(0) {}
  int Capacity;
  PointListNode* First;
  int Length; // cache for speed; we could calculate it in O(n) by iterating thru the nodes
};

class BoundingBox
{
public:
  Point Center;
  Point HalfDimension;
  bool Contains(const Point& p)
  {
    return p.X >= Center.X - HalfDimension.X
      && p.X <= Center.X + HalfDimension.X
      && p.Y >= Center.Y - HalfDimension.Y
      && p.Y <= Center.Y + HalfDimension.Y;
  }
  bool Intersects(const BoundingBox& other)
  {
    return Center.X + HalfDimension.X > other.Center.X - other.HalfDimension.X
      && Center.X - HalfDimension.X < other.Center.X + other.HalfDimension.X
      && Center.Y + HalfDimension.Y > other.Center.Y - other.HalfDimension.Y
      && Center.Y - HalfDimension.Y < other.Center.Y + other.HalfDimension.Y;
  }
};

/// interface
class Quadtree
{
public:
  virtual bool Insert(const Point& p) = 0;
  virtual std::vector<Point> Query(const BoundingBox&) = 0;
  virtual BoundingBox Boundary() = 0;
};

}

#endif // quadtreeH
