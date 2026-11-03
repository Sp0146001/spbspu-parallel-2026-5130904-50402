#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <cstddef>
#include <string>
#include <sys/types.h>
#include <vector>

#include "../common/shape.hpp"

namespace petrov {

  constexpr int invalid_descriptor = -1;

  struct request_t {
    int calculation_id = 0;
    int threads = 0;
    int tries = 0;
    int shape_count = 0;
  };

  struct shape_data_t {
    double radius = 0.0;
    double x = 0.0;
    double y = 0.0;
  };

  struct result_t {
    int calculation_id = 0;
    double milliseconds = 0.0;
    double area = 0.0;
  };

  struct process_t {
    std::string name;
    pid_t pid = 0;
    int request_descriptor = invalid_descriptor;
    int result_descriptor = invalid_descriptor;
  };

  process_t createProcess(const std::string& name, std::size_t seed, const std::vector< int >& inherited);
  bool sendCalculation(const process_t& process, const request_t& request, const std::vector< shape_t >& shapes);
  bool receiveResult(process_t& process, result_t& result);
  bool receiveResultBlocking(process_t& process, result_t& result);
  void closeProcess(process_t& process);

}

#endif
