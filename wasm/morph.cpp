#include <emscripten/bind.h>
using namespace emscripten;

using namespace std;

#include "geometricTool.h"


//The width and height of the picture
int imgWidthOrig = 0;
int imgHeightOrig = 0;
int imgWidthDest = 0;
int imgHeightDest = 0;

//the pixel
typedef struct pix{
  unsigned char r,g,b,a;
} pixel;

//The pixmap is the pointer array which contains the pointers that point at the pixels
pixel ** srcImgMap;
pixel ** dstImgMap;
pixel ** morphMap;

//the featureline of sourceImage, destImage and the morphImage
vector<FeatureLine> srcLines;
vector<FeatureLine> dstLines;
vector<FeatureLine> morphLines;

//the GUI variables
int currentButton;
int currentPhase = 0;
Point * hitpoint;
Point * startPoint;

//The name of input and output files
string inputFileOrig;
string inputFileDest;
string outputFile;
//the parameter of the weight
string pStr, aStr, bStr, tStr;
float p = 0;
float a = 1;
float b = 2;
float t = 0.5;



//--------------------------------------------------------------------------
//------------------------allocPixmap---------------------------------------
//--------------------------------------------------------------------------
void allocPixmap(int w, int h, pixel ** &map){
  //clear the previous pixmap
  delete [] map;
  map = new pixel* [h];
  map[0] = new pixel[w * h];
  
  for(int i = 1; i < h; i++){  
    map[i] = map[i - 1] + w;
  }

  for(int row = 0; row < h; row++)
    for(int col = 0; col < w; col++)
      map[row][col].r = map[row][col].g = map[row][col].b = map[row][col].a = 255;
}

void pixmapFromVector(int w, int h, std::vector<unsigned char> imageData, pixel** &map){
  //clear the previous pixmap
  delete [] map;
  map = new pixel* [h];
  map[0] = new pixel[w * h];
  
  for(int i = 1; i < h; i++){  
    map[i] = map[i - 1] + w;
  }
  size_t idx = 0;
  for(int row = 0; row < h; row++){
    for(int col = 0; col < w; col++){
      map[row][col].r = imageData[idx++];
      map[row][col].g = imageData[idx++];
      map[row][col].b = imageData[idx++];
      map[row][col].a = imageData[idx++];
    }
  }
}

std::vector<unsigned char> vectorFromPixmap(int w, int h, pixel** &map){
  std::vector<unsigned char> result;
  for(int row = 0; row < h; row++){
    for(int col = 0; col < w; col++){
      result.push_back(map[row][col].r);
      result.push_back(map[row][col].g);
      result.push_back(map[row][col].b);
      result.push_back(map[row][col].a);
    }
  }
  return result;
}


//--------------------------------------------------------------------------------------------------
//--------------------------line interpolating function---------------------------------------------
//--------------------------------------------------------------------------------------------------
void lineInterpolate(const vector<FeatureLine>& sourceLines, 
                     const vector<FeatureLine>& destLines , vector<FeatureLine>& interLines, float t)
{
  interLines.reserve(sourceLines.size());
	int i;
	for(i=0;i<sourceLines.size();i++){
	  interLines[i].startPoint = (1-t)*(sourceLines[i].startPoint) + t*(destLines[i].startPoint);
	  interLines[i].endPoint = (1-t)*(sourceLines[i].endPoint) + t*(destLines[i].endPoint);
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
void warp(const Vector2d& interPt, const vector<FeatureLine>& interLines,
          const vector<FeatureLine>& sourceLines,
          float p, float a, float b, Vector2d& src)
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

	for (i=0; i<sourceLines.size(); i++) {
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
        else if (u > 1) {
          qd.x = interPt.x - interLines[i].endPoint.x;
		      qd.y = interPt.y - interLines[i].endPoint.y;
          dist = sqrt(qd.x * qd.x + qd.y * qd.y);
        }else{
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
void bilinear(pixel ** &Im, float row, float col, pixel& pix)
{
	int cm, cn, fm, fn;
	double alpha, beta;

	cm = (int)ceil(row);
	fm = (int)floor(row);
	cn = (int)ceil(col);
	fn = (int)floor(col);
	alpha = ceil(row) - row;
	beta = ceil(col) - col;
	
	pix.r = (unsigned int)( alpha*beta*Im[fm][fn].r
	                          + (1-alpha)*beta*Im[cm][fn].r 
	                          + alpha*(1-beta)*Im[fm][cn].r
	                          + (1-alpha)*(1-beta)*Im[cm][cn].r );
	pix.g = (unsigned int)( alpha*beta*Im[fm][fn].g
		                        + (1-alpha)*beta*Im[cm][fn].g 
		                        + alpha*(1-beta)*Im[fm][cn].g
		                        + (1-alpha)*(1-beta)*Im[cm][cn].g );
	pix.b = (unsigned int)( alpha*beta*Im[fm][fn].b
		                        + (1-alpha)*beta*Im[cm][fn].b 
		                        + alpha*(1-beta)*Im[fm][cn].b
		                        + (1-alpha)*(1-beta)*Im[cm][cn].b );
  pix.a = 255;
}
//---------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------color interpolation function--------------------------------------------
//--------------------------------------------------------------------------------------------------
void ColorInterPolate(const Vector2d& Src_P, 
                      const Vector2d& Dest_P, 
                      float t, pixel ** &imgSrc,
                      pixel ** &imgDest, pixel& rgb)
{
    pixel srcColor, destColor;

    //bilinear(imgSrc, Src_P.x, Src_P.y, srcColor);
    //bilinear(imgDest, Dest_P.x, Dest_P.y, destColor);
    bilinear(imgSrc, Src_P.y, Src_P.x, srcColor);
    bilinear(imgDest, Dest_P.y, Dest_P.x, destColor);

    rgb.b = srcColor.b*(1-t)+ destColor.b*t;
    rgb.g = srcColor.g*(1-t)+ destColor.g*t;
    rgb.r = srcColor.r*(1-t)+ destColor.r*t;
}
//---------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------morph-------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

EMSCRIPTEN_KEEPALIVE std::vector<unsigned char> doMorph(int w, int h, std::vector<unsigned char> imageData){

    // printf("px0 %d %d %d %d \n", imageData[0], imageData[1], imageData[2], imageData[3]);
    printf("w h %d %d \n", w, h);
    // assign to new IMAGE
    allocPixmap(imgWidthOrig, imgHeightOrig, morphMap);

    pixmapFromVector(w, h, imageData, srcImgMap);

  	int i, j; //for the for loop
  	Vector2d dest, src;
    Vector2d q;
    pixel interColor;

    // line interpolation
  	lineInterpolate(srcLines, dstLines, morphLines, t);

  	for (i=0; i<imgHeightOrig; i++) {
	    for (j=0; j<imgWidthOrig; j++) {
          q.x = j;
          q.y = i;
          // warping
    	   	warp(q, morphLines, srcLines, p, a, b, src);
	       	// warp(q, morphLines, dstLines, p, a, b, dest);

              if (src.x < 0)
                  src.x = 0;
              if (src.x > imgWidthOrig-1)
                  src.x = imgWidthOrig - 1;
              if (src.y < 0)
                  src.y = 0;
              if (src.y > imgHeightOrig-1)
                  src.y = imgHeightOrig - 1;
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
          ColorInterPolate(src, src, t, srcImgMap, srcImgMap, interColor);
          
          morphMap[i][j].r = interColor.r;
          morphMap[i][j].g = interColor.g;
          morphMap[i][j].b = interColor.b;
          morphMap[i][j].a = interColor.a;
	    }
   }
   return vectorFromPixmap(w, h, morphMap);

}


// // Binding code
EMSCRIPTEN_BINDINGS(myvoronoi) {
  register_vector<unsigned char>("VectorByte");

  emscripten::function("doMorph", &doMorph);

}
