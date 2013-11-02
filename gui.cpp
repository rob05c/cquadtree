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

const unsigned int NODE_CAPACITY = 4;
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

bool draw_boundary = false;
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
//  box(stdscr, 0, 0);
  mvprintw(window_height, window_width / 2 - msg.size() / 2, msg.c_str());
  mvprintw(window_height, 1,"%s", "Quadtree demo ");
  const string msgExit = " Press 'q' to exit";
  mvprintw(window_height, window_width - msgExit.size() - 1, msgExit.c_str());
}

void drawBoundary(const BoundingBox& b)
{
  int sx;
  int sy;
  getyx(stdscr, sy, sx);

  const int length = (int)(b.HalfDimension.Y * 2);
  const int width = (int)(b.HalfDimension.X * 2);

  int x = (int)(b.Center.X - b.HalfDimension.X);
  int y = (int)(b.Center.Y - b.HalfDimension.Y);
  move(y, x);
  vline('|', length);
  hline('-', width);
  move(y, x);
  hline('-', width);
  vline('|', length);
  move(sy, sx);
}

/// recursively draws subtree borders.
void drawTreeBorders(Quadtree* q)
{
  if(q == nullptr)
    return;
  int x;
  int y;
  getyx(stdscr, y, x);
  drawBoundary(q->Boundary());

  LockfreeQuadtree* lq = (LockfreeQuadtree*)q; ///< @todo fix this by adding subtree funcs to Quadtree
  drawTreeBorders(lq->nw());
  drawTreeBorders(lq->ne());
  drawTreeBorders(lq->sw());
  drawTreeBorders(lq->se());
  move(y, x);
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

  if(draw_boundary)
    drawTreeBorders(q);

  return string() + "points: " + to_string(points.size());
}


/// broken. Getting the current char doesn't work.
void drawPoint(Point* p)
{

  const string currentChar = string(1, mvinch(p->Y, p->X));
  string newChar = NUM[0];
  for(size_t i = 0, end = NUM_LEN; i != end; ++i)
  {
    if(currentChar == string(NUM[i]))
    {
      newChar =  i + 1 > NUM_LEN ? MANY : NUM[i + 1];
      break;
    }
  }
  mvprintw((int)p->Y, (int)p->X, newChar.c_str());
//  printGui((string() + "X" + currentChar + "X").c_str());
}

void insertPoint(int y, int x, Quadtree* q)
{
  Point p = {(double)x, (double)y};
  q->Insert(p);
  int cx;
  int cy;
  getyx(stdscr, cy, cx);
  drawTree(q);
  move(cy, cx);
}

void handleMouse(Quadtree* q)
{
  MEVENT e;
  getmouse(&e);
  insertPoint(e.y, e.x, q);
  refresh();
}

void handleKey(int c, Quadtree* q)
{
  int x;
  int y;
  getyx(stdscr, y, x);
  if(c == KEY_UP)
    move(y-1, x);
  else if(c == KEY_DOWN)
    move(y+1, x);
  else if(c == KEY_RIGHT)
    move(y, x+1);
  else if(c == KEY_LEFT)
    move(y, x-1);
  else if(c == ' ')
    insertPoint(y, x, q);
  refresh();
}

int main(int argc, char** argv)
{
  setlocale(LC_ALL, "");
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, true);
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

  if(argc > 3)
  {
    const auto b = static_cast<unsigned int>(strtoul(argv[2], 0, 10));
    draw_boundary = (b != 0);
  }

  srand(time(nullptr));

  Point center = {(double)(window_width / 2), (double)(window_height / 2)};
  const BoundingBox b = {center, center}; // half-dimension is same as center, for our screen-size box
  LockfreeQuadtree q(b, NODE_CAPACITY);

  testInsert(&q, points, threads);
  const string treeMsg = drawTree(&q);
  printGui(treeMsg);
//  curs_set(0); // hide cursor
  move(0,0);
  refresh();

  mousemask(BUTTON1_PRESSED, NULL);
  while(true)
  {
    const int c = getch();
    if(c == KEY_MOUSE)
      handleMouse(&q);
    else if(c == 'q')
      break;
    else
      handleKey(c, &q);
  }

  endwin();
  return 0;
}
