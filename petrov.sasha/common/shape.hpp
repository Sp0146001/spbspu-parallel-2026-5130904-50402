#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <iosfwd>
#include <vector>

namespace petrov {

  struct point_t {
    double x = 0.0;
    double y = 0.0;
  };

  struct shape_t {
    double radius = 0.0;
    point_t center;
  };

  struct frame_t {
    point_t min_point;
    point_t max_point;
  };

  std::ostream& operator<<(std::ostream& out, const point_t& point);
  std::ostream& operator<<(std::ostream& out, const shape_t& shape);
  std::ostream& operator<<(std::ostream& out, const frame_t& frame);
  std::istream& operator>>(std::istream& in, point_t& point);
  std::istream& operator>>(std::istream& in, shape_t& shape);
  std::vector< shape_t > readShapes(std::istream& input);

  bool isInside(const point_t& point, const shape_t& shape);
  frame_t findFrame(const shape_t& shape);
  frame_t findFrame(const std::vector< shape_t >& shapes);
  double frameArea(const frame_t& frame);

}

#endif
