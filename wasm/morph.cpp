//
//    Source: https://github.com/OneSlashNinja/FeatureBasedImageMorphing
//    Copyright (C) OneSlashNinja
//    under: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//    modified and extenden by Julian Eder
//

#include <emscripten/bind.h>
using namespace emscripten;

using namespace std;

#include "geometricTool.h"
#include <cstdio>
#include <vector>
#include <limits>

// the pixel
typedef struct pix
{
  unsigned char r, g, b, a;
} pixel;


//--------------------------------------------------------------------------
//------------------------allocPixmap---------------------------------------
//--------------------------------------------------------------------------
// If I compile with -O3 the compiler removes this function when it does not have a print statement in the loop - I dont know why, Julian
pixel **allocPixmap(int w, int h)
{
  pixel **map;
  map = new pixel *[h];
  map[0] = new pixel[w * h];

  for (int i = 1; i < h; i++)
  {
    map[i] = map[i - 1] + w;
  }

  for (int row = 0; row < h; row++)
  {
    for (int col = 0; col < w; col++)
    {
      map[row][col].r = 0;
      map[row][col].g = 0;
      map[row][col].b = 0;
      map[row][col].a = 255;
    }
    // printf("row\n");
  }
  return map;
}

pixel **pixmapFromVector(int w, int h, vector<unsigned char> imageData)
{
  pixel **map = new pixel *[h];
  map[0] = new pixel[w * h];

  for (int i = 1; i < h; i++)
  {
    map[i] = map[i - 1] + w;
  }
  size_t idx = 0;
  for (int row = 0; row < h; row++)
  {
    for (int col = 0; col < w; col++)
    {
      map[row][col].r = imageData[idx++];
      map[row][col].g = imageData[idx++];
      map[row][col].b = imageData[idx++];
      map[row][col].a = imageData[idx++];
    }
  }
  return map;
}

vector<unsigned char> vectorFromPixmap(int w, int h, pixel **map)
{
  vector<unsigned char> result;
  for (int y = 0; y < h; y++)
  {
    for (int x = 0; x < w; x++)
    {
      result.push_back(map[y][x].r);
      result.push_back(map[y][x].g);
      result.push_back(map[y][x].b);
      result.push_back(map[y][x].a);
    }
  }
  return result;
}

//--------------------------------------------------------------------------------------------------
//--------------------------line interpolating function---------------------------------------------
//--------------------------------------------------------------------------------------------------
void lineInterpolate(const vector<FeatureLine> sourceLines,
                     const vector<FeatureLine> destLines, vector<FeatureLine> &interLines, float t)
{
  int i;
  for (i = 0; i < sourceLines.size(); i++)
  {
    FeatureLine f(
        (1 - t) * (sourceLines[i].startPoint) + t * (destLines[i].startPoint),
        (1 - t) * (sourceLines[i].endPoint) + t * (destLines[i].endPoint));
    interLines.push_back(f);
  }
}
//---------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------warp function-----------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/* warping function (backward mapping)
   input:
   uv_in = the point in the intermediary image
   dstLines = given line in the intermediary image
   srcLines = given line in the source image
   p, a, b = parameters of the weight function
   output:
   uv_out = the corresponding point */
void warp(const Vector2d &uv_in,
          const vector<FeatureLine> &srcLines,
          const vector<FeatureLine> &dstLines,
          float p, float a, float b, Vector2d &uv_out)
{
  int i, j;
  float interLength, srcLength;
  float weight, weightSum, dist;
  float sum_x, sum_y; // weighted sum of the coordination of the point "src"
  float u, v;
  Vector2d pd, pq, qd;
  float X, Y;

  sum_x = 0;
  sum_y = 0;
  weightSum = 0;

  if (srcLines.size() != dstLines.size())
    throw std::runtime_error("Different Number of Features for warp");

  for (i = 0; i < srcLines.size(); i++)
  {
    pd.x = uv_in.x - dstLines[i].startPoint.x;
    pd.y = uv_in.y - dstLines[i].startPoint.y;
    pq.x = dstLines[i].endPoint.x - dstLines[i].startPoint.x;
    pq.y = dstLines[i].endPoint.y - dstLines[i].startPoint.y;
    interLength = pq.x * pq.x + pq.y * pq.y;
    u = (pd.x * pq.x + pd.y * pq.y) / interLength;

    interLength = sqrt(interLength); // length of the vector PQ

    v = (pd.x * pq.y - pd.y * pq.x) / interLength;

    pq.x = srcLines[i].endPoint.x - srcLines[i].startPoint.x;
    pq.y = srcLines[i].endPoint.y - srcLines[i].startPoint.y;

    srcLength = sqrt(pq.x * pq.x + pq.y * pq.y); // length of the vector P'Q'
    // corresponding point based on the ith line
    X = srcLines[i].startPoint.x + u * pq.x + v * pq.y / srcLength;
    Y = srcLines[i].startPoint.y + u * pq.y - v * pq.x / srcLength;

    // the distance from the corresponding point to the line P'Q'
    if (u < 0)
      dist = sqrt(pd.x * pd.x + pd.y * pd.y);
    else if (u > 1)
    {
      qd.x = uv_in.x - dstLines[i].endPoint.x;
      qd.y = uv_in.y - dstLines[i].endPoint.y;
      dist = sqrt(qd.x * qd.x + qd.y * qd.y);
    }
    else
    {
      dist = abs(v);
    }

    weight = pow(pow(interLength, p) / (a + dist), b);
    sum_x += X * weight;
    sum_y += Y * weight;
    weightSum += weight;
  }

  uv_out.x = sum_x / weightSum;
  uv_out.y = sum_y / weightSum;
}

//--------------------------------------------------------------------------------------------------
//--------------------------bilinear interpolation--------------------------------------------------
//--------------------------------------------------------------------------------------------------
pixel bilinear(pixel **&Im, float row, float col)
{
  int cm, cn, fm, fn;
  double alpha, beta;

  cm = (int)ceil(row);
  fm = (int)floor(row);
  cn = (int)ceil(col);
  fn = (int)floor(col);
  alpha = ceil(row) - row;
  beta = ceil(col) - col;

  pixel pix;
  pix.r = (unsigned int)(alpha * beta * Im[fm][fn].r + (1 - alpha) * beta * Im[cm][fn].r + alpha * (1 - beta) * Im[fm][cn].r + (1 - alpha) * (1 - beta) * Im[cm][cn].r);
  pix.g = (unsigned int)(alpha * beta * Im[fm][fn].g + (1 - alpha) * beta * Im[cm][fn].g + alpha * (1 - beta) * Im[fm][cn].g + (1 - alpha) * (1 - beta) * Im[cm][cn].g);
  pix.b = (unsigned int)(alpha * beta * Im[fm][fn].b + (1 - alpha) * beta * Im[cm][fn].b + alpha * (1 - beta) * Im[fm][cn].b + (1 - alpha) * (1 - beta) * Im[cm][cn].b);
  pix.a = 255;
  return pix;
}
//---------------------------------------------------------------------------

bool approximatelyEqual(float a, float b, float epsilon)
{
  return fabs(a - b) <= epsilon;
}

vector<FeatureLine> sortOutlineLines(vector<FeatureLine> &outlineLines)
{
  vector<FeatureLine> sorted;
  int i = 0;
  int cnt = 0;
  do
  {
    bool found = false;
    for (int j = 0; j < outlineLines.size(); j++)
    {
      if (approximatelyEqual(outlineLines[i].endPoint.x, outlineLines[j].startPoint.x, 1e-3f) && approximatelyEqual(outlineLines[i].endPoint.y, outlineLines[j].startPoint.y, 1e-3f))
      {
        found = true;
        sorted.push_back(outlineLines[i]);
        i = j;
        break;
      }
    }
    if (!found)
      throw std::runtime_error("Not a valid Loop");
    cnt++;
  } while (cnt < outlineLines.size());
  return sorted;
}

bool isBlack(Vector2dInt c, pixel **srcImgMap, int w, int h)
{
  if (c.x < 0 || c.x >= w)
    return true;
  if (c.y < 0 || c.y >= h)
    return true;
  pixel pix = srcImgMap[c.y][c.x];
  return pix.r == 0 && pix.g == 0 && pix.b == 0 && pix.a == 255;
}

Vector2dInt SearchAlongLineRec(Vector2d s, Vector2d d, Vector2dInt prev_c, pixel **srcImgMap, int w, int h, int depth, bool inverse, bool verbose = false)
{
  Vector2d center = s + (d / 2.0);
  Vector2dInt c(center);
  if (c == prev_c){
    if(inverse){
      while (isBlack(c, srcImgMap, w, h)){ // step into the direction a little bit more to garante we are inside the border
        center = center + (-d / 2.0);
        c = center;
      }
    }else{
      while (isBlack(c, srcImgMap, w, h)){ // step into the direction a little bit more to garante we are inside the border
        center = center + (d / 2.0);
        c = center;
      }
    }
    return c;
  }

  if(verbose) printf("depth %d\n", depth);
  if(verbose) printf("s (%f %f)\n", s.x, s.y);
  if(verbose) printf("d (%f %f)\n", d.x, d.y);
  if(verbose) printf("d/2 (%f %f)\n", (d / 2.0).x, (d / 2.0).y);
  if(verbose) printf("center (%f %f)\n", center.x, center.y);

  if (isBlack(c, srcImgMap, w, h))
  {
    if(verbose) printf("B c (%d %d)\n", c.x, c.y);
    return SearchAlongLineRec(center, d / 2.0, c, srcImgMap, w, h, depth + 1, inverse, verbose);
  }
  else
  {
    if(verbose) printf("W c (%d %d)\n", c.x, c.y);
    return SearchAlongLineRec(s, d / 2.0, c, srcImgMap, w, h, depth + 1, inverse, verbose);
  }
  
}

Vector2d transformPoint(Vector2d v, vector<double> M)
{
  Vector2d r(
      M[0] * v.x + M[2] * v.y + M[4],
      M[1] * v.x + M[3] * v.y + M[5]);
  return r;
}

Vector2d transformDirection(Vector2d v, vector<double> M)
{
  Vector2d r(
      M[0] * v.x + M[2] * v.y,
      M[1] * v.x + M[3] * v.y);
  return r;
}

Point closetsPointOnSkelleton(Vector2d p, vector<FeatureLine> skelletonLines){
  
  Vector2d e;

  float dist_s = std::numeric_limits<float>::max();
  bool isStartClosest = false;
  int idx_s_closest = -1;
  
  for (int j = 0; j < skelletonLines.size(); j++)
  {
    // We work with squared distances to avoid sqrt
    // The s point has to be transformed, because outlineLines is in tiling space, while skelletonLines is in imageSpace

    e.x = skelletonLines[j].startPoint.x;
    e.y = skelletonLines[j].startPoint.y;
    float d_p_s = (p.x - e.x) * (p.x - e.x) + (p.y - e.y) * (p.y - e.y);

    e.x = skelletonLines[j].endPoint.x;
    e.y = skelletonLines[j].endPoint.y;
    float d_p_e = (p.x - e.x) * (p.x - e.x) + (p.y - e.y) * (p.y - e.y);


    if (d_p_s < dist_s)
    {
      isStartClosest = true;
      idx_s_closest = j;
      dist_s = d_p_s;
    }
    if (d_p_e < dist_s)
    {
      isStartClosest = false;
      idx_s_closest = j;
      dist_s = d_p_e;
    }
  }

  // Move Direction (Start Point)
  if (isStartClosest)
  {
    e.x = skelletonLines[idx_s_closest].startPoint.x;
    e.y = skelletonLines[idx_s_closest].startPoint.y;
  }
  else
  {
    e.x = skelletonLines[idx_s_closest].endPoint.x;
    e.y = skelletonLines[idx_s_closest].endPoint.y;
  }

  return e;
}

vector<FeatureLine> projectOutlineLines(vector<FeatureLine> &outlineLines, vector<FeatureLine> skelletonLines, pixel **srcImgMap, int w, int h)
{
  vector<FeatureLine> outlineLinesMorphed;
  for (int i = 0; i < outlineLines.size(); i++)
  {
    // find closest
    Vector2d d, s, e;

    float dist_s = std::numeric_limits<float>::max();
    float dist_e = std::numeric_limits<float>::max();
    bool s_isStartClosest = false;
    bool e_isStartClosest = false;
    int idx_s_closest = -1;
    int idx_e_closest = -1;
    for (int j = 0; j < skelletonLines.size(); j++)
    {
      // We work with squared distances to avoid sqrt
      // The s point has to be transformed, because outlineLines is in tiling space, while skelletonLines is in imageSpace

      s.x = outlineLines[i].startPoint.x;
      s.y = outlineLines[i].startPoint.y;
      e.x = skelletonLines[j].startPoint.x;
      e.y = skelletonLines[j].startPoint.y;
      float d_s_s = (s.x - e.x) * (s.x - e.x) + (s.y - e.y) * (s.y - e.y);

      e.x = skelletonLines[j].endPoint.x;
      e.y = skelletonLines[j].endPoint.y;
      float d_s_e = (s.x - e.x) * (s.x - e.x) + (s.y - e.y) * (s.y - e.y);

      s.x = outlineLines[i].endPoint.x;
      s.y = outlineLines[i].endPoint.y;
      e.x = skelletonLines[j].startPoint.x;
      e.y = skelletonLines[j].startPoint.y;

      float d_e_s = (s.x - e.x) * (s.x - e.x) + (s.y - e.y) * (s.y - e.y);

      e.x = skelletonLines[j].endPoint.x;
      e.y = skelletonLines[j].endPoint.y;
      float d_e_e = (s.x - e.x) * (s.x - e.x) + (s.y - e.y) * (s.y - e.y);

      if (d_s_s < dist_s)
      {
        s_isStartClosest = true;
        idx_s_closest = j;
        dist_s = d_s_s;
      }
      if (d_s_e < dist_s)
      {
        s_isStartClosest = false;
        idx_s_closest = j;
        dist_s = d_s_e;
      }

      if (d_e_s < dist_e)
      {
        e_isStartClosest = true;
        idx_e_closest = j;
        dist_e = d_e_s;
      }
      if (d_e_e < dist_e)
      {
        e_isStartClosest = false;
        idx_e_closest = j;
        dist_e = d_e_e;
      }
    }

    // Move Direction (Start Point)
    if (s_isStartClosest)
    {
      s.x = outlineLines[i].startPoint.x;
      s.y = outlineLines[i].startPoint.y;
      e.x = skelletonLines[idx_s_closest].startPoint.x;
      e.y = skelletonLines[idx_s_closest].startPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }
    else
    {
      s.x = outlineLines[i].startPoint.x;
      s.y = outlineLines[i].startPoint.y;
      e.x = skelletonLines[idx_s_closest].endPoint.x;
      e.y = skelletonLines[idx_s_closest].endPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }

    // If we start serach inside the image (because the tile border is inside the texture) we search outwards instead with a max search of the distance to the skelletal line
    Vector2dInt shift_s;
    if(!isBlack(s, srcImgMap, w, h)){
      shift_s = s;
    }else{
      // Binary Search Along Line
      shift_s = SearchAlongLineRec(s, d, s, srcImgMap, w, h, 0, false);
    }

    // Move Direction (End Point)
    if (e_isStartClosest)
    {
      s.x = outlineLines[i].endPoint.x;
      s.y = outlineLines[i].endPoint.y;
      e.x = skelletonLines[idx_e_closest].startPoint.x;
      e.y = skelletonLines[idx_e_closest].startPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }
    else
    {
      s.x = outlineLines[i].endPoint.x;
      s.y = outlineLines[i].endPoint.y;
      e.x = skelletonLines[idx_e_closest].endPoint.x;
      e.y = skelletonLines[idx_e_closest].endPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }

    Vector2dInt shift_e;
    // If we start serach inside the image (because the tile border is inside the texture) we search outwards instead with a max search of the distance to the skelletal line
    if(!isBlack(s, srcImgMap, w, h)){
      shift_e = s;
    }else{
      // Binary Search Along Line
      shift_e = SearchAlongLineRec(s, d, s, srcImgMap, w, h, 0, false);
    }

    outlineLinesMorphed.push_back(FeatureLine(Point(shift_s), Point(shift_e)));

  }
  return outlineLinesMorphed;
}

void removeZeroLengthLines(vector<FeatureLine> &outlineLinesSorted, vector<FeatureLine> &outlineLinesMorphed)
{
  // Remove lines of length zero in the result because they cause NaN problems later
  for (int i = outlineLinesMorphed.size() - 1; i >= 0; i--)
  {
    double lx = outlineLinesMorphed[i].startPoint.x - outlineLinesMorphed[i].endPoint.x;
    double ly = outlineLinesMorphed[i].startPoint.y - outlineLinesMorphed[i].endPoint.y;
    double l = lx * lx + ly * ly; // sqrt omited
    if (l == 0)
    {
      outlineLinesSorted.erase(outlineLinesSorted.begin() + i);
      outlineLinesMorphed.erase(outlineLinesMorphed.begin() + i);
    }
  }
}

bool isBoundaryPoint(Vector2dInt c, pixel **srcImgMap, int w, int h)
{
  if(isBlack(c, srcImgMap, w, h)){
    return false;
  }else{
    Vector2dInt N(c.x, c.y-1);
    Vector2dInt S(c.x, c.y+1);
    Vector2dInt E(c.x+1, c.y);
    Vector2dInt W(c.x-1, c.y);
    return isBlack(N, srcImgMap, w, h)
        || isBlack(S, srcImgMap, w, h)
        || isBlack(E, srcImgMap, w, h)
        || isBlack(W, srcImgMap, w, h);
  }
}


Vector2dInt* neigh(Vector2dInt p){
      static Vector2dInt n[8];
      n[0].x = p.x-1;
      n[0].y = p.y-1;

      n[1].x = p.x;
      n[1].y = p.y-1;
      
      n[2].x = p.x+1;
      n[2].y = p.y-1;
      
      n[3].x = p.x+1;
      n[3].y = p.y;

      n[4].x = p.x+1;
      n[4].y = p.y+1;

      n[5].x = p.x;
      n[5].y = p.y+1;

      n[6].x = p.x-1;
      n[6].y = p.y+1;

      n[7].x = p.x-1;
      n[7].y = p.y;
      return n;
}

bool neighContain(Vector2dInt p, Vector2dInt g){
  if(p == g) return true;
  if(p.x + 1 == g.x && p.y == g.y) return true;
  if(p.x - 1 == g.x && p.y == g.y) return true;
  if(p.x == g.x && p.y + 1 == g.y) return true;
  if(p.x == g.x && p.y - 1 == g.y) return true;
  return false;
}

void subdivideAlongBoundary(vector<Vector2dInt>& boundaryPoints, vector<FeatureLine>::iterator& it, vector<FeatureLine>::iterator& it_o, vector<FeatureLine>& outlineLinesSorted, vector<FeatureLine>& outlineLinesMorphed, vector<FeatureLine>& result_inner, vector<FeatureLine>& result_outer, bool verbose = false){

  int l = boundaryPoints.size();
  int steps = (l / 20) + 1;

  if(steps > 1){
    vector<FeatureLine> subdiv_outer;
    vector<FeatureLine> subdiv_inner;
    Vector2d c_inner(
      (it->endPoint.x - it->startPoint.x) / ((double)steps),
      (it->endPoint.y - it->startPoint.y) / ((double)steps)
    );
    Vector2d c_outer(
      (it_o->endPoint.x - it_o->startPoint.x) / ((double)steps),
      (it_o->endPoint.y - it_o->startPoint.y) / ((double)steps)
    );

    for(int j = 0; j<steps; j++){
      
      FeatureLine ins_outer;
      ins_outer.startPoint.x = it_o->startPoint.x + c_outer.x * j;
      ins_outer.startPoint.y = it_o->startPoint.y + c_outer.y * j;
      ins_outer.endPoint.x = it_o->startPoint.x + c_outer.x * (j+1);
      ins_outer.endPoint.y = it_o->startPoint.y + c_outer.y * (j+1);
      subdiv_outer.push_back(ins_outer);
      result_outer.push_back(ins_outer);

      FeatureLine ins_inner;
      int idx_start = (l / steps) * j;
      int idx_end = (j == steps-1) ? (l-1) : ((l / steps) * (j+1));
      ins_inner.startPoint = boundaryPoints[idx_start];
      ins_inner.endPoint   = boundaryPoints[idx_end];
      subdiv_inner.push_back(ins_inner);
      result_inner.push_back(ins_inner);
    }
  }
  else{
      result_outer.push_back(*it_o);
      result_inner.push_back(*it);
  }

  boundaryPoints.clear();
}

void traceBoundary(
  vector<FeatureLine> &outlineLinesSorted, 
  vector<FeatureLine> &outlineLinesMorphed, 
  vector<FeatureLine> skelletonLines, 
  vector<FeatureLine> &result_inner,
  vector<FeatureLine> &result_outer,
  pixel **srcImgMap, int w, int h){  

  vector<FeatureLine>::iterator it_o = outlineLinesSorted.begin();
  vector<FeatureLine>::iterator it_end = outlineLinesMorphed.end();

  int idx_seg = 0;
  for (vector<FeatureLine>::iterator it = outlineLinesMorphed.begin(); it < it_end; ++it, ++it_o, ++idx_seg){
    if(!isBoundaryPoint(it->startPoint, srcImgMap, w, h) || !isBoundaryPoint(it->endPoint, srcImgMap, w, h))
    {
      result_inner.push_back(*it);
      result_outer.push_back(*it_o);
      continue;
    }

    FeatureLine prev_line;
    if(it == outlineLinesMorphed.begin()) 
      prev_line = outlineLinesMorphed[outlineLinesMorphed.size()-1];
    else
      prev_line = outlineLinesMorphed[idx_seg-1];

    vector<Vector2dInt> boundaryPoints;
    Vector2dInt p = it->startPoint;
    Vector2dInt b;
    int idx_b = -1;
    
    Vector2dInt *n = neigh(p);

    int nNEIGH = 8;
    for(int i = 0; i < nNEIGH; i++){
      if(isBlack(n[i], srcImgMap, w, h)){
        b = n[i];
        idx_b = i;
        break;
      }
    }

    bool backwardFound = false;
    bool forwardFoundAfterBackward = false;
    int cnt = 0;
    while(cnt++ < w*h){

      for(int i = 0; i < nNEIGH; i++){
        if(isBlack(n[i], srcImgMap, w, h)){
          idx_b = i;
          break;
        }
      }

      for(int i = 0; i < nNEIGH; i++){
        if(!isBlack(n[(idx_b + i) % nNEIGH], srcImgMap, w, h)){
          p = n[(idx_b + i) % nNEIGH];
          b = n[(idx_b + i - 1) % nNEIGH];
          idx_b = (idx_b + i - 1) % nNEIGH;
          boundaryPoints.push_back(p);
          n = neigh(p);
          break;
        }
      }

      if(p == it->endPoint)
      {
        if(backwardFound){

          boundaryPoints.clear();
          forwardFoundAfterBackward = true;
          boundaryPoints.push_back(it->endPoint);

        }else{

          if(!(p == it->endPoint))
            boundaryPoints.push_back(p);

          subdivideAlongBoundary(boundaryPoints, it, it_o, outlineLinesSorted, outlineLinesMorphed, result_inner, result_outer);
          
          break;
        }
      }
      else if(p == prev_line.startPoint){
        int l = boundaryPoints.size();
        backwardFound = true;
      }
      else if(p == it->startPoint)
      {
        if(forwardFoundAfterBackward){
          reverse(boundaryPoints.begin(),boundaryPoints.end());
          subdivideAlongBoundary(boundaryPoints, it, it_o, outlineLinesSorted, outlineLinesMorphed, result_inner, result_outer, true);
          break;
        }else{
          printf("found bad loop \n");
          break;
        }
      }
    }

    it_end = outlineLinesMorphed.end();

  }
}

void transformAll(vector<FeatureLine> &outlineLines, vector<double> M)
{
  for (int i = 0; i < outlineLines.size(); i++)
  {
    outlineLines[i].startPoint = transformPoint(outlineLines[i].startPoint, M);
    outlineLines[i].endPoint = transformPoint(outlineLines[i].endPoint, M);
  }
}

//--------------------------------------------------------------------------------------------------
//--------------------------morph-------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
EMSCRIPTEN_KEEPALIVE vector<FeatureLine> getMorphOutline(int w, int h, float t,
                                                         vector<unsigned char> imageData,
                                                         vector<FeatureLine> skelletonLines,
                                                         vector<FeatureLine> outlineLines,
                                                         vector<double> Minv)
{
  pixel **srcImgMap = pixmapFromVector(w, h, imageData);

  vector<FeatureLine> outlineLinesSorted = sortOutlineLines(outlineLines);

  transformAll(outlineLinesSorted, Minv);

  vector<FeatureLine> outlineLinesMorphed = projectOutlineLines(outlineLinesSorted, skelletonLines, srcImgMap, w, h);

  removeZeroLengthLines(outlineLinesSorted, outlineLinesMorphed);

  vector<FeatureLine> outlineLinestraced_inner;
  vector<FeatureLine> outlineLinestraced_outer;
  traceBoundary(outlineLinesSorted, outlineLinesMorphed, skelletonLines, outlineLinestraced_inner, outlineLinestraced_outer, srcImgMap, w, h);

  vector<FeatureLine> srcLines;

  lineInterpolate(outlineLinestraced_outer, outlineLinestraced_inner, srcLines, t);

  return srcLines;
}

EMSCRIPTEN_KEEPALIVE vector<int> getBBox(vector<FeatureLine> outlineLines, vector<double> matrixVector)
{
  int xl = std::numeric_limits<int>::max();
  int xh = std::numeric_limits<int>::min();
  int yl = std::numeric_limits<int>::max();
  int yh = std::numeric_limits<int>::min();
  for (int i = 0; i < outlineLines.size(); i++)
  {
    Vector2d s = transformPoint(outlineLines[i].startPoint, matrixVector);
    Vector2d e = transformPoint(outlineLines[i].endPoint, matrixVector);

    if (s.x < xl)
      xl = (int)floor(s.x);
    if (s.x > xh)
      xh = (int)ceil(s.x);
    if (e.x < xl)
      xl = (int)floor(e.x);
    if (e.x > xh)
      xh = (int)ceil(e.x);

    if (s.y < yl)
      yl = (int)floor(s.y);
    if (s.y > yh)
      yh = (int)ceil(s.y);
    if (e.y < yl)
      yl = (int)floor(e.y);
    if (e.y > yh)
      yh = (int)ceil(e.y);
  }

  vector<int> result;
  result.push_back(xl);
  result.push_back(yl);
  result.push_back(xh);
  result.push_back(yh);

  return result;
}

EMSCRIPTEN_KEEPALIVE vector<unsigned char> doMorph(int w, int h, float p, float a, float b, float t,
                                                   vector<unsigned char> imageData,
                                                   vector<unsigned char> imageDataProcessed,
                                                   vector<FeatureLine> skelletonLines,
                                                   vector<FeatureLine> outlineLines,
                                                   vector<double> matrixVector)
{

  pixel **srcImgMap = pixmapFromVector(w, h, imageData);
  pixel **srcImgMapProcessed = pixmapFromVector(w, h, imageDataProcessed);

  vector<int> bbox = getBBox(outlineLines, matrixVector);
  int xl = bbox[0];
  int yl = bbox[1];
  int xh = bbox[2];
  int yh = bbox[3];
  int w_dest = xh - xl;
  int h_dest = yh - yl;

  pixel **morphMap = allocPixmap(w_dest, h_dest);

  vector<FeatureLine> outlineLinesSorted = sortOutlineLines(outlineLines);

  transformAll(outlineLinesSorted, matrixVector);
  transformAll(outlineLines, matrixVector);

  vector<FeatureLine> outlineLinesMorphed = projectOutlineLines(outlineLinesSorted, skelletonLines, srcImgMapProcessed, w, h);

  removeZeroLengthLines(outlineLinesSorted, outlineLinesMorphed);
  
  vector<FeatureLine> outlineLinestraced_inner;
  vector<FeatureLine> outlineLinestraced_outer;
  traceBoundary(outlineLinesSorted, outlineLinesMorphed, skelletonLines, outlineLinestraced_inner, outlineLinestraced_outer, srcImgMapProcessed, w, h);

  // the featureline of sourceImage, destImage and the morphImage
  vector<FeatureLine> srcLines;
  vector<FeatureLine> dstLines;

  // srcLines.insert(srcLines.end(), skelletonLines.begin(), skelletonLines.end());
  // dstLines.insert(dstLines.end(), skelletonLines.begin(), skelletonLines.end());

  dstLines.insert(dstLines.end(), outlineLinestraced_outer.begin(), outlineLinestraced_outer.end());
  lineInterpolate(outlineLinestraced_outer, outlineLinestraced_inner, srcLines, t);

  Vector2d uv_src;
  Vector2d uv_dst;
  pixel interColor;

  for (int i = yl; i < yh; i++)
  {
    for (int j = xl; j < xh; j++)
    {
      uv_dst.x = j;
      uv_dst.y = i;

      // warping
      warp(uv_dst, srcLines, dstLines, p, a, b, uv_src);

      bool outside = false;
      if (uv_src.x < 0)
      {
        uv_src.x = 0;
        outside = true;
      }
      if (uv_src.x > w - 1)
      {
        uv_src.x = w - 1;
        outside = true;
      }
      if (uv_src.y < 0)
      {
        uv_src.y = 0;
        outside = true;
      }
      if (uv_src.y > h - 1)
      {
        uv_src.y = h - 1;
        outside = true;
      }

      if (outside)
      {
        morphMap[i - yl][j - xl].r = 0;
        morphMap[i - yl][j - xl].g = 0;
        morphMap[i - yl][j - xl].b = 0;
        morphMap[i - yl][j - xl].a = 255;
      }
      else
      {

        pixel bilin = bilinear(srcImgMap, uv_src.y, uv_src.x);

        morphMap[i - yl][j - xl].r = bilin.r;
        morphMap[i - yl][j - xl].g = bilin.g;
        morphMap[i - yl][j - xl].b = bilin.b;
        morphMap[i - yl][j - xl].a = bilin.a;
      }
    }
  }

  vector<unsigned char> result = vectorFromPixmap(w_dest, h_dest, morphMap);

  // clear the previous pixmap
  delete[] srcImgMap;
  delete[] srcImgMapProcessed;
  delete[] morphMap;

  return result;
}

// Binding code
EMSCRIPTEN_BINDINGS(myvoronoi)
{
  register_vector<unsigned char>("VectorByte");
  register_vector<double>("VectorDouble");
  register_vector<int>("VectorInt");
  register_vector<FeatureLine>("VectorFeatureLine");

  value_object<Point>("Point")
      .field("x", &Point::x)
      .field("y", &Point::y);

  value_object<FeatureLine>("FeatureLine")
      .field("startPoint", &FeatureLine::startPoint)
      .field("endPoint", &FeatureLine::endPoint);

  emscripten::function("doMorph", &doMorph);
  emscripten::function("getMorphOutline", &getMorphOutline);
  emscripten::function("getBBox", &getBBox);
}
