#include "shape.hpp"

#include <algorithm>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace petrov {

  std::ostream& operator<<(std::ostream& out, const point_t& point)
  {
    return out << '(' << point.x << ' ' << point.y << ')';
  }

  std::ostream& operator<<(std::ostream& out, const shape_t& shape)
  {
    return out << shape.radius << ' ' << shape.center;
  }

  std::ostream& operator<<(std::ostream& out, const frame_t& frame)
  {
    return out << frame.min_point << ' ' << frame.max_point;
  }

  std::istream& operator>>(std::istream& in, point_t& point)
  {
    const std::istream::sentry sentry(in);
    if (!sentry) {
      return in;
    }
    return in >> point.x >> point.y;
  }

  std::istream& operator>>(std::istream& in, shape_t& shape)
  {
    const std::istream::sentry sentry(in);
    if (!sentry) {
      return in;
    }
    double ignored_radius = 0.0;
    return in >> shape.radius >> ignored_radius >> shape.center;
  }

  std::vector< shape_t > readShapes(std::istream& input)
  {
    std::vector< shape_t > shapes;
    while (true) {
      input >> std::ws;
      if (input.eof()) {
        break;
      }
      shape_t shape = {};
      if (!(input >> shape)) {
        throw std::runtime_error("incorrect input data");
      }
      shapes.push_back(shape);
    }
    return shapes;
  }

  bool isInside(const point_t& point, const shape_t& shape)
  {
    const double delta_x = point.x - shape.center.x;
    const double delta_y = point.y - shape.center.y;
    return ((delta_x * delta_x) + (delta_y * delta_y)) <= (shape.radius * shape.radius);
  }

  frame_t findFrame(const shape_t& shape)
  {
    frame_t frame = {};
    frame.min_point.x = shape.center.x - shape.radius;
    frame.min_point.y = shape.center.y - shape.radius;
    frame.max_point.x = shape.center.x + shape.radius;
    frame.max_point.y = shape.center.y + shape.radius;
    return frame;
  }

  frame_t findFrame(const std::vector< shape_t >& shapes)
  {
    frame_t frame = {};
    if (shapes.empty()) {
      return frame;
    }
    frame = findFrame(shapes.front());
    for (const shape_t& shape : shapes) {
      const frame_t shape_frame = findFrame(shape);
      frame.min_point.x = std::min(frame.min_point.x, shape_frame.min_point.x);
      frame.min_point.y = std::min(frame.min_point.y, shape_frame.min_point.y);
      frame.max_point.x = std::max(frame.max_point.x, shape_frame.max_point.x);
      frame.max_point.y = std::max(frame.max_point.y, shape_frame.max_point.y);
    }
    return frame;
  }

  double frameArea(const frame_t& frame)
  {
    const double width = frame.max_point.x - frame.min_point.x;
    const double height = frame.max_point.y - frame.min_point.y;
    return width * height;
  }

}
