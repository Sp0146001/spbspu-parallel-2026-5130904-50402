#ifndef MONTECARLO_HPP
#define MONTECARLO_HPP

#include <cstddef>
#include <vector>

#include "shape.hpp"

namespace petrov {

  struct hits_t {
    std::size_t union_count = 0;
    std::size_t intersection_count = 0;
  };

  hits_t countHits(const std::vector< shape_t >& shapes, const frame_t& frame, std::size_t tries, std::size_t threads,
      std::size_t seed);
  double computeArea(const frame_t& frame, std::size_t hits, std::size_t tries);
  double coveredArea(const std::vector< shape_t >& shapes, std::size_t threads, std::size_t tries, std::size_t seed);

}

#endif
