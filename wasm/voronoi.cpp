// Boost.Polygon library voronoi_basic_tutorial.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.
// #include <emscripten/emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;


#include <cstdio>
#include <vector>

#include <boost/polygon/voronoi.hpp>
#include <boost/polygon/polygon.hpp>

#include "voronoi_visual_utils.hpp"

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;
using boost::polygon::x;
using boost::polygon::y;
using boost::polygon::low;
using boost::polygon::high;
using namespace boost::polygon;


// // #include "voronoi_visual_utils.hpp"

struct Point {
  int a;
  int b;
  Point(int x, int y) : a(x), b(y) {}
};

struct Segment {
  Point p0;
  Point p1;
  Segment(int x1, int y1, int x2, int y2) : p0(x1, y1), p1(x2, y2) {}
};

namespace boost {
namespace polygon {

template <>
struct geometry_concept<Point> {
  typedef point_concept type;
};

template <>
struct point_traits<Point> {
  typedef int coordinate_type;

  static inline coordinate_type get(
      const Point& point, orientation_2d orient) {
    return (orient == HORIZONTAL) ? point.a : point.b;
  }
};

template <>
struct geometry_concept<Segment> {
  typedef segment_concept type;
};

template <>
struct segment_traits<Segment> {
  typedef int coordinate_type;
  typedef Point point_type;

  static inline point_type get(const Segment& segment, direction_1d dir) {
    return dir.to_int() ? segment.p1 : segment.p0;
  }
};
}  // polygon
}  // boost


  typedef double coordinate_type;
  typedef point_data<coordinate_type> point_type;
  typedef segment_data<coordinate_type> segment_type;
  typedef rectangle_data<coordinate_type> rect_type;
  typedef voronoi_builder<int> VB;
  typedef voronoi_diagram<coordinate_type> VD;
  typedef VD::cell_type cell_type;
  typedef VD::cell_type::source_index_type source_index_type;
  typedef VD::cell_type::source_category_type source_category_type;
  typedef VD::edge_type edge_type;
  typedef VD::cell_container_type cell_container_type;
  typedef VD::cell_container_type vertex_container_type;
  typedef VD::edge_container_type edge_container_type;
  typedef VD::const_cell_iterator const_cell_iterator;
  typedef VD::const_vertex_iterator const_vertex_iterator;
  typedef VD::const_edge_iterator const_edge_iterator;

// #ifdef __cplusplus
// #define EXTERN extern "C"
// #else
// #define EXTERN
// #endif

struct EdgeResult {
  double x1;
  double y1;
  double x2;
  double y2;
  bool isFinite;
  bool isCurved;
  bool isPrimary;
  std::vector<double> samples;
};
struct DiagrammResult {
  std::vector<double> vertices;
  std::vector<EdgeResult> edges;
  int numVerticies;
};

point_type retrieve_point(
  const voronoi_diagram<double>::cell_type& cell,   
  std::vector<point_type> pointSites,
  std::vector<segment_type> lineSites
  ) {
  voronoi_diagram<double>::cell_type::source_index_type index = cell.source_index();
  voronoi_diagram<double>::cell_type::source_category_type category = cell.source_category();
  if (category == boost::polygon::SOURCE_CATEGORY_SINGLE_POINT) {
    return pointSites[index];
  }
  index -= pointSites.size();
  if (category == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT) {
    return low(lineSites[index]);
  } else {
    return high(lineSites[index]);
  }
}

segment_type retrieve_segment(const cell_type& cell, std::vector<point_type> pointSites, std::vector<segment_type> lineSites) {
  source_index_type index = cell.source_index() - pointSites.size();
  return lineSites[index];
}

void sample_curved_edge(
    const edge_type& edge,
    std::vector<point_type>* sampled_edge,
    std::vector<point_type> pointSites,
    std::vector<segment_type> lineSites,
    std::vector<double> bbox) 
{
  double xl = bbox[0];
  double xh = bbox[2];
  coordinate_type max_dist = 1E-3 * (xh - xl);
  point_type point = edge.cell()->contains_point() ?
      retrieve_point(*edge.cell(), pointSites, lineSites) :
      retrieve_point(*edge.twin()->cell(), pointSites, lineSites);
  segment_type segment = edge.cell()->contains_point() ?
      retrieve_segment(*edge.twin()->cell(), pointSites, lineSites) :
      retrieve_segment(*edge.cell(), pointSites, lineSites);
  voronoi_visual_utils<coordinate_type>::discretize(
      point, segment, max_dist, sampled_edge, bbox);
}

void createVertex(
  const voronoi_diagram<double>::edge_type& edge, 
  EdgeResult* edgeResult, 
  double x, double y){
  if (edge.vertex0() == NULL) {
    edgeResult->x1 = x;
    edgeResult->y1 = y;
  } else {
    edgeResult->x1 = edge.vertex0()->x();
    edgeResult->y1 = edge.vertex0()->y();
  }
  if (edge.vertex1() == NULL) {
    edgeResult->x2 = x;
    edgeResult->y2 = y;
  } else {
    edgeResult->x2 = edge.vertex1()->x();
    edgeResult->y2 = edge.vertex1()->y();
  }
}


void clip_finite_edge(
  const voronoi_diagram<double>::edge_type& edge, 
  DiagrammResult* result,
  EdgeResult* edgeResult, 
  std::vector<point_type> pointSites,
  std::vector<segment_type> lineSites,
  std::vector<double> bbox
  ) {

    std::vector<point_type> samples_;
    double xl = bbox[0];
    double xh = bbox[2];
    double yl = bbox[1];
    double yh = bbox[3];

    // completely outside
    if((edge.vertex0()->x() <= xl && edge.vertex1()->x() <= xl)  
    || (edge.vertex0()->x() >= xh && edge.vertex1()->x() >= xh)  
    || (edge.vertex0()->y() <= yl && edge.vertex1()->y() <= yl)  
    || (edge.vertex0()->y() >= yh && edge.vertex1()->y() >= yh)){
      printf("compl outside \n");
      return; 
    } 


    point_type direction;
    direction.x(edge.vertex1()->x() - edge.vertex0()->x());
    direction.y(edge.vertex1()->y() - edge.vertex0()->y());
    double dxdy = direction.x() / direction.y();
    double dydx = direction.y() / direction.x();

    // Clip X1
    if(edge.vertex0()->x() < xl){
      edgeResult->x1 = edge.vertex0()->x() + (xl - edge.vertex0()->x()) ;
      edgeResult->y1 = edge.vertex0()->y() + (xl - edge.vertex0()->x()) * dydx;

      printf("clip x1a %f -> %f \n", 
        edge.vertex0()->x(),
        edgeResult->x1);
    }
    else if(edge.vertex0()->x() > xh){
      edgeResult->x1 = edge.vertex0()->x() + (xh - edge.vertex0()->x());
      edgeResult->y1 = edge.vertex0()->y() + (xh - edge.vertex0()->x()) * dydx;

      printf("clip x1b %f -> %f \n", 
        edge.vertex0()->x(),
        edgeResult->x1);
    }
    else {
      edgeResult->x1 = edge.vertex0()->x();
      edgeResult->y1 = edge.vertex0()->y();
    }

    // Clip X2
    if(edge.vertex1()->x() < xl){
      edgeResult->x2 = edge.vertex1()->x() + (xl - edge.vertex1()->x());
      edgeResult->y2 = edge.vertex1()->y() + (xl - edge.vertex1()->x()) * dydx;

      printf("clip x2a %f -> %f \n", 
        edge.vertex1()->x(),
        edgeResult->x2);
    }
    else if(edge.vertex1()->x() > xh){
      edgeResult->x2 = edge.vertex1()->x() + (xh - edge.vertex1()->x());
      edgeResult->y2 = edge.vertex1()->y() + (xh - edge.vertex1()->x()) * dydx;

      printf("clip x2b %f -> %f \n", 
        edge.vertex1()->x(),
        edgeResult->x2);
    }
    else {
      edgeResult->x2 = edge.vertex1()->x();
      edgeResult->y2 = edge.vertex1()->y();
    }

    // Clip Y1
    if(edgeResult->y1 < yl){
      edgeResult->x1 = edgeResult->x1 + (yl - edgeResult->y1) * dxdy;
      edgeResult->y1 = edgeResult->y1 + (yl - edgeResult->y1);
      printf("clip y1 \n");
    }
    else if(edgeResult->y1 > yh){
      if(edgeResult->y2 > yh) return; // completely outside
      edgeResult->x1 = edgeResult->x1 + (yh - edgeResult->y1) * dxdy;
      edgeResult->y1 = edgeResult->y1 + (yh - edgeResult->y1);
      printf("clip y1 \n");

    }
    // else // unchanged 

    // Clip Y2
    if(edgeResult->y2 < yl){
      edgeResult->x2 = edgeResult->x2 + (yl - edgeResult->y2) * dxdy;
      edgeResult->y2 = edgeResult->y2 + (yl - edgeResult->y2);
      printf("clip y2 \n");

    }
    else if(edgeResult->y2 > yh){
      edgeResult->x2 = edgeResult->x2 + (yh - edgeResult->y2) * dxdy;
      edgeResult->y2 = edgeResult->y2 + (yh - edgeResult->y2);
      printf("clip y2 \n");
    }
    // else // unchanged 

    printf("finite %d %f %f - %f %f \n", 
      edge.is_primary(),
      edgeResult->x1,
      edgeResult->y1,
      edgeResult->x2,
      edgeResult->y2);
    
    if (edge.is_curved()) { // only finite edges can be curved
      samples_.push_back(point_type(edgeResult->x1, edgeResult->y1));
      samples_.push_back(point_type(edgeResult->x2, edgeResult->y2));
      sample_curved_edge(edge, &samples_, pointSites, lineSites, bbox);
    }
    for (size_t i = 0; i < samples_.size(); i++)
    {
      edgeResult->samples.push_back(samples_[i].x());
      edgeResult->samples.push_back(samples_[i].y());
    }

    result->edges.push_back(*edgeResult);

}
void clip_infinite_edge(
  const voronoi_diagram<double>::edge_type& edge, 
  DiagrammResult* result,  
  EdgeResult* edgeResult, 
  std::vector<point_type> pointSites,
  std::vector<segment_type> lineSites,
  std::vector<double> bbox
  ) {
    if (edge.vertex0() == NULL)
      return;

    printf("infinite %d %f %f - %f %f \n", 
      edge.is_primary(),
      edge.vertex0()->x(),
      edge.vertex0()->y(),
      edge.vertex1()->x(),
      edge.vertex1()->y());

    const voronoi_diagram<double>::cell_type& cell1 = *edge.cell();
    const voronoi_diagram<double>::cell_type& cell2 = *edge.twin()->cell();
    point_type origin, direction;
    // Infinite edges could not be created by two segment sites.
    if (cell1.contains_point() && cell2.contains_point()) {
      point_type p1 = retrieve_point(cell1, pointSites, lineSites);
      point_type p2 = retrieve_point(cell2, pointSites, lineSites);
      printf("p1 %f %f p2 %f %f \n", 
        p1.x(),
        p1.y(),
        p2.x(),
        p2.y());
      origin.x((p1.x() + p2.x()) * 0.5);
      origin.y((p1.y() + p2.y()) * 0.5);
      direction.x(p1.y() - p2.y()); // orthogonal to the direction between the points
      direction.y(p2.x() - p1.x());
    } else {
      origin = cell1.contains_segment() ?
          retrieve_point(cell2, pointSites, lineSites) :
          retrieve_point(cell1, pointSites, lineSites);
      segment_type segment = cell1.contains_segment() ?
          retrieve_segment(cell1, pointSites, lineSites) :
          retrieve_segment(cell2, pointSites, lineSites);
      coordinate_type dx = high(segment).x() - low(segment).x();
      coordinate_type dy = high(segment).y() - low(segment).y();
      if ((low(segment) == origin) ^ cell1.contains_point()) {
        direction.x(dy);
        direction.y(-dx);
      } else {
        direction.x(-dy);
        direction.y(dx);
      }
    }

    //normalize
    double l = sqrt(direction.x()*direction.x() + direction.y()*direction.y());
    direction.x(direction.x() / l);
    direction.y(direction.y() / l);

    printf("  origin %f %f \n",
      origin.x(),
      origin.y());
    printf("  direction %f %f \n", 
      direction.x(),
      direction.y()
      );

    double xl = bbox[0];
    double xh = bbox[2];
    double yl = bbox[1];
    double yh = bbox[3];

    double fm, fb;

    if(direction.x() == 0){ // Vertical
      // doesn't intersect with viewport
      if (origin.x() < xl || origin.x() > xh) { 
        return; 
      }
      // downward
      if (direction.y() < 0) {
        // printf("v %f %f \n", 
        //   origin.x(),
        //   yl);
        createVertex(edge, edgeResult, origin.x(), yl);
      }
      // upward
      else {
        // printf("v %f %f \n", 
        //   origin.x(),
        //   yh);
        createVertex(edge, edgeResult, origin.x(), yh);
      }
    }
    
    else{
      // Ray Box Intersection
      // https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection.html
      double t0x = (xl - origin.x()) / direction.x(); // 0 - 400 / -0,12 =  3333,33
      double t0y = (yl - origin.y()) / direction.y(); // 0 - 250 / 0,991 = -252,27


      double t1x = (xh - origin.x()) / direction.x(); // 500 - 400 / -0,12 = -833,33
      double t1y = (yh - origin.y()) / direction.y(); // 300 - 250 / 0,991 = 50,45

      if(t0x < 0 && t1x > 0)
        std::swap(t0x, t1x);
      if(t0y < 0 && t1y > 0)
        std::swap(t0y, t1y);

      double tmin = (t0x < t0y) ? t0x : t0y; // (3333 < 50) = 50
      double tmax = (t1x > t1y) ? t1x : t1y; // (-833,33 > -252,27) = -252,27

      printf("t %f %f \n", 
        tmin,
        tmax);
      // the one in the same dir as the ray is >0
      if(tmin > 0){
        double cx = origin.x() + tmin * direction.x();
        double cy = origin.y() + tmin * direction.y();
        printf("c %f %f \n", 
          cx,
          cy);
        createVertex(edge, edgeResult, cx, cy);
      }
      else if(tmax > 0){
        double cx = origin.x() + tmax * direction.x();
        double cy = origin.y() + tmax * direction.y();
        printf("c %f %f \n", 
          cx,
          cy);
        createVertex(edge, edgeResult, cx, cy);
      }
    }
    result->edges.push_back(*edgeResult);
}

EMSCRIPTEN_KEEPALIVE DiagrammResult compute(std::vector<double> bbox, std::vector<int> points, std::vector<int> segments) {  
  std::vector<point_type> pointSites;
  std::vector<segment_type> lineSites;

  // printf("size %zu \n", 
  //         points.size());
  for (size_t i = 0; i < points.size(); i += 2)
  {
      pointSites.push_back(point_type(points[i], points[i+1]));
      // printf("pointSite %zu %f %f \n", 
      //     i,
      //     pointSites[i/2].x(),
      //     pointSites[i/2].y());
  }

  for (size_t i = 0; i < segments.size(); i += 4)
  {
      lineSites.push_back(segment_type(point_type(segments[i], segments[i+1]), point_type(segments[i+2], segments[i+3])));
  }


  // Construction of the Voronoi Diagram.
  voronoi_diagram<double> vd;
  construct_voronoi(pointSites.begin(), pointSites.end(),
                    lineSites.begin(), lineSites.end(),
                    &vd);

  DiagrammResult result;

  for (voronoi_diagram<double>::const_edge_iterator it = vd.edges().begin(); it != vd.edges().end(); ++it) {

    EdgeResult edgeResult;

    edgeResult.isFinite = it->is_finite();
    edgeResult.isPrimary = it->is_primary();
    edgeResult.isCurved = it->is_curved(); 
    
    if(it->is_finite()){
      clip_finite_edge(*it, &result, &edgeResult, pointSites, lineSites, bbox);      
    }else{
      clip_infinite_edge(*it, &result, &edgeResult, pointSites, lineSites, bbox);
    }

      
  }

  result.numVerticies = vd.num_vertices();
  for (voronoi_diagram<double>::const_vertex_iterator it = vd.vertices().begin(); it != vd.vertices().end(); ++it) {
    const voronoi_diagram<double>::vertex_type& vertex = *it;
    result.vertices.push_back(vertex.x());
    result.vertices.push_back(vertex.y());
  }

  return result;  
}


// // Binding code
EMSCRIPTEN_BINDINGS(myvoronoi) {
  register_vector<int>("VectorInt");
  register_vector<double>("VectorDouble");
  register_vector<EdgeResult>("VectorEdgeResult");
  
  value_object<EdgeResult>("EdgeResult")
    .field("x1", &EdgeResult::x1)
    .field("y1", &EdgeResult::y1)
    .field("x2", &EdgeResult::x2)
    .field("y2", &EdgeResult::y2)
    .field("isFinite", &EdgeResult::isFinite)
    .field("isCurved", &EdgeResult::isCurved)
    .field("isPrimary", &EdgeResult::isPrimary)
    .field("samples", &EdgeResult::samples)
    ;

  value_object<DiagrammResult>("DiagrammResult")
    .field("vertices", &DiagrammResult::vertices)
    .field("edges", &DiagrammResult::edges)
    .field("numVerticies", &DiagrammResult::numVerticies)
    ;

  emscripten::function("computevoronoi", &compute);


 
}
