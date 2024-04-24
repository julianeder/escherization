// Boost.Polygon library voronoi_basic_tutorial.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_boost.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.
// Modified by Julian Eder

#include <emscripten/bind.h>
using namespace emscripten;


#include <cstdio>
#include <vector>
#include <algorithm>


#include <boost/polygon/voronoi.hpp>
#include <boost/polygon/polygon.hpp>

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;
using boost::polygon::x;
using boost::polygon::y;
using boost::polygon::low;
using boost::polygon::high;
using namespace boost::polygon;

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


struct Point {
  int a;
  int b;
  Point(int x, int y) : a(x), b(y) {}
  const int x(){ return a; }
  const int y(){ return b; }

  inline bool operator==(point_type& rhs) {
    return x() == ((int)round(rhs.x())) && y() == ((int)round(rhs.y()));
  }

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
  
struct CellResult {
  size_t source_index;
  int source_category;
  bool is_degenerate;
  bool contains_point;
  bool contains_segment;
  std::vector<int> edge_indices;
  int color;
  int tile_idx;
};

struct EdgeResult {
  double x1;
  double y1;
  double x2;
  double y2;
  bool isFinite;
  bool isCurved;
  bool isPrimary;
  bool isWithinCell;
  const voronoi_diagram<double>::edge_type* edge_ref; // not for javascript
  std::vector<double> controll_points;
};

struct DiagrammResult {
  std::vector<double> vertices;
  std::vector<EdgeResult> edges;
  std::vector<CellResult> cells;
  int numVerticies;
};

Point retrieve_point(
  const voronoi_diagram<double>::cell_type* cell,
  std::vector<Point> pointSites,
  std::vector<Segment> lineSites
  ) {
  voronoi_diagram<double>::cell_type::source_index_type index = cell->source_index();
  voronoi_diagram<double>::cell_type::source_category_type category = cell->source_category();
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

Segment retrieve_segment(const cell_type* cell, std::vector<Point> pointSites, std::vector<Segment> lineSites) {
  source_index_type index = cell->source_index() - pointSites.size();
  return lineSites[index];
}

double get_point_projection(
      const point_type& point, const Segment& segment, bool verbose = false) {

    double segment_vec_x = x(high(segment)) - x(low(segment));
    double segment_vec_y = y(high(segment)) - y(low(segment));
    double point_vec_x = x(point) - x(low(segment));
    double point_vec_y = y(point) - y(low(segment));

    double sqr_segment_length =
        segment_vec_x * segment_vec_x + segment_vec_y * segment_vec_y;
    double vec_dot = segment_vec_x * point_vec_x + segment_vec_y * point_vec_y;
    return vec_dot / sqr_segment_length;
  }

// Compute y(x) = ((x - a) * (x - a) + b * b) / (2 * b)
static double parabola_y(double x, double a, double b) {
  return ((x - a) * (x - a) + b * b) / (b + b);
}

// Compute dy(x)/dx = ((x - a)² + b²) / (2b)' = (x-a) / b
static double parabola_deriv_y(double x, double a, double b) {
  return (x - a) / b;
}

void calc_control_points(
  Point& point,
  Segment& segment,
  std::vector<point_type>* control_points)
{
    // Save the first and last point.
    point_type c0 = (*control_points)[0];
    point_type c1;
    point_type c2 = (*control_points)[1];
    control_points->pop_back();
    control_points->pop_back();

    // Apply the linear transformation to move start point of the segment to
    // the point with coordinates (0, 0) and the direction of the segment to
    // coincide the positive direction of the x-axis.
    double segm_vec_x = high(segment).x() - low(segment).x();
    double segm_vec_y = high(segment).y() - low(segment).y();
    double sqr_segment_length = segm_vec_x * segm_vec_x + segm_vec_y * segm_vec_y;

    // Compute x-coordinates of the endpoints of the edge
    // in the transformed space.
    double c0_x_proj = sqr_segment_length * 
        get_point_projection(c0, segment);
    double c2_x_proj = sqr_segment_length * 
        get_point_projection(c2, segment);


    // Compute parabola parameters in the transformed space.
    // Parabola has next representation:
    // f(x) = ((x-rot_x)^2 + rot_y^2) / (2.0*rot_y).
    double point_vec_x = point.x() - low(segment).x();
    double point_vec_y = point.y() - low(segment).y();

    double rot_x = segm_vec_x * point_vec_x + segm_vec_y * point_vec_y;
    double rot_y = segm_vec_x * point_vec_y - segm_vec_y * point_vec_x;

    double y_0_proj = parabola_y(c0_x_proj, rot_x, rot_y); 
    double y_2_proj = parabola_y(c2_x_proj, rot_x, rot_y); 
    
    // special case where the line is vertical and therefore the derivative is infinity
    // in this case the parabolar degenerates to a line and we can set the middle controllpoint (c1) to the midpoint of c0 and c2
    if(c0_x_proj == rot_x){ 
      double u1 = (x(c0) + x(c2)) / 2.0;
      double v1 = (y(c0) + y(c2)) / 2.0;

      c1.x(u1);
      c1.y(v1);

      control_points->push_back(c0);
      control_points->push_back(c1);
      control_points->push_back(c2);
      return;
    }
    
    double dy_0_proj = parabola_deriv_y(c0_x_proj, rot_x, rot_y);
    double dy_2_proj = parabola_deriv_y(c2_x_proj, rot_x, rot_y);
    
   
    double v1_proj = (dy_2_proj * c2_x_proj + (dy_2_proj/dy_0_proj) * (y_0_proj) - dy_2_proj * c0_x_proj - y_2_proj) / ((dy_2_proj/dy_0_proj) - 1);
    double u1_proj = (((v1_proj-y_0_proj) + dy_0_proj*c0_x_proj) / dy_0_proj);

    // Project Back
    double u1 = (segm_vec_x * u1_proj - segm_vec_y * v1_proj) /
        sqr_segment_length + x(low(segment));
    double v1 = (segm_vec_x * v1_proj + segm_vec_y * u1_proj) /
        sqr_segment_length + y(low(segment));

    c1.x(u1);
    c1.y(v1);

    control_points->push_back(c0);
    control_points->push_back(c1);
    control_points->push_back(c2);

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


bool clip_add_finite_edge(
  const voronoi_diagram<double>::edge_type& edge,
  DiagrammResult* result,
  EdgeResult* edgeResult,
  std::vector<Point> pointSites,
  std::vector<Segment> lineSites,
  std::vector<double> bbox,
  int i
  ) {

  std::vector<point_type> controll_points_;
  double xl = bbox[0];
  double xh = bbox[2];
  double yl = bbox[1];
  double yh = bbox[3];

  // completely outside
  if((edge.vertex0()->x() <= xl && edge.vertex1()->x() <= xl)
  || (edge.vertex0()->x() >= xh && edge.vertex1()->x() >= xh)
  || (edge.vertex0()->y() <= yl && edge.vertex1()->y() <= yl)
  || (edge.vertex0()->y() >= yh && edge.vertex1()->y() >= yh)){
    return false;
  }


  point_type direction;
  direction.x(edge.vertex1()->x() - edge.vertex0()->x());
  direction.y(edge.vertex1()->y() - edge.vertex0()->y());

  if(isnan(direction.x()) || isnan(direction.y()))
    return false;

  double dxdy = direction.x() / direction.y();
  double dydx = direction.y() / direction.x();

  // Clip X1
  if(edge.vertex0()->x() < xl){
    edgeResult->x1 = edge.vertex0()->x() + (xl - edge.vertex0()->x()) ;
    edgeResult->y1 = edge.vertex0()->y() + (xl - edge.vertex0()->x()) * dydx;
  }
  else if(edge.vertex0()->x() > xh){
    edgeResult->x1 = edge.vertex0()->x() + (xh - edge.vertex0()->x());
    edgeResult->y1 = edge.vertex0()->y() + (xh - edge.vertex0()->x()) * dydx;
  }
  else {
    edgeResult->x1 = edge.vertex0()->x();
    edgeResult->y1 = edge.vertex0()->y();
  }

  // Clip X2
  if(edge.vertex1()->x() < xl){
    edgeResult->x2 = edge.vertex1()->x() + (xl - edge.vertex1()->x());
    edgeResult->y2 = edge.vertex1()->y() + (xl - edge.vertex1()->x()) * dydx;
  }
  else if(edge.vertex1()->x() > xh){
    edgeResult->x2 = edge.vertex1()->x() + (xh - edge.vertex1()->x());
    edgeResult->y2 = edge.vertex1()->y() + (xh - edge.vertex1()->x()) * dydx;
  }
  else {
    edgeResult->x2 = edge.vertex1()->x();
    edgeResult->y2 = edge.vertex1()->y();
  }

  // Clip Y1
  if(edgeResult->y1 < yl){
    edgeResult->x1 = edgeResult->x1 + (yl - edgeResult->y1) * dxdy;
    edgeResult->y1 = edgeResult->y1 + (yl - edgeResult->y1);
  }
  else if(edgeResult->y1 > yh){
    edgeResult->x1 = edgeResult->x1 + (yh - edgeResult->y1) * dxdy;
    edgeResult->y1 = edgeResult->y1 + (yh - edgeResult->y1);
  }

  // Clip Y2
  if(edgeResult->y2 < yl){
    edgeResult->x2 = edgeResult->x2 + (yl - edgeResult->y2) * dxdy;
    edgeResult->y2 = edgeResult->y2 + (yl - edgeResult->y2);

  }
  else if(edgeResult->y2 > yh){
    edgeResult->x2 = edgeResult->x2 + (yh - edgeResult->y2) * dxdy;
    edgeResult->y2 = edgeResult->y2 + (yh - edgeResult->y2);
  }


  if (edge.is_curved()) { // only finite edges can be curved
    controll_points_.push_back(point_type(edgeResult->x1, edgeResult->y1));
    controll_points_.push_back(point_type(edgeResult->x2, edgeResult->y2));
    Point point = edge.cell()->contains_point() ?
      retrieve_point(edge.cell(), pointSites, lineSites) :
      retrieve_point(edge.twin()->cell(), pointSites, lineSites);
    Segment segment = edge.cell()->contains_point() ?
      retrieve_segment(edge.twin()->cell(), pointSites, lineSites) :
      retrieve_segment(edge.cell(), pointSites, lineSites);
    calc_control_points(point, segment, &controll_points_);
  }

  for (size_t i = 0; i < controll_points_.size(); i++)
  {
    edgeResult->controll_points.push_back(controll_points_[i].x());
    edgeResult->controll_points.push_back(controll_points_[i].y());
  }
  result->edges.push_back(*edgeResult);
  return true;
}

bool clip_add_infinite_edge(
  const voronoi_diagram<double>::edge_type& edge,
  DiagrammResult* result,
  EdgeResult* edgeResult,
  std::vector<Point> pointSites,
  std::vector<Segment> lineSites,
  std::vector<double> bbox,
  int i
  ) {
    // vertex - voronoi vertex from which the voronoi edge starts
    // p1, p2 - sitePoints that the edge is equal distance from

    double xl = bbox[0];
    double xh = bbox[2];
    double yl = bbox[1];
    double yh = bbox[3];

    bool vertex0isZero = false;

    // completely outside (we assume that the edge goes outwards from the bbox and therefore is not visible if the finite start point is inside the bbox)
    if((edge.vertex0() != NULL) && (edge.vertex1() == NULL)){
      if((edge.vertex0()->x() <= xl)
      || (edge.vertex0()->x() >= xh)
      || (edge.vertex0()->y() <= yl)
      || (edge.vertex0()->y() >= yh)){
        return false;
      }
    }
    if((edge.vertex0() == NULL) && (edge.vertex1() != NULL)){
      vertex0isZero = true;
      if((edge.vertex1()->x() <= xl)
      || (edge.vertex1()->x() >= xh)
      || (edge.vertex1()->y() <= yl)
      || (edge.vertex1()->y() >= yh)){
        return false;
      }
    }

    const voronoi_diagram<double>::cell_type* cell1 = edge.cell();
    const voronoi_diagram<double>::cell_type* cell2 = edge.twin()->cell();
    if(vertex0isZero){
      cell1 = edge.twin()->cell();
      cell2 = edge.cell();
    }

    point_type origin, direction;
    // Infinite edges could not be created by two segment sites.
    if (cell1->contains_point() && cell2->contains_point()) {
      Point p1 = retrieve_point(cell1, pointSites, lineSites);
      Point p2 = retrieve_point(cell2, pointSites, lineSites);
      origin.x((p1.x() + p2.x()) * 0.5);
      origin.y((p1.y() + p2.y()) * 0.5);
      direction.x(p1.y() - p2.y()); // orthogonal to the direction between the points
      direction.y(p2.x() - p1.x());
    } else {
      origin = cell1->contains_segment() ?
          retrieve_point(cell2, pointSites, lineSites) :
          retrieve_point(cell1, pointSites, lineSites);
      Segment segment = cell1->contains_segment() ?
          retrieve_segment(cell1, pointSites, lineSites) :
          retrieve_segment(cell2, pointSites, lineSites);
      coordinate_type dx = high(segment).x() - low(segment).x();
      coordinate_type dy = high(segment).y() - low(segment).y();
      if ((low(segment) == origin) ^ cell1->contains_point()) {
        direction.x(dy);
        direction.y(-dx);
      } else {
        direction.x(-dy);
        direction.y(dx);
      }
    }

    if(isnan(direction.x()) || isnan(direction.y())){
      return false;
    }

    //normalize
    double l = sqrt(direction.x()*direction.x() + direction.y()*direction.y());
    direction.x(direction.x() / l);
    direction.y(direction.y() / l);

    double fm, fb;

    if(direction.x() == 0){ // Vertical
      // doesn't intersect with viewport
      if (origin.x() < xl || origin.x() > xh) {
        return false;
      }
      // downward
      if (direction.y() < 0) {
        createVertex(edge, edgeResult, origin.x(), yl);
      }
      // upward
      else {
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


      // the one in the same dir as the ray is >0
      if(tmin > 0){
        double cx = origin.x() + tmin * direction.x();
        double cy = origin.y() + tmin * direction.y();
        createVertex(edge, edgeResult, cx, cy);
      }
      else if(tmax > 0){
        double cx = origin.x() + tmax * direction.x();
        double cy = origin.y() + tmax * direction.y();
        createVertex(edge, edgeResult, cx, cy);
      }
    }
    result->edges.push_back(*edgeResult);
    return true;
}

EMSCRIPTEN_KEEPALIVE DiagrammResult compute(
  std::vector<double> bbox, std::vector<int> points, 
  std::vector<int> segments, 
  std::vector<int> pointColors, 
  std::vector<int> segmentColors,
  std::vector<int> pointTileIdxs,
  std::vector<int> segmentTileIdxs
  ) {
  std::vector<Point> pointSites;
  std::vector<Segment> lineSites;

  for (size_t i = 0; i < points.size(); i += 2)
  {
      pointSites.push_back(Point(points[i], points[i+1]));
  }

  for (size_t i = 0; i < segments.size(); i += 4)
  {
      lineSites.push_back(Segment(segments[i], segments[i+1], segments[i+2], segments[i+3]));
  }


  // Construction of the Voronoi Diagram.
  voronoi_diagram<double> vd;
  construct_voronoi(pointSites.begin(), pointSites.end(),
                    lineSites.begin(), lineSites.end(),
                    &vd);

  DiagrammResult result;

  // --------- CELLS --------------
  // we need to do this part before edges were iterated
  for (voronoi_diagram<double>::const_cell_iterator it = vd.cells().begin(); it != vd.cells().end(); ++it) {
    const voronoi_diagram<double>::cell_type& cell = *it;

    CellResult cellResult;

    cellResult.source_index = cell.source_index();
    switch(cell.source_category()){
        case boost::polygon::SOURCE_CATEGORY_SINGLE_POINT:
          cellResult.source_category = 0;
          break;
        case boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT:
          cellResult.source_category = 1;
          break;
        case boost::polygon::SOURCE_CATEGORY_SEGMENT_END_POINT:
          cellResult.source_category = 2;
          break;

        // Segment subtypes.
        case boost::polygon::SOURCE_CATEGORY_INITIAL_SEGMENT:
          cellResult.source_category = 3;
          break;
        case boost::polygon::SOURCE_CATEGORY_REVERSE_SEGMENT:
          cellResult.source_category = 4;
          break;
        case boost::polygon::SOURCE_CATEGORY_GEOMETRY_SHIFT:
          cellResult.source_category = 5;
          break;
        case boost::polygon::SOURCE_CATEGORY_BITMASK:
          cellResult.source_category = 6;
          break;
    }

    if(cellResult.source_category == 0){
      cell.color(pointColors[cell.source_index()]);
      cellResult.tile_idx = pointTileIdxs[cell.source_index()];
    }else{
      cell.color(segmentColors[cell.source_index()]);
      cellResult.tile_idx = segmentTileIdxs[cell.source_index()];
    }

    cellResult.is_degenerate = cell.is_degenerate();
    cellResult.contains_point = cell.contains_point();
    cellResult.contains_segment = cell.contains_segment();
    cellResult.color = cell.color();
    
    

    result.cells.push_back(cellResult);
  }

  // --------- EDGES --------------
  std::vector<const voronoi_diagram<double>::edge_type*> processed; // to Avoid Duplicates 
  int i = 0;
  for (voronoi_diagram<double>::const_edge_iterator it = vd.edges().begin(); it != vd.edges().end(); ++it) {
    const voronoi_diagram<double>::edge_type* edge = &(*it);
    
    // Avoid Duplicates if we have rendered the twin half-edge (which is the same in the other direction) already
    // bool found = false;
    // for (std::vector<const voronoi_diagram<double>::edge_type*>::iterator it2 = processed.begin(); it2 != processed.end(); ++it2) {
    //   if((*it2) == edge->twin())
    //     found = true;
    // }
    // if(found) {
    //   continue;
    // }

    // we only add the edge to processed at the end if it was not cliped (because cliping removes some halfEdges that are not cliped from the other side)

    EdgeResult edgeResult;
    
    edgeResult.edge_ref = edge;

    edgeResult.isFinite = edge->is_finite();
    edgeResult.isPrimary = edge->is_primary();
    edgeResult.isCurved = edge->is_curved();
    edgeResult.isWithinCell = segmentTileIdxs[edge->cell()->source_index()] == segmentTileIdxs[edge->twin()->cell()->source_index()];
  
    bool added = false;

    if(edge->is_finite()){
      added = clip_add_finite_edge(*edge, &result, &edgeResult, pointSites, lineSites, bbox, i);
    }else{
      added = clip_add_infinite_edge(*edge, &result, &edgeResult, pointSites, lineSites, bbox, i);
    }

    if(added){
      processed.push_back(edge);
    }
    i++;
  }

  // --------- VERTICIES --------------
  result.numVerticies = vd.num_vertices();
  i = 0;
  for (voronoi_diagram<double>::const_vertex_iterator it = vd.vertices().begin(); it != vd.vertices().end(); ++it) {
    const voronoi_diagram<double>::vertex_type& vertex = *it;
    result.vertices.push_back(vertex.x());
    result.vertices.push_back(vertex.y());
  }


  // -------- CELLS 2 ---------------
  // we need to do this part after edges were iterated
  for (int j = 0; j < vd.cells().size(); ++j) {
    const voronoi_diagram<double>::cell_type& cell = vd.cells()[j];

    const voronoi_diagram<double>::edge_type* edge = cell.incident_edge();
      do {
        for (int i = 0; i < result.edges.size(); i++){
          if(result.edges[i].edge_ref == edge){
            result.cells[j].edge_indices.push_back(i);
          }
        }
        edge = edge->next();
      } while (edge != cell.incident_edge());
  }

  return result;
}


// // Binding code
EMSCRIPTEN_BINDINGS(myvoronoi) {
  register_vector<int>("VectorInt");
  register_vector<double>("VectorDouble");
  register_vector<EdgeResult>("VectorEdgeResult");
  register_vector<CellResult>("VectorCellResult");
  
  value_object<CellResult>("CellResult")
    .field("sourceIndex", &CellResult::source_index)
    .field("sourceCategory", &CellResult::source_category)
    .field("isDegenerate", &CellResult::is_degenerate)
    .field("containsPoint", &CellResult::contains_point)
    .field("containsSegment", &CellResult::contains_segment)
    .field("edgeIndices", &CellResult::edge_indices)
    .field("color", &CellResult::color)
    .field("tileIdx", &CellResult::tile_idx)
    ;

  value_object<EdgeResult>("EdgeResult")
    .field("x1", &EdgeResult::x1)
    .field("y1", &EdgeResult::y1)
    .field("x2", &EdgeResult::x2)
    .field("y2", &EdgeResult::y2)
    .field("isFinite", &EdgeResult::isFinite)
    .field("isCurved", &EdgeResult::isCurved)
    .field("isPrimary", &EdgeResult::isPrimary)
    .field("controll_points", &EdgeResult::controll_points)
    .field("isWithinCell", &EdgeResult::isWithinCell)
    ;

  value_object<DiagrammResult>("DiagrammResult")
    .field("vertices", &DiagrammResult::vertices)
    .field("edges", &DiagrammResult::edges)
    .field("cells", &DiagrammResult::cells)
    .field("numVerticies", &DiagrammResult::numVerticies)
    ;

  emscripten::function("computevoronoi", &compute);

}
