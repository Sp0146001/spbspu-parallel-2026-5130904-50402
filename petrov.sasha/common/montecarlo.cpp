#include "montecarlo.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <random>
#include <thread>
#include <vector>

namespace petrov {

  namespace {

    std::size_t findWorkerCount(std::size_t threads)
    {
      const std::size_t requested_count = (threads == 0) ? 1 : threads;
      const std::size_t hardware_count = static_cast< std::size_t >(std::thread::hardware_concurrency());
      const std::size_t hardware_limit = (hardware_count == 0) ? 1 : hardware_count;
      return std::min(requested_count, hardware_limit);
    }

    std::size_t findWorkerTries(std::size_t tries, std::size_t worker_count, std::size_t worker_index)
    {
      const std::size_t tries_per_worker = tries / worker_count;
      const std::size_t remainder = tries % worker_count;
      return tries_per_worker + ((worker_index < remainder) ? 1 : 0);
    }

    bool isInsideUnion(const point_t& point, const std::vector< shape_t >& shapes)
    {
      for (const shape_t& shape : shapes) {
        if (isInside(point, shape)) {
          return true;
        }
      }
      return false;
    }

    bool isInsideIntersection(const point_t& point, const std::vector< shape_t >& shapes)
    {
      for (const shape_t& shape : shapes) {
        if (!isInside(point, shape)) {
          return false;
        }
      }
      return true;
    }

    void countWorkerHits(
        const std::vector< shape_t >& shapes, const frame_t& frame, std::size_t tries, std::size_t seed, hits_t& result)
    {
      std::default_random_engine generator(seed);
      std::uniform_real_distribution< double > x_distribution(frame.min_point.x, frame.max_point.x);
      std::uniform_real_distribution< double > y_distribution(frame.min_point.y, frame.max_point.y);
      hits_t hits = {};
      for (std::size_t attempt = 0; attempt < tries; ++attempt) {
        const double x = x_distribution(generator);
        const double y = y_distribution(generator);
        const point_t point = {x, y};
        if (isInsideUnion(point, shapes)) {
          ++hits.union_count;
        }
        if (isInsideIntersection(point, shapes)) {
          ++hits.intersection_count;
        }
      }
      result = hits;
    }

    void countWorkerCover(const std::vector< shape_t >& shapes, const frame_t& frame, std::size_t tries,
        std::size_t seed, std::size_t& result)
    {
      std::default_random_engine generator(seed);
      const double generator_begin = static_cast< double >(generator.min());
      const double generator_width = static_cast< double >(generator.max()) - generator_begin;
      const double step_x = (frame.max_point.x - frame.min_point.x) / generator_width;
      const double step_y = (frame.max_point.y - frame.min_point.y) / generator_width;
      std::size_t hits = 0;
      for (std::size_t attempt = 0; attempt < tries; ++attempt) {
        const double x = frame.min_point.x + ((static_cast< double >(generator()) - generator_begin) * step_x);
        const double y = frame.min_point.y + ((static_cast< double >(generator()) - generator_begin) * step_y);
        const point_t point = {x, y};
        if (isInsideUnion(point, shapes)) {
          ++hits;
        }
      }
      result = hits;
    }

  }

  hits_t countHits(const std::vector< shape_t >& shapes, const frame_t& frame, std::size_t tries, std::size_t threads,
      std::size_t seed)
  {
    const std::size_t worker_count = findWorkerCount(threads);
    std::vector< hits_t > results(worker_count);
    std::vector< std::thread > workers;
    workers.reserve(worker_count);
    for (std::size_t worker_index = 0; worker_index < worker_count; ++worker_index) {
      workers.emplace_back(countWorkerHits, std::cref(shapes), std::cref(frame),
          findWorkerTries(tries, worker_count, worker_index), seed + worker_index, std::ref(results[worker_index]));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }

    hits_t total = {};
    for (const hits_t& hits : results) {
      total.union_count += hits.union_count;
      total.intersection_count += hits.intersection_count;
    }
    return total;
  }

  double computeArea(const frame_t& frame, std::size_t hits, std::size_t tries)
  {
    const double hit_share = static_cast< double >(hits) / static_cast< double >(tries);
    return frameArea(frame) * hit_share;
  }

  double coveredArea(const std::vector< shape_t >& shapes, std::size_t threads, std::size_t tries, std::size_t seed)
  {
    if (shapes.empty() || (tries == 0)) {
      return 0.0;
    }
    const frame_t frame = findFrame(shapes);
    const std::size_t worker_count = findWorkerCount(threads);
    std::vector< std::size_t > results(worker_count, 0);
    std::vector< std::thread > workers;
    workers.reserve(worker_count);
    for (std::size_t worker_index = 0; worker_index < worker_count; ++worker_index) {
      workers.emplace_back(countWorkerCover, std::cref(shapes), std::cref(frame),
          findWorkerTries(tries, worker_count, worker_index), seed + worker_index, std::ref(results[worker_index]));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }

    std::size_t total = 0;
    for (const std::size_t hits : results) {
      total += hits;
    }
    return computeArea(frame, total, tries);
  }

}
