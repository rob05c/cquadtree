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

using quadtree::BoundingBox;
using quadtree::Point;
using quadtree::LockfreeQuadtree;
}

int main() {
  //#if !__has_feature(cxx_atomic)
  //  cout << "no atomic :(" << endl;
  //#endif

  BoundingBox b = {{100, 100}, {50, 50}};
  LockfreeQuadtree q(b, 4);
  Point p = {125, 125};
  q.Insert(p);
  vector<Point> ps = q.Query(b);
  cout << "queried " << ps.size() << endl;
  cout << "found ";
  for(auto i = ps.begin(), end = ps.end(); i != end; ++i)
    cout << "(" << i->X << " " << i->Y << "), ";
  cout << endl;

  return 0;
}
