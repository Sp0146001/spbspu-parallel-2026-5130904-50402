#include "montecarlo.hpp"

#include <algorithm>
#include <cstddef>
#include <istream>
#include <random>
#include <stdexcept>
#include <vector>

namespace petrov {

  std::istream& operator>>(std::istream& in, p_t& point)
  {
    const std::istream::sentry sentry(in);
    if (!sentry) {
      return in;
    }
    return in >> point.x >> point.y;
  }

  std::istream& operator>>(std::istream& in, circle_t& circle)
  {
    const std::istream::sentry sentry(in);
    if (!sentry) {
      return in;
    }
    double ignored_parameter = 0.0;
    return in >> circle.radius >> ignored_parameter >> circle.center;
  }

  std::vector< circle_t > readCircles(std::istream& in)
  {
    std::vector< circle_t > circles;
    while (true) {
      in >> std::ws;
      if (in.eof()) {
        break;
      }
      circle_t circle = {};
      if (!(in >> circle)) {
        throw std::runtime_error("incorrect input data");
      }
      circles.push_back(circle);
    }
    return circles;
  }

  bool isInside(const p_t& point, const circle_t& circle)
  {
    const double del_x = point.x - circle.center.x;
    const double del_y = point.y - circle.center.y;
    return ((del_x * del_x) + (del_y * del_y)) <= (circle.radius * circle.radius);
  }

  bool isInsideUnion(const p_t& point, const std::vector< circle_t >& circles)
  {
    for (const circle_t& circle : circles) {
      if (isInside(point, circle)) {
        return true;
      }
    }
    return false;
  }

  bool isInsideIntersection(const p_t& point, const std::vector< circle_t >& circles)
  {
    for (const circle_t& circle : circles) {
      if (!isInside(point, circle)) {
        return false;
      }
    }
    return true;
  }

  box_t findBoundingBox(const std::vector< circle_t >& circles)
  {
    box_t box = {{0.0, 0.0}, {0.0, 0.0}};
    if (circles.empty()) {
      return box;
    }
    const circle_t& first_circle = circles.front();
    box.min_point.x = first_circle.center.x - first_circle.radius;
    box.min_point.y = first_circle.center.y - first_circle.radius;
    box.max_point.x = first_circle.center.x + first_circle.radius;
    box.max_point.y = first_circle.center.y + first_circle.radius;
    for (const circle_t& circle : circles) {
      box.min_point.x = std::min(box.min_point.x, circle.center.x - circle.radius);
      box.min_point.y = std::min(box.min_point.y, circle.center.y - circle.radius);
      box.max_point.x = std::max(box.max_point.x, circle.center.x + circle.radius);
      box.max_point.y = std::max(box.max_point.y, circle.center.y + circle.radius);
    }
    return box;
  }

  double computeBoxArea(const box_t& box)
  {
    const double width = box.max_point.x - box.min_point.x;
    const double height = box.max_point.y - box.min_point.y;
    return width * height;
  }

  hits_t countHits(const std::vector< circle_t >& circles, const box_t& box, std::size_t tries, std::size_t seed)
  {
    std::default_random_engine generator(seed);
    std::uniform_real_distribution< double > x_distribution(box.min_point.x, box.max_point.x);
    std::uniform_real_distribution< double > y_distribution(box.min_point.y, box.max_point.y);
    hits_t hits = {0, 0};
    for (std::size_t attempt = 0; attempt < tries; ++attempt) {
      const double x = x_distribution(generator);
      const double y = y_distribution(generator);
      const p_t point = {x, y};
      if (isInsideUnion(point, circles)) {
        ++hits.union_count;
      }
      if (isInsideIntersection(point, circles)) {
        ++hits.intersection_count;
      }
    }
    return hits;
  }

  double computeArea(const box_t& box, std::size_t hits, std::size_t tries)
  {
    const double hit_share = static_cast< double >(hits) / static_cast< double >(tries);
    return computeBoxArea(box) * hit_share;
  }

}
