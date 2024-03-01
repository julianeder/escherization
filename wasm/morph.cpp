// Source: https://github.com/OneSlashNinja/FeatureBasedImageMorphing

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
float p = 0;
float a = 1;
float b = 2;
float t = 0.5;

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
      map[row][col].r = map[row][col].g = map[row][col].b = map[row][col].a = 255;
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
  for (int row = 0; row < h; row++)
  {
    for (int col = 0; col < w; col++)
    {
      // result->push_back((map[row][col].r + 100) % 255);
      result.push_back(map[row][col].r);
      result.push_back(map[row][col].g);
      result.push_back(map[row][col].b);
      result.push_back(map[row][col].a);
    }
    // printf("row %zu \n", result->size());
  }
  return result;
}

//--------------------------------------------------------------------------------------------------
//--------------------------line interpolating function---------------------------------------------
//--------------------------------------------------------------------------------------------------
void lineInterpolate(const vector<FeatureLine> &sourceLines,
                     const vector<FeatureLine> &destLines, vector<FeatureLine> &interLines, float t)
{
  interLines.reserve(sourceLines.size());
  int i;
  for (i = 0; i < sourceLines.size(); i++)
  {
    interLines[i].startPoint = (1 - t) * (sourceLines[i].startPoint) + t * (destLines[i].startPoint);
    interLines[i].endPoint = (1 - t) * (sourceLines[i].endPoint) + t * (destLines[i].endPoint);
  }
}
//---------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------warp function-----------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/* warping function (backward mapping)
   input:
   interPt = the point in the intermediary image
   interLines = given line in the intermediary image
   srcLines = given line in the source image
   p, a, b = parameters of the weight function
   output:
   src = the corresponding point */
void warp(const Vector2d &interPt, const vector<FeatureLine> &interLines,
          const vector<FeatureLine> &sourceLines,
          float p, float a, float b, Vector2d &src)
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

  for (i = 0; i < sourceLines.size(); i++)
  {
    pd.x = interPt.x - interLines[i].startPoint.x;
    pd.y = interPt.y - interLines[i].startPoint.y;
    pq.x = interLines[i].endPoint.x - interLines[i].startPoint.x;
    pq.y = interLines[i].endPoint.y - interLines[i].startPoint.y;
    interLength = pq.x * pq.x + pq.y * pq.y;
    u = (pd.x * pq.x + pd.y * pq.y) / interLength;

    interLength = sqrt(interLength); // length of the vector PQ

    v = (pd.x * pq.y - pd.y * pq.x) / interLength;

    pq.x = sourceLines[i].endPoint.x - sourceLines[i].startPoint.x;
    pq.y = sourceLines[i].endPoint.y - sourceLines[i].startPoint.y;

    srcLength = sqrt(pq.x * pq.x + pq.y * pq.y); // length of the vector P'Q'
    // corresponding point based on the ith line
    X = sourceLines[i].startPoint.x + u * pq.x + v * pq.y / srcLength;
    Y = sourceLines[i].startPoint.y + u * pq.y - v * pq.x / srcLength;

    // the distance from the corresponding point to the line P'Q'
    if (u < 0)
      dist = sqrt(pd.x * pd.x + pd.y * pd.y);
    else if (u > 1)
    {
      qd.x = interPt.x - interLines[i].endPoint.x;
      qd.y = interPt.y - interLines[i].endPoint.y;
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

  src.x = sum_x / weightSum;
  src.y = sum_y / weightSum;
}

//--------------------------------------------------------------------------------------------------
//--------------------------bilinear interpolation--------------------------------------------------
//--------------------------------------------------------------------------------------------------
void bilinear(pixel **&Im, float row, float col, pixel &pix)
{
  int cm, cn, fm, fn;
  double alpha, beta;

  cm = (int)ceil(row);
  fm = (int)floor(row);
  cn = (int)ceil(col);
  fn = (int)floor(col);
  alpha = ceil(row) - row;
  beta = ceil(col) - col;

  pix.r = (unsigned int)(alpha * beta * Im[fm][fn].r + (1 - alpha) * beta * Im[cm][fn].r + alpha * (1 - beta) * Im[fm][cn].r + (1 - alpha) * (1 - beta) * Im[cm][cn].r);
  pix.g = (unsigned int)(alpha * beta * Im[fm][fn].g + (1 - alpha) * beta * Im[cm][fn].g + alpha * (1 - beta) * Im[fm][cn].g + (1 - alpha) * (1 - beta) * Im[cm][cn].g);
  pix.b = (unsigned int)(alpha * beta * Im[fm][fn].b + (1 - alpha) * beta * Im[cm][fn].b + alpha * (1 - beta) * Im[fm][cn].b + (1 - alpha) * (1 - beta) * Im[cm][cn].b);
  pix.a = 255;
}
//---------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------color interpolation function--------------------------------------------
//--------------------------------------------------------------------------------------------------
void ColorInterPolate(const Vector2d &Src_P,
                      const Vector2d &Dest_P,
                      float t, pixel **&imgSrc,
                      pixel **&imgDest, pixel &rgb)
{
  pixel srcColor, destColor;

  // bilinear(imgSrc, Src_P.x, Src_P.y, srcColor);
  // bilinear(imgDest, Dest_P.x, Dest_P.y, destColor);
  bilinear(imgSrc, Src_P.y, Src_P.x, srcColor);
  bilinear(imgDest, Dest_P.y, Dest_P.x, destColor);

  rgb.b = srcColor.b * (1 - t) + destColor.b * t;
  rgb.g = srcColor.g * (1 - t) + destColor.g * t;
  rgb.r = srcColor.r * (1 - t) + destColor.r * t;
}

bool approximatelyEqual(float a, float b, float epsilon)
{
  return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

vector<FeatureLine> sortOutlineLines(vector<FeatureLine> &outlineLines)
{
  printf("outlineLines size  %zu \n", outlineLines.size());
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
      // printf("%f == %f && %f == %f \n", outlineLines[i].endPoint.x, outlineLines[j].startPoint.x,
      // outlineLines[i].endPoint.y, outlineLines[j].startPoint.y);
      // printf("j e  %f %f \n", outlineLines[j].endPoint.x, outlineLines[j].endPoint.y);
      if (approximatelyEqual(outlineLines[i].endPoint.x, outlineLines[j].startPoint.x, 1e-2f) && approximatelyEqual(outlineLines[i].endPoint.y, outlineLines[j].startPoint.y, 1e-2f))
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
  if (c.x < 0 || c.x > w)
    return true;
  if (c.y < 0 || c.y > h)
    return true;

  pixel pix = srcImgMap[c.y][c.x];
  return pix.r == 0 && pix.g == 0 && pix.b == 0 && pix.a == 255;
}

Vector2dInt SearchAlongLineRec(Vector2d s, Vector2d d, Vector2dInt prev_c, pixel **srcImgMap, int w, int h, int depth)
{
  Vector2d center = s + (d / 2.0);
  Vector2dInt c(center);
  if (c == prev_c)
    return prev_c;

  printf("depth %d\n", depth);
  printf("s (%f %f)\n", s.x, s.y);
  printf("d (%f %f)\n", d.x, d.y);
  printf("d/2 (%f %f)\n", (d / 2.0).x, (d / 2.0).y);
  printf("center (%f %f)\n", center.x, center.y);

  if (isBlack(c, srcImgMap, w, h))
  {
    printf("B c (%d %d)\n", c.x, c.y);
    return SearchAlongLineRec(center, d / 2.0, c, srcImgMap, w, h, depth + 1);
  }
  else
  {
    printf("W c (%d %d)\n", c.x, c.y);
    return SearchAlongLineRec(s, d / 2.0, c, srcImgMap, w, h, depth + 1);
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

void projectOutlineLines(vector<FeatureLine> &outlineLines, vector<FeatureLine> skelletonLines, pixel **srcImgMap, vector<double> M, int w, int h)
{
  for (int i = 0; i < outlineLines.size(); i++)
  {
    // printf("outlineLines[%d].startPoint (%f %f)\n", i, outlineLines[i].startPoint.x,outlineLines[i].startPoint.y);
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
      s = transformPoint(s, M);
      e.x = skelletonLines[j].startPoint.x;
      e.y = skelletonLines[j].startPoint.y;
      float d_s_s = (s.x - e.x) * (s.x - e.x) + (s.y - e.y) * (s.y - e.y);

      e.x = skelletonLines[j].endPoint.x;
      e.y = skelletonLines[j].endPoint.y;
      float d_s_e = (s.x - e.x) * (s.x - e.x) + (s.y - e.y) * (s.y - e.y);

      s.x = outlineLines[i].endPoint.x;
      s.y = outlineLines[i].endPoint.y;
      s = transformPoint(s, M);
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
      s = transformPoint(s, M);
      e.x = skelletonLines[idx_s_closest].startPoint.x;
      e.y = skelletonLines[idx_s_closest].startPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }
    else
    {
      s.x = outlineLines[i].startPoint.x;
      s.y = outlineLines[i].startPoint.y;
      s = transformPoint(s, M);
      e.x = skelletonLines[idx_s_closest].endPoint.x;
      e.y = skelletonLines[idx_s_closest].endPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }
    // Binary Search Along Line

    printf("SearchAlongLine\n");
    printf("start (%f %f)\n", s.x, s.y);
    printf("end %d (%f %f)\n",idx_s_closest, e.x, e.y);
    printf("dir (%f %f)\n", d.x, d.y);

    Vector2dInt shift_s = SearchAlongLineRec(s, d, s, srcImgMap, w, h, 0);

    // Move Direction (End Point)
    // Vector2d d, s, e;
    if (e_isStartClosest)
    {
      s.x = outlineLines[i].endPoint.x;
      s.y = outlineLines[i].endPoint.y;
      s = transformPoint(s, M);
      e.x = skelletonLines[idx_s_closest].startPoint.x;
      e.y = skelletonLines[idx_s_closest].startPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }
    else
    {
      s.x = outlineLines[i].endPoint.x;
      s.y = outlineLines[i].endPoint.y;
      s = transformPoint(s, M);
      e.x = skelletonLines[idx_s_closest].endPoint.x;
      e.y = skelletonLines[idx_s_closest].endPoint.y;
      d.x = e.x - s.x;
      d.y = e.y - s.y;
    }
    // Binary Search Along Line

    printf("SearchAlongLine\n");
    Vector2dInt shift_e = SearchAlongLineRec(s, d, s, srcImgMap, w, h, 0);

    // printf("s (%f %f) -> (%d %d)\n", outlineLines[i].startPoint.x, outlineLines[i].startPoint.y, shift_s.x, shift_s.y);
    // printf("e (%f %f) -> (%d %d)\n", outlineLines[i].endPoint.x, outlineLines[i].endPoint.y, shift_e.x, shift_e.y);

    outlineLines[i].startPoint.x = shift_s.x;
    outlineLines[i].startPoint.y = shift_s.y;
    outlineLines[i].endPoint.x = shift_e.x;
    outlineLines[i].endPoint.y = shift_e.y;

    // if (i > 1)
    //   break;
  }
}

//---------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------morph-------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
EMSCRIPTEN_KEEPALIVE vector<FeatureLine> getMorphOutline(int w, int h,
                                                         vector<unsigned char> imageData,
                                                         vector<FeatureLine> skelletonLines,
                                                         vector<FeatureLine> outlineLines,
                                                         vector<double> matrixVector)
{
  // printf("(%f %d) -> (%f %f)\n", 123.20508075629999, 150, transformPoint(Vector2d(123.20508075629999, 150) ,matrixVector).x, transformPoint(Vector2d(123.20508075629999, 150) ,matrixVector).y);
  // printf("(%f %d) -> (%f %f)\n", 223.20508075629999, 250, transformPoint(Vector2d(223.20508075629999, 250) ,matrixVector).x, transformPoint(Vector2d(223.20508075629999, 250) ,matrixVector).y);

  // // pixel ** dstImgMap;
  pixel **srcImgMap = pixmapFromVector(w, h, imageData);

  printf("skelletonLines size  %zu \n", skelletonLines.size());

  vector<FeatureLine> outlineLinesSorted = sortOutlineLines(outlineLines);
  projectOutlineLines(outlineLinesSorted, skelletonLines, srcImgMap, matrixVector, w, h);

  return outlineLinesSorted;
}

EMSCRIPTEN_KEEPALIVE vector<unsigned char> doMorph(int w, int h,
                                                   vector<unsigned char> imageData,
                                                   vector<FeatureLine> skelletonLines,
                                                   vector<FeatureLine> outlineLines,
                                                   vector<double> matrixVector)
{

  // // pixel ** dstImgMap;
  pixel **morphMap = allocPixmap(w, h);
  pixel **srcImgMap = pixmapFromVector(w, h, imageData);

  printf("skelletonLines size  %zu \n", skelletonLines.size());

  vector<FeatureLine> outlineLinesSorted = sortOutlineLines(outlineLines);
  projectOutlineLines(outlineLinesSorted, skelletonLines, srcImgMap, matrixVector, w, h);

  // the featureline of sourceImage, destImage and the morphImage
  vector<FeatureLine> srcLines;
  // // vector<FeatureLine> dstLines;
  vector<FeatureLine> morphLines;

  // printf("px0 %d %d %d %d \n", imageData[0], imageData[1], imageData[2], imageData[3]);
  // printf("w h %d %d %zu \n", w, h, imageData.size());
  // assign to new IMAGE

  // printf("morphMap %d %d %d %d \n", morphMap[0][0].r, morphMap[0][0].g, morphMap[0][0].b, morphMap[0][0].a);

  // printf("map1 %d %d %d %d \n", srcImgMap[0][0].r, srcImgMap[0][0].g, srcImgMap[0][0].b, srcImgMap[0][0].a);
  Vector2d dest, src;
  Vector2d q;
  pixel interColor;

  // line interpolation
  // lineInterpolate(srcLines, dstLines, morphLines, t);

  for (int i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++)
    {
      q.x = j;
      q.y = i;
      // warping
      warp(q, morphLines, srcLines, p, a, b, src);
      // warp(q, morphLines, dstLines, p, a, b, dest);

      if (src.x < 0)
        src.x = 0;
      if (src.x > w - 1)
        src.x = w - 1;
      if (src.y < 0)
        src.y = 0;
      if (src.y > h - 1)
        src.y = h - 1;
      // if (dest.x < 0)
      //     dest.x = 0;
      // if (dest.x > imgWidthDest-1)
      //     dest.x = imgWidthDest - 1;
      // if (dest.y < 0)
      //     dest.y = 0;
      // if (dest.y > imgHeightDest-1)
      //     dest.y = imgHeightDest-1;

      // color interpolation
      // ColorInterPolate(src, dest, t, srcImgMap, dstImgMap, interColor);
      // ColorInterPolate(src, src, t, srcImgMap, srcImgMap, interColor);

      morphMap[i][j].r = interColor.r;
      morphMap[i][j].g = interColor.g;
      morphMap[i][j].b = interColor.b;
      morphMap[i][j].a = interColor.a;
    }
  }
  vector<unsigned char> result = vectorFromPixmap(w, h, srcImgMap);

  // printf("result %d %d %d %d \n", result[0], result[1], result[2], result[3]);
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
  register_vector<FeatureLine>("VectorFeatureLine");

  value_object<Point>("Point")
      .field("x", &Point::x)
      .field("y", &Point::y);

  value_object<FeatureLine>("FeatureLine")
      .field("startPoint", &FeatureLine::startPoint)
      .field("endPoint", &FeatureLine::endPoint);

  emscripten::function("doMorph", &doMorph);
  emscripten::function("getMorphOutline", &getMorphOutline);
}
