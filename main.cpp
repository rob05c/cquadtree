#include <iostream>
#include <vector>
#include "quadtree.h"
#include "free_quadtree.h"
#include "lock_quadtree.h"
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
using std::chrono::duration_cast;
using std::chrono::time_point;
using std::chrono::high_resolution_clock;
using std::chrono::duration;
using quadtree::BoundingBox;
using quadtree::Point;
using quadtree::Quadtree;
using quadtree::LockfreeQuadtree;
using quadtree::LockQuadtree;

const unsigned int DEFAULT_CAPACITY = 4;
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
    }
  };


  cout << "inserting " << tpoints << " per thread\n";  

  vector<shared_ptr<thread>> threads;
  for(int i = 0, end = numThreads; i != end; ++i)
    threads.push_back(shared_ptr<thread>(new thread(insertPoint)));

  for(auto i : threads)
    i->join();

  return tpoints * numThreads;
}


/// @param q the quadtree to insert into
/// @param points the number of points to insert. Actual points inserted is floor(points/threads)/threads
/// @param threads the number of threads to use to insert in parallel
/// @return the number of points inserted. This will equal floor(points/threads)*threads, not points
int testInsertQuery(Quadtree* q, int points, int numThreads)
{
  shared_ptr<std::atomic<bool>> doneInserting = shared_ptr<atomic<bool>>(new std::atomic<bool>());
  shared_ptr<std::atomic<size_t>> numQueries = shared_ptr<atomic<size_t>>(new std::atomic<size_t>());
  doneInserting->store(false);
  numQueries->store(0u);

  const auto tpoints = points / numThreads;
  const auto insertPoint = [tpoints, q] () {
    for(int i = 0, end = tpoints; i != end; ++i)
    {
      const auto p = Point(frand()*100.0 + 50.0, frand() * 100.0 + 50.0);
      const bool ok = q->Insert(p);
      if(!ok)
	cout << "testInsert insert failed" << endl;
    }
    q->Query(q->Boundary());
  };

  // not really forever
  const auto foreverQuery = [q, doneInserting, numQueries] () {
    while(doneInserting->load() == false) 
    {
      q->Query(q->Boundary());
      ++(*numQueries.get());
    }
  };

  cout << "query-inserting " << tpoints << " per thread\n";  

  vector<shared_ptr<thread>> queryThreads;
  for(int i = 0, end = 1; i != end; ++i)
    queryThreads.push_back(shared_ptr<thread>(new thread(foreverQuery)));

  vector<shared_ptr<thread>> threads;
  for(int i = 0, end = numThreads; i != end; ++i)
    threads.push_back(shared_ptr<thread>(new thread(insertPoint)));

  for(auto i : threads)
    i->join();

  doneInserting->store(true);
  for(auto i : queryThreads)
    i->join();

  cout << "queries: " << std::to_string(numQueries.get()->load()) << endl;
  return tpoints * numThreads;
}

void printTree(Quadtree* q)
{
  const BoundingBox b = {{100.0, 100.0}, {25.0, 25.0}};

  const time_point<high_resolution_clock> start = high_resolution_clock::now();
//  vector<Point> ps = q->Query(q->Boundary());
  vector<Point> ps = q->Query(b);

  const time_point<high_resolution_clock> end = high_resolution_clock::now();
  const duration<double> elapsed = duration_cast<duration<double>>(end - start);

  cout << "queried " << ps.size() << " in " << elapsed.count() << " seconds." << endl;

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
  auto capacity = DEFAULT_CAPACITY;

  if(argc > 1)
  {
    const auto p = static_cast<unsigned int>(strtoul(argv[1], 0, 10));
    if(p == 0)
    {
      cout << "Usage: quadtree points threads lockfree capacity\n";
      return 0;
    }
    if(p > 0)
      points = p;
  }
  if(argc > 2)
  {
    const auto t = static_cast<unsigned int>(strtoul(argv[2], 0, 10));
    if(t > 0)
      threads = t;
  }

  bool lockfree = true;
  if(argc > 3)
  {
    const auto t = static_cast<unsigned int>(strtoul(argv[3], 0, 10));
    lockfree = t > 0;
  }

  if(argc > 4)
  {
    const auto c = static_cast<unsigned int>(strtoul(argv[4], 0, 10));
    if(c > 0)
      capacity = c;
  }

  cout << (lockfree ? "Lock Free\n" : "Lock Based\n");

  srand(time(nullptr));
  cout << std::fixed;

  cout << "threads: " << threads << endl;
  cout << "points: " << points << endl;
  cout << "capacity: " << capacity << endl;

  //#if !__has_feature(cxx_atomic)
  //  cout << "no atomic :(" << endl;
  //#endif

  const BoundingBox b = {{100, 100}, {50, 50}};
  auto q = std::unique_ptr<Quadtree>(lockfree ? (Quadtree*)new LockfreeQuadtree(b, capacity) : (Quadtree*)new LockQuadtree(b, capacity));

  const time_point<high_resolution_clock> start = high_resolution_clock::now();

  const int inserted = testInsert(q.get(), points, threads);

  const time_point<high_resolution_clock> end = high_resolution_clock::now();
  const duration<double> elapsed = duration_cast<duration<double>>(end - start);

  cout << "inserted " << inserted << " in " << elapsed.count() << " seconds with " << threads << " threads." << endl;

  printTree(q.get());

  return 0;
}
