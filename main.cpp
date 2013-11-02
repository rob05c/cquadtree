#include <iostream>
#include <vector>
#include "quadtree.h"
#include "free_quadtree.h"
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>

namespace
{
using std::vector;
using std::cout;
using std::endl;
using std::atomic;
using std::thread;
using std::shared_ptr;
using std::srand;
using std::time;
using std::max;
using std::strtoul;
using std::chrono::time_point;
using std::chrono::system_clock;
using std::chrono::duration;
using quadtree::BoundingBox;
using quadtree::Point;
using quadtree::Quadtree;
using quadtree::LockfreeQuadtree;

const unsigned int NODE_CAPACITY = 4;
const unsigned int DEFAULT_THREADS = max(thread::hardware_concurrency(), 1u);
const unsigned int DEFAULT_POINTS = 10000000;

inline double frand()
{
  return (double)rand() / (double)RAND_MAX;
}
}


/// @param q the quadtree to insert into
/// @param points the number of points to insert. Actual points inserted is floor(points/threads)/threads
/// @param threads the number of threads to use to insert in parallel
/// @return the number of points inserted. This will equal floor(points/threads)*threads, not points
int testInsert(Quadtree* q, int points, int numThreads)
{
  const auto tpoints = points / numThreads;
  const auto insertPoint = [tpoints, q] () {
    for(int i = 0, end = tpoints; i != end; ++i)
    {
      const auto p = Point(frand()*100.0 + 50.0, frand() * 100.0 + 50.0);
      const bool ok = q->Insert(p);
      if(!ok)
	cout << "testInsert insert failed" << endl;

      // debug
/*
      if(i % 100000 == 0)
      {
	const std::string msg = std::string() + "inserted " + std::to_string(i) + "\n";
	cout << msg;
      }
*/
    }
  };

  cout << "inserting " << tpoints << " per thread\n";  

  vector<shared_ptr<thread>> threads;
  for(int i = 0, end = numThreads; i != end; ++i)
  {
//    cout << "starting thread\n";
    threads.push_back(shared_ptr<thread>(new thread(insertPoint)));
  }

  for(auto i : threads)
  {
    i->join();
//    cout << "joined thread\n";
  }

  return tpoints * numThreads;
}

void printTree(Quadtree* q)
{
  const time_point<system_clock> start = system_clock::now();

  vector<Point> ps = q->Query(q->Boundary());

  const time_point<system_clock> end = system_clock::now();
  const duration<double> elapsed = end - start;
  const int seconds_elapsed = elapsed.count();
  cout << "queried " << ps.size() << " in " << seconds_elapsed << " seconds." << endl;

  if(ps.size() < 1000)
  {
    cout << "found ";
    for(auto i = ps.begin(), end = ps.end(); i != end; ++i)
      cout << "(" << i->X << " " << i->Y << "), ";
    cout << endl;
  }
}

int main(int argc, char** argv)
{
  // @todo cout whether unsigned long is atomic!!

//  cout << sizeof(int*);

  auto points = DEFAULT_POINTS;
  auto threads = DEFAULT_THREADS;

  if(argc > 1)
  {
    const auto p = static_cast<unsigned int>(strtoul(argv[1], 0, 10));
    if(p > 0)
      points = p;
  }
  if(argc > 2)
  {
    const auto t = static_cast<unsigned int>(strtoul(argv[2], 0, 10));
    if(t > 0)
      threads = t;
  }

  srand(time(nullptr));
  cout << std::fixed;

  cout << "threads: " << threads << endl;
  cout << "points: " << points << endl;
  cout << "capacity: " << NODE_CAPACITY << endl;

  //#if !__has_feature(cxx_atomic)
  //  cout << "no atomic :(" << endl;
  //#endif

  const BoundingBox b = {{100, 100}, {50, 50}};
  LockfreeQuadtree q(b, NODE_CAPACITY);

/*
  auto qpoints = q.points.load();
  if(qpoints->First == nullptr)
    cout << "first null\n";
  else if(qpoints->First->Next == nullptr)
    cout << "first->next null\n";

  const auto p = Point(frand()*100.0 + 50.0, frand() * 100.0 + 50.0);
  const bool ok = q.Insert(p);

  qpoints = q.points.load();
  if(qpoints->First == nullptr)
    cout << "2 first null\n";
  else if(qpoints->First->Next == nullptr)
    cout << "2 first->next null\n";
*/


  const time_point<system_clock> start = system_clock::now();

  const int inserted = testInsert(&q, points, threads);

  const time_point<system_clock> end = system_clock::now();
  const duration<double> elapsed = end - start;
  const int seconds_elapsed = elapsed.count();

  cout << "inserted " << inserted << " in " << seconds_elapsed << " seconds with " << threads << " threads." << endl;

  printTree(&q);


/*  
  cout << "mass insert...\n";
  const auto p = Point(frand()*100.0 + 50.0, frand() * 100.0 + 50.0);
  for(int i = 0, end = NODE_CAPACITY + 1; i != end; ++i)
    q.Insert(p);
//  printTree(&q);
*/
  return 0;
}
