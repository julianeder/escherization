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

void clip_infinite_edge(
  const voronoi_diagram<double>::edge_type& edge, 
  EdgeResult* clipped_edge,   
  std::vector<point_type> pointSites,
  std::vector<segment_type> lineSites,
  std::vector<double> bbox
  ) {

    const voronoi_diagram<double>::cell_type& cell1 = *edge.cell();
    const voronoi_diagram<double>::cell_type& cell2 = *edge.twin()->cell();

    point_type origin, direction;
    // Infinite edges could not be created by two segment sites.
    if (cell1.contains_point() && cell2.contains_point()) {
      point_type p1 = retrieve_point(cell1, pointSites, lineSites);
      point_type p2 = retrieve_point(cell2, pointSites, lineSites);
      origin.x((p1.x() + p2.x()) * 0.5);
      origin.y((p1.y() + p2.y()) * 0.5);
      direction.x(p1.y() - p2.y());
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
    coordinate_type side = bbox[2]; //width
    coordinate_type koef =
        side / (std::max)(fabs(direction.x()), fabs(direction.y()));
    if (edge.vertex0() == NULL) {
      clipped_edge->x1 = origin.x() - direction.x() * koef;
      clipped_edge->y1 = origin.y() - direction.y() * koef;
    } else {
      clipped_edge->x1 = edge.vertex0()->x();
      clipped_edge->y1 = edge.vertex0()->y();
    }
    if (edge.vertex1() == NULL) {
      clipped_edge->x2 = origin.x() + direction.x() * koef;
      clipped_edge->y2 = origin.y() + direction.y() * koef;
    } else {
      clipped_edge->x2 = edge.vertex1()->x();
      clipped_edge->y2 = edge.vertex1()->y();
    }
  }

EMSCRIPTEN_KEEPALIVE DiagrammResult compute(std::vector<int> points) {  
  std::vector<point_type> pointSites;
  std::vector<segment_type> lineSites;

  std::vector<double> bbox;
  bbox.push_back(0);
  bbox.push_back(0);
  bbox.push_back(500);
  bbox.push_back(500);

  for (size_t i = 0; i < points.size() * 2; i+=2)
  {
      pointSites.push_back(point_type(points[i], points[i+1]));
  }


  // Construction of the Voronoi Diagram.
  voronoi_diagram<double> vd;
  construct_voronoi(pointSites.begin(), pointSites.end(),
                    lineSites.begin(), lineSites.end(),
                    &vd);



  DiagrammResult result;
  result.numVerticies = vd.num_vertices();
  for (voronoi_diagram<double>::const_vertex_iterator it = vd.vertices().begin(); it != vd.vertices().end(); ++it) {
    const voronoi_diagram<double>::vertex_type& vertex = *it;
    result.vertices.push_back(vertex.x());
    result.vertices.push_back(vertex.y());
  }


  for (voronoi_diagram<double>::const_edge_iterator it = vd.edges().begin(); it != vd.edges().end(); ++it) {
    if (false && !it->is_primary()) {
      continue;
    }
      EdgeResult edgeResult;
      edgeResult.isFinite= it->is_finite();
      if(it->is_finite()){
        edgeResult.x1 = it->vertex0()->x();
        edgeResult.y1 = it->vertex0()->y();
        edgeResult.x2 = it->vertex1()->x();
        edgeResult.y2 = it->vertex1()->y();
      }else{
        clip_infinite_edge(*it, &edgeResult, pointSites, lineSites, bbox);
      }
      result.edges.push_back(edgeResult);
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
    ;

  value_object<DiagrammResult>("DiagrammResult")
    .field("vertices", &DiagrammResult::vertices)
    .field("edges", &DiagrammResult::edges)
    .field("numVerticies", &DiagrammResult::numVerticies)
    ;

  emscripten::function("computevoronoi", &compute);


 
}
