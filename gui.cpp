#include <iostream>
#include <vector>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <string>
#include <unordered_map>
#include <ncurses.h>
#include "quadtree.h"
#include "free_quadtree.h"
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
using std::string;
using std::to_string;
using std::unordered_map;
using std::chrono::time_point;
using std::chrono::system_clock;
using std::chrono::duration;
using quadtree::BoundingBox;
using quadtree::Point;
using quadtree::Quadtree;
using quadtree::LockfreeQuadtree;

const unsigned int NODE_CAPACITY = 100;
const unsigned int DEFAULT_THREADS = max(thread::hardware_concurrency(), 1u);
const unsigned int DEFAULT_POINTS = 50000;

inline double frand()
{
  return (double)rand() / (double)RAND_MAX;
}

const char* NUM[] = {
  u8" ",
  u8"\u00b7",
  u8"\u205a",
  u8"\u2056",
  u8"\u2058",
  u8"\u2059",
  u8"\u28f6",
  u8"\u28f7",
  u8"\u28ff",
  u8"\u2468",
  u8"\u2469",
  u8"\u246a",
  u8"\u246b",
  u8"\u246c",
  u8"\u246d",
  u8"\u246e",
  u8"\u246f",
  u8"\u2470",
  u8"\u2471",
  u8"\u2472",
  u8"\u2473",
  u8"\u3251",
  u8"\u3252",
  u8"\u3253",
  u8"\u3254",
  u8"\u3255",
  u8"\u3256",
  u8"\u3257",
  u8"\u3258",
  u8"\u3259",
  u8"\u325a",
  u8"\u325b",
  u8"\u325c",
  u8"\u325d",
  u8"\u325e",
  u8"\u325f",
  u8"\u32b1",
  u8"\u32b2",
  u8"\u32b3",
  u8"\u32b4",
  u8"\u32b5",
  u8"\u32b6",
  u8"\u32b7",
  u8"\u32b8",
  u8"\u32b9",
  u8"\u32ba",
  u8"\u32bb",
  u8"\u32bc",
  u8"\u32bd",
  u8"\u32be",
  u8"\u32bf"
};
const size_t NUM_LEN = sizeof(NUM) / sizeof(NUM[0]);

const char* MANY = u8"\u2603";

inline const char* numstr(const size_t& n)
{
//  return u8"\u00b7";
  return n > NUM_LEN ? MANY : NUM[n];
}

int window_height = 0;
int window_width = 0;
}


/// @param q the quadtree to insert into
/// @param points the number of points to insert. Actual points inserted is floor(points/threads)/threads
/// @param threads the number of threads to use to insert in parallel
/// @return the number of points inserted. This will equal floor(points/threads)*threads, not points
int testInsert(Quadtree* q, int points, int numThreads)
{
  const BoundingBox b = q->Boundary();
  const auto width = b.Center.X + b.HalfDimension.X;
  const auto height = b.Center.Y + b.HalfDimension.Y;
  const auto tpoints = points / numThreads;
  const auto insertPoint = [tpoints, q, width, height] () {
    for(int i = 0, end = tpoints; i != end; ++i)
    {
      q->Insert(Point(frand()*(width-1) + 1, frand()*(height-1) + 1));
    }
  };

  vector<shared_ptr<thread>> threads;
  for(int i = 0, end = numThreads; i != end; ++i)
    threads.push_back(shared_ptr<thread>(new thread(insertPoint)));
  for(auto i : threads)
    i->join();

  return tpoints * numThreads;
}

/// prints basic GUI that should always be visible
void printGui(const string& msg)
{
  box(stdscr, 0, 0);
  mvprintw(window_height, window_width / 2 - msg.size() / 2, msg.c_str());
  mvprintw(window_height, 1,"%s", "Quadtree demo ");
  const string msgExit = " Press any key to exit";
  mvprintw(window_height, window_width - msgExit.size() - 1, msgExit.c_str());
}

/// Does NOT refresh the window.
/// @return tree info message
string drawTree(Quadtree* q)
{

  vector<Point> points = q->Query(q->Boundary());

  unordered_map<long, unordered_map<long, size_t>> pointMap;
  for(auto i = points.begin(), end = points.end(); i != end; ++i)
    ++pointMap[(int)i->X][(int)i->Y];

  for(auto i = pointMap.begin(), end = pointMap.end(); i != end; ++i)
    for(auto j = i->second.begin(), jend = i->second.end(); j != jend; ++j)
      mvprintw(j->first, i->first, numstr(j->second));

  return string() + "points: " + to_string(points.size());
}

int main(int argc, char** argv)
{
  setlocale(LC_ALL, "");
  initscr();
  noecho();

  getmaxyx(stdscr, window_height, window_width);
  --window_height;

  printGui("loading...");
  refresh();

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

  Point center = {(double)(window_width / 2), (double)(window_height / 2)};
  const BoundingBox b = {center, center}; // half-dimension is same as center, for our screen-size box
  LockfreeQuadtree q(b, NODE_CAPACITY);

  testInsert(&q, points, threads);
  const string treeMsg = drawTree(&q);
  printGui(treeMsg);
  curs_set(0); // hide cursor
  refresh();
  getch();
  endwin();
  return 0;
}
