#include "montecarlo.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace petrov {
  namespace {

    constexpr int minimal_argument_count = 3;
    constexpr int maximal_argument_count = 4;
    constexpr int threads_argument_index = 1;
    constexpr int tries_argument_index = 2;
    constexpr int seed_argument_index = 3;
    constexpr std::size_t default_seed = 0;

    struct parameters_t {
      std::size_t thread_count;
      std::size_t try_count;
      std::size_t seed;
    };

    std::size_t parseSize(const char* text)
    {
      std::size_t parsed_length = 0;
      long long value = 0;
      try {
        value = std::stoll(text, &parsed_length);
      } catch (const std::exception& exception) {
        throw std::runtime_error(std::string("incorrect value of parameter"));
      }
      if ((parsed_length != std::strlen(text)) || (value < 0)) {
        throw std::runtime_error(std::string("incorrect value of parameter"));
      }
      return static_cast< std::size_t >(value);
    }

    parameters_t parseParameters(int argc, const char* const* argv)
    {
      if ((argc != minimal_argument_count) && (argc != maximal_argument_count)) {
        throw std::runtime_error("incorrect number of arguments");
      }
      const std::size_t thread_count = parseSize(argv[threads_argument_index]);
      const std::size_t try_count = parseSize(argv[tries_argument_index]);
      if (try_count == 0) {
        throw std::runtime_error("the number of tries must be >0");
      }
      const std::size_t seed = (argc == maximal_argument_count) ? parseSize(argv[seed_argument_index]) : default_seed;
      return {thread_count, try_count, seed};
    }

    void runMonteCarlo(
        const std::vector< circle_t >& circles, const box_t& box, std::size_t tries, std::size_t seed, hits_t& result)
    {
      result = countHits(circles, box, tries, seed);
    }

    hits_t countHitsInThreads(const std::vector< circle_t >& circles, const box_t& box, const parameters_t& parameters)
    {
      const std::size_t requested_count = (parameters.thread_count == 0) ? 1 : parameters.thread_count;
      const std::size_t hardware_count = static_cast< std::size_t >(std::thread::hardware_concurrency());
      const std::size_t hardware_limit = (hardware_count == 0) ? 1 : hardware_count;
      const std::size_t worker_count = std::min(requested_count, hardware_limit);

      const std::size_t tries_per_worker = parameters.try_count / worker_count;
      const std::size_t remainder = parameters.try_count % worker_count;

      std::vector< hits_t > results(worker_count);
      std::vector< std::thread > workers;
      workers.reserve(worker_count);

      for (std::size_t worker_index = 0; worker_index < worker_count; ++worker_index) {
        const std::size_t worker_tries = tries_per_worker + ((worker_index < remainder) ? 1 : 0);
        const std::size_t worker_seed = parameters.seed + worker_index;
        workers.emplace_back(runMonteCarlo, std::cref(circles), std::cref(box), worker_tries, worker_seed,
            std::ref(results[worker_index]));
      }

      for (std::thread& worker : workers) {
        worker.join();
      }

      hits_t total_hits = {0, 0};
      for (const hits_t& hits : results) {
        total_hits.union_count += hits.union_count;
        total_hits.intersection_count += hits.intersection_count;
      }
      return total_hits;
    }

  }
}

int main(int argc, char** argv)
{
  try {
    const petrov::parameters_t parameters = petrov::parseParameters(argc, argv);
    const std::vector< petrov::circle_t > circles = petrov::readCircles(std::cin);
    const petrov::box_t box = petrov::findBoundingBox(circles);

    const petrov::hits_t hits = petrov::countHitsInThreads(circles, box, parameters);

    const double covered_area = petrov::computeArea(box, hits.union_count, parameters.try_count);
    const double intersection_area = petrov::computeArea(box, hits.intersection_count, parameters.try_count);
    std::cout << covered_area << ' ' << intersection_area << '\n';
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return 1;
  }
  return 0;
}
