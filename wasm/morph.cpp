//
//    Source: https://github.com/OneSlashNinja/FeatureBasedImageMorphing
//    Copyright (C) OneSlashNinja
//    under: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//    modified by Julian Eder
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

// the GUI variables
int currentButton;
int currentPhase = 0;
Point *hitpoint;
Point *startPoint;

// The name of input and output files
string inputFileOrig;
string inputFileDest;
string outputFile;
// the parameter of the weight
string pStr, aStr, bStr, tStr;
// float p = 0;
// float a = 1;
// float b = 2;

//--------------------------------------------------------------------------
//------------------------allocPixmap---------------------------------------
//--------------------------------------------------------------------------
// If I compile with -O3 the compiler removes this function when it does not have a print statement in the loop - I dont know why
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
      // result->push_back((map[row][col].r + 100) % 255);
      result.push_back(map[y][x].r);
      result.push_back(map[y][x].g);
      result.push_back(map[y][x].b);
      result.push_back(map[y][x].a);
    }
    // printf("row %zu \n", result->size());
  }
  return result;
}

//--------------------------------------------------------------------------------------------------
//--------------------------line interpolating function---------------------------------------------
//--------------------------------------------------------------------------------------------------
void lineInterpolate(const vector<FeatureLine> sourceLines,
                     const vector<FeatureLine> destLines, vector<FeatureLine> &interLines, float t)
{
  // interLines->reserve(sourceLines->size());
  int i;
  for (i = 0; i < sourceLines.size(); i++)
  {
    FeatureLine f(
        (1 - t) * (sourceLines[i].startPoint) + t * (destLines[i].startPoint),
        (1 - t) * (sourceLines[i].endPoint) + t * (destLines[i].endPoint));
    interLines.push_back(f);
    // (*interLines)[i].startPoint = (1 - t) * ((*sourceLines[i]).startPoint) + t * ((*destLines[i]).startPoint);
    // (*interLines)[i].endPoint = (1 - t) * ((*sourceLines[i]).endPoint) + t * ((*destLines[i]).endPoint);
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

// //--------------------------------------------------------------------------------------------------
// //--------------------------color interpolation function--------------------------------------------
// //--------------------------------------------------------------------------------------------------
// void ColorInterPolate(const Vector2d &Src_P,
//                       // const Vector2d &Dest_P,
//                       // float t,
//                       pixel **&imgSrc,
//                       // pixel **&imgDest,
//                       pixel &rgb)
// {
//   pixel srcColor; //, destColor;

//   // bilinear(imgSrc, Src_P.x, Src_P.y, srcColor);
//   // bilinear(imgDest, Dest_P.x, Dest_P.y, destColor);
//   bilinear(imgSrc, Src_P.y, Src_P.x, srcColor);
//   // bilinear(imgDest, Dest_P.y, Dest_P.x, destColor);

//   rgb.b = srcColor.b; // * (1 - t) + destColor.b * t;
//   rgb.g = srcColor.g; // * (1 - t) + destColor.g * t;
//   rgb.r = srcColor.r; // * (1 - t) + destColor.r * t;
// }

bool approximatelyEqual(float a, float b, float epsilon)
{
  return fabs(a - b) <= epsilon;
  // return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

vector<FeatureLine> sortOutlineLines(vector<FeatureLine> &outlineLines)
{
  // printf("outlineLines size  %zu \n", outlineLines.size());
  vector<FeatureLine> sorted;
  int i = 0;
  int cnt = 0;
  do
  {
    bool found = false;
    // printf("i s  %f %f \n", outlineLines[i].startPoint.x, outlineLines[i].startPoint.y);
    // printf("i e  %f %f \n", outlineLines[i].endPoint.x, outlineLines[i].endPoint.y);
    for (int j = 0; j < outlineLines.size(); j++)
    {
      // printf("j e  %f %f \n", outlineLines[j].endPoint.x, outlineLines[j].endPoint.y);
      if (approximatelyEqual(outlineLines[i].endPoint.x, outlineLines[j].startPoint.x, 1e-3f) && approximatelyEqual(outlineLines[i].endPoint.y, outlineLines[j].startPoint.y, 1e-3f))
      {
        // printf("%f == %f && %f == %f \n", outlineLines[i].endPoint.x, outlineLines[j].startPoint.x,
        // outlineLines[i].endPoint.y, outlineLines[j].startPoint.y);
        // printf("i %d j %d \n", i, j);
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
  // printf("c %d %d\n",c.x, c.y);
  pixel pix = srcImgMap[c.y][c.x];
  return pix.r == 0 && pix.g == 0 && pix.b == 0 && pix.a == 255;
}

Vector2dInt SearchAlongLineRec(Vector2d s, Vector2d d, Vector2dInt prev_c, pixel **srcImgMap, int w, int h, int depth, bool inverse = false, bool verbose = false)
{
  Vector2d center = s + (d / 2.0);
  Vector2dInt c(center);
  if (c == prev_c)
    return prev_c;

  if(verbose) printf("depth %d\n", depth);
  if(verbose) printf("s (%f %f)\n", s.x, s.y);
  if(verbose) printf("d (%f %f)\n", d.x, d.y);
  if(verbose) printf("d/2 (%f %f)\n", (d / 2.0).x, (d / 2.0).y);
  if(verbose) printf("center (%f %f)\n", center.x, center.y);

  if(inverse){
    if (isBlack(c, srcImgMap, w, h))
    {
      if(verbose) printf("iB c (%d %d)\n", c.x, c.y);
      return SearchAlongLineRec(s, d / 2.0, c, srcImgMap, w, h, depth + 1, inverse, verbose);
    }
    else
    {
      if(verbose) printf("iW c (%d %d)\n", c.x, c.y);
      return SearchAlongLineRec(center, d / 2.0, c, srcImgMap, w, h, depth + 1, inverse, verbose);
    }
  }else{
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
}

Vector2d transformPoint(Vector2d v, vector<double> M)
{
  Vector2d r(
      M[0] * v.x + M[2] * v.y + M[4],
      M[1] * v.x + M[3] * v.y + M[5]);

  // printf("P (%f %f) -> (%f %f)\n",v.x,v.y,r.x,r.y);

  return r;
}

Vector2d transformDirection(Vector2d v, vector<double> M)
{
  Vector2d r(
      M[0] * v.x + M[2] * v.y,
      M[1] * v.x + M[3] * v.y);

  // printf("D (%f %f) -> (%f %f)\n",v.x,v.y,r.x,r.y);

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

    // s = transformPoint(s, M); // M: tilingSpace -> imageSpace
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
  vector<FeatureLine> result;
  for (int i = 0; i < outlineLines.size(); i++)
  {
    // printf("outlineLines[%d].startPoint (%f %f)\n", i, outlineLines[i].startPoint.x, outlineLines[i].startPoint.y);
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
      // s = transformPoint(s, M); // M: tilingSpace -> imageSpace
      e.x = skelletonLines[j].startPoint.x;
      e.y = skelletonLines[j].startPoint.y;
      float d_s_s = (s.x - e.x) * (s.x - e.x) + (s.y - e.y) * (s.y - e.y);

      e.x = skelletonLines[j].endPoint.x;
      e.y = skelletonLines[j].endPoint.y;
      float d_s_e = (s.x - e.x) * (s.x - e.x) + (s.y - e.y) * (s.y - e.y);

      s.x = outlineLines[i].endPoint.x;
      s.y = outlineLines[i].endPoint.y;
      // s = transformPoint(s, M);
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
      // s = transformPoint(s, M);
      e.x = skelletonLines[idx_s_closest].startPoint.x;
      e.y = skelletonLines[idx_s_closest].startPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;

      // printf("s skelletonLines[%d].startPoint (%f %f)\n", idx_s_closest, skelletonLines[idx_s_closest].startPoint.x, skelletonLines[idx_s_closest].startPoint.y);
      // printf("d (%f %f)\n", d.x, d.y);
    }
    else
    {
      s.x = outlineLines[i].startPoint.x;
      s.y = outlineLines[i].startPoint.y;
      // s = transformPoint(s, M);
      e.x = skelletonLines[idx_s_closest].endPoint.x;
      e.y = skelletonLines[idx_s_closest].endPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;

      // printf("e skelletonLines[%d].endPoint (%f %f)\n", idx_s_closest, skelletonLines[idx_s_closest].endPoint.x, skelletonLines[idx_s_closest].endPoint.y);
    }
    // Binary Search Along Line

    // printf("SearchAlongLine\n");
    // printf("start (%f %f)\n", s.x, s.y);
    // printf("end %d (%f %f)\n",idx_s_closest, e.x, e.y);
    // printf("dir (%f %f)\n", d.x, d.y);

    Vector2dInt shift_s = SearchAlongLineRec(s, d, s, srcImgMap, w, h, 0);
    // printf("shift_s (%d %d)\n", shift_s.x, shift_s.y);

    // Move Direction (End Point)
    if (e_isStartClosest)
    {
      s.x = outlineLines[i].endPoint.x;
      s.y = outlineLines[i].endPoint.y;
      // s = transformPoint(s, M);
      e.x = skelletonLines[idx_e_closest].startPoint.x;
      e.y = skelletonLines[idx_e_closest].startPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }
    else
    {
      s.x = outlineLines[i].endPoint.x;
      s.y = outlineLines[i].endPoint.y;
      // s = transformPoint(s, M);
      e.x = skelletonLines[idx_e_closest].endPoint.x;
      e.y = skelletonLines[idx_e_closest].endPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }
    // Binary Search Along Line

    // printf("SearchAlongLine\n");
    Vector2dInt shift_e = SearchAlongLineRec(s, d, s, srcImgMap, w, h, 0);

    // printf("s (%f %f) -> (%d %d)\n", outlineLines[i].startPoint.x, outlineLines[i].startPoint.y, shift_s.x, shift_s.y);
    // printf("e (%f %f) -> (%d %d)\n", outlineLines[i].endPoint.x, outlineLines[i].endPoint.y, shift_e.x, shift_e.y);

    result.push_back(FeatureLine(Point(shift_s), Point(shift_e)));

    // outlineLines[i].startPoint.x = shift_s.x;
    // outlineLines[i].startPoint.y = shift_s.y;
    // outlineLines[i].endPoint.x = shift_e.x;
    // outlineLines[i].endPoint.y = shift_e.y;

    // if (i > 1)
    //   break;
  }
  return result;
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
    // printf("sort %i (%f %f -> (%f %f)\n", i, outlineLinesMorphed[i].startPoint.x, outlineLinesMorphed[i].startPoint.y, outlineLinesMorphed[i].endPoint.x, outlineLinesMorphed[i].endPoint.y);
  }
}

// bool fitToImageBetter(vector<FeatureLine> &outlineLinesSorted, vector<FeatureLine> &outlineLinesMorphed, vector<FeatureLine> skelletonLines, pixel **srcImgMap, int w, int h)
// {
//       // printf("\n\nsize %ld\n", outlineLinesMorphed.size());



//   bool subdividesAnythng = false;
//   auto it_o = outlineLinesSorted.begin();
//   auto it_end = outlineLinesMorphed.end();
//   for (auto it = outlineLinesMorphed.begin(); it != it_end; ++it, ++it_o)
//   {
//     double lx = it->startPoint.x - it->endPoint.x;
//     double ly = it->startPoint.y - it->endPoint.y;
//     double l = lx * lx + ly * ly; // sqrt omited
//     // printf("l %f\n", l);

//     if(l > 5 * 5) //sqr
//     {
//       subdividesAnythng = true; 

//       Vector2d c_outer(
//         (it_o->startPoint.x + it_o->endPoint.x) / 2.0,
//         (it_o->startPoint.y + it_o->endPoint.y) / 2.0
//       );
      
//       FeatureLine ins_outer;
//       ins_outer.startPoint.x = c_outer.x;
//       ins_outer.startPoint.y = c_outer.y;
//       ins_outer.endPoint.x = it_o->endPoint.x;
//       ins_outer.endPoint.y = it_o->endPoint.y;
//       it_o = outlineLinesSorted.insert(it_o+1, ins_outer);
//       it_o--;
//       it_o->endPoint.x = c_outer.x;
//       it_o->endPoint.y = c_outer.y;   

//       Vector2d c_inner(
//         (it->startPoint.x + it->endPoint.x) / 2.0,
//         (it->startPoint.y + it->endPoint.y) / 2.0
//       );

//       FeatureLine ins_inner;
//       ins_inner.startPoint.x = c_inner.x;
//       ins_inner.startPoint.y = c_inner.y;
//       ins_inner.endPoint.x = it->endPoint.x;
//       ins_inner.endPoint.y = it->endPoint.y;
//       it = outlineLinesMorphed.insert(it+1, ins_inner);
//       it--;
//       it->endPoint.x = c_inner.x;
//       it->endPoint.y = c_inner.y;   




//       Vector2d d, s, e; 
//       s = c_inner;
//       bool black = isBlack(c_inner, srcImgMap, w, h);
//       if(black){ // Search toward center
//         e = closetsPointOnSkelleton(c_inner, skelletonLines);
//       }else{ // Search toward outside
//         e = c_outer;
//       }
//       d.x = e.x - s.x;
//       d.y = e.y - s.y;

//       Vector2dInt shift = SearchAlongLineRec(s, d, s, srcImgMap, w, h, 0, !black, false);

//       // printf("%ld %d s(%f %f) e(%f %f) d(%f %f) \n", it - outlineLinesMorphed.begin(), black, s.x, s.y, e.x, e.y, d.x, d.y);

//       // printf("(%f %f) -> (%d %d) \n", it->endPoint.x, it->endPoint.y, shift.x, shift.y);

//       it->endPoint = shift;
//       (it+1)->startPoint = shift;  


//       it++; // decrease if something was inserted, this processes the shortened again, therefore simulating recursive calling
//       it_o++;
//     }
//     it_end = outlineLinesMorphed.end();

//   }
//   return subdividesAnythng;
// }

void transformAll(vector<FeatureLine> &outlineLines, vector<double> M)
{
  for (int i = 0; i < outlineLines.size(); i++)
  {
    outlineLines[i].startPoint = transformPoint(outlineLines[i].startPoint, M);
    outlineLines[i].endPoint = transformPoint(outlineLines[i].endPoint, M);
  }
}

//---------------------------------------------------------------------------

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

  // printf("outlineLines\n");
  // for (int i = 0; i < outlineLines.size(); i++)
  // {
  //   printf("%d %f %f %f %f\n", i, outlineLines[i].startPoint.x, outlineLines[i].startPoint.y, outlineLines[i].endPoint.x, outlineLines[i].endPoint.y);
  // }

  vector<FeatureLine> outlineLinesSorted = sortOutlineLines(outlineLines);

  // printf("sorted\n");
  // for (int i = 0; i < outlineLinesSorted.size(); i++)
  // {
  //   printf("%d %f %f %f %f\n", i, outlineLinesSorted[i].startPoint.x, outlineLinesSorted[i].startPoint.y, outlineLinesSorted[i].endPoint.x, outlineLinesSorted[i].endPoint.y);
  // }
  transformAll(outlineLinesSorted, Minv);
  // printf("transformed\n");
  // for (int i = 0; i < outlineLinesSorted.size(); i++)
  // {
  //   printf("%d %f %f %f %f\n", i, outlineLinesSorted[i].startPoint.x, outlineLinesSorted[i].startPoint.y, outlineLinesSorted[i].endPoint.x, outlineLinesSorted[i].endPoint.y);
  // }

  vector<FeatureLine> outlineLinesMorphed = projectOutlineLines(outlineLinesSorted, skelletonLines, srcImgMap, w, h);

  // fitToImageBetter(outlineLinesSorted, outlineLinesMorphed, skelletonLines, srcImgMap, w, h);
  // fitToImageBetter(outlineLinesSorted, outlineLinesMorphed, skelletonLines, srcImgMap, w, h);
  // fitToImageBetter(outlineLinesSorted, outlineLinesMorphed, skelletonLines, srcImgMap, w, h);
  removeZeroLengthLines(outlineLinesSorted, outlineLinesMorphed);

      vector<FeatureLine>
          srcLines;

  lineInterpolate(outlineLinesSorted, outlineLinesMorphed, srcLines, t);

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
                                                   vector<FeatureLine> skelletonLines,
                                                   vector<FeatureLine> outlineLines,
                                                   vector<double> matrixVector)
{

  // // pixel ** dstImgMap;
  pixel **srcImgMap = pixmapFromVector(w, h, imageData);

  // printf("skelletonLines size  %zu \n", skelletonLines.size());

  vector<int> bbox = getBBox(outlineLines, matrixVector);
  int xl = bbox[0];
  int yl = bbox[1];
  int xh = bbox[2];
  int yh = bbox[3];
  int w_dest = xh - xl;
  int h_dest = yh - yl;

  // printf("xl %d xh %d yl %d yh %d\n",xl, xh, yl, yh);

  pixel **morphMap = allocPixmap(w_dest, h_dest);

  vector<FeatureLine> outlineLinesSorted = sortOutlineLines(outlineLines);

  transformAll(outlineLinesSorted, matrixVector);
  transformAll(outlineLines, matrixVector);
  // for (int i = 0; i < outlineLinesSorted.size(); i++)
  // {
  //   printf("sort %i %f %f %f %f\n", i, outlineLines[i].startPoint.x, outlineLines[i].endPoint.x, outlineLinesSorted[i].startPoint.x, outlineLinesSorted[i].endPoint.x);
  // }

  vector<FeatureLine> outlineLinesMorphed = projectOutlineLines(outlineLinesSorted, skelletonLines, srcImgMap, w, h);

  // TODO: Instead of this: follow the border and subdivide at the center pixel along the border
  // fitToImageBetter(outlineLinesSorted, outlineLinesMorphed, skelletonLines, srcImgMap, w, h);
  // fitToImageBetter(outlineLinesSorted, outlineLinesMorphed, skelletonLines, srcImgMap, w, h);
  // fitToImageBetter(outlineLinesSorted, outlineLinesMorphed, skelletonLines, srcImgMap, w, h); 
  removeZeroLengthLines(outlineLinesSorted, outlineLinesMorphed);
  printf("\n\nsize %ld\n", outlineLinesMorphed.size());

  // the featureline of sourceImage, destImage and the morphImage
  vector<FeatureLine> srcLines;
  vector<FeatureLine> dstLines;

  srcLines.insert(srcLines.end(), skelletonLines.begin(), skelletonLines.end());
  dstLines.insert(dstLines.end(), skelletonLines.begin(), skelletonLines.end());

  dstLines.insert(dstLines.end(), outlineLinesSorted.begin(), outlineLinesSorted.end());
  lineInterpolate(outlineLinesSorted, outlineLinesMorphed, srcLines, t);

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

      // if(i == yl && j == xl)
      //   printf("(%f %f) -> (%f %f) \n", uv_src.x, uv_src.y, uv_dst.x, uv_dst.y);
      // if(i == yh-1 && j == xh-1)
      //   printf("(%f %f) -> (%f %f) \n", uv_src.x, uv_src.y, uv_dst.x, uv_dst.y);
    }
  }

  // int cnt = 0;
  // for(int i = 0; i < yh-yl; i++){
  //   for(int j = 0; j < xh-xl; j++){

  //     if(morphMap[i][j].r != 0 || morphMap[i][j].g != 0 || morphMap[i][j].b != 0){
  //       printf("result %d %d %d %d %d \n", i,  morphMap[i][j].r, morphMap[i][j].g, morphMap[i][j].b, morphMap[i][j].a);
  //       cnt ++;
  //     }
  //     if(cnt > 10)
  //       break;
  //   }
  // }

  vector<unsigned char> result = vectorFromPixmap(w_dest, h_dest, morphMap);

  // clear the previous pixmap
  delete[] srcImgMap;
  delete[] morphMap;

  return result;
}

// // Binding code
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
