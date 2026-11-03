#include <cstddef>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../common/montecarlo.hpp"
#include "../common/shape.hpp"

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

    std::size_t parseSize(const char* const text)
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

    parameters_t parseParameters(int argc, const char* const* const argv)
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

  }
}

int main(int argc, char** argv)
{
  try {
    const petrov::parameters_t parameters = petrov::parseParameters(argc, argv);
    const std::vector< petrov::shape_t > shapes = petrov::readShapes(std::cin);
    const petrov::frame_t frame = petrov::findFrame(shapes);
    const petrov::hits_t hits
        = petrov::countHits(shapes, frame, parameters.try_count, parameters.thread_count, parameters.seed);

    const double covered_area = petrov::computeArea(frame, hits.union_count, parameters.try_count);
    const double intersection_area = petrov::computeArea(frame, hits.intersection_count, parameters.try_count);
    std::cout << covered_area << ' ' << intersection_area << '\n';
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return 1;
  }
  return 0;
}
