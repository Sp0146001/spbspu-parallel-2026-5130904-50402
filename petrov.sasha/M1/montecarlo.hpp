#ifndef MONTECARLO_HPP
#define MONTECARLO_HPP

#include <cstddef>
#include <iosfwd>
#include <vector>

namespace petrov {

  struct p_t {
    double x;
    double y;
  };

  struct circle_t {
    double radius;
    p_t center;
  };

  struct box_t {
    p_t min_point;
    p_t max_point;
  };

  struct hits_t {
    std::size_t union_count;
    std::size_t intersection_count;
  };

  std::istream& operator>>(std::istream& in, p_t& point);
  std::istream& operator>>(std::istream& in, circle_t& circle);
  std::vector< circle_t > readCircles(std::istream& in);
  bool isInside(const p_t& point, const circle_t& circle);
  bool isInsideUnion(const p_t& point, const std::vector< circle_t >& circles);
  bool isInsideIntersection(const p_t& point, const std::vector< circle_t >& circles);
  box_t findBoundingBox(const std::vector< circle_t >& circles);
  double computeBoxArea(const box_t& box);
  hits_t countHits(const std::vector< circle_t >& circles, const box_t& box, std::size_t tries, std::size_t seed);
  double computeArea(const box_t& box, std::size_t hits, std::size_t tries);

}

#endif
