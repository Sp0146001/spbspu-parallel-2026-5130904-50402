#include "process.hpp"

#include <chrono>
#include <cstddef>
#include <fcntl.h>
#include <signal.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "../common/montecarlo.hpp"

namespace petrov {

  namespace {

    constexpr int pipe_end_count = 2;

    bool sendAll(int descriptor, const void* const data, std::size_t size)
    {
      const char* const buffer = static_cast< const char* >(data);
      std::size_t sent = 0;
      while (sent < size) {
        const ssize_t written = write(descriptor, buffer + sent, size - sent);
        if (written <= 0) {
          return false;
        }
        sent += static_cast< std::size_t >(written);
      }
      return true;
    }

    bool receiveAll(int descriptor, void* const data, std::size_t size)
    {
      char* const buffer = static_cast< char* >(data);
      std::size_t received = 0;
      while (received < size) {
        const ssize_t count = read(descriptor, buffer + received, size - received);
        if (count <= 0) {
          return false;
        }
        received += static_cast< std::size_t >(count);
      }
      return true;
    }

    void setDescriptorMode(int descriptor, bool is_non_blocking)
    {
      const int flags = fcntl(descriptor, F_GETFL, 0);
      if (flags == invalid_descriptor) {
        return;
      }
      const int mode = is_non_blocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
      static_cast< void >(fcntl(descriptor, F_SETFL, mode));
    }

    void closeDescriptor(int& descriptor)
    {
      if (descriptor != invalid_descriptor) {
        static_cast< void >(close(descriptor));
        descriptor = invalid_descriptor;
      }
    }

    void runCalculator(int request_descriptor, int result_descriptor, std::size_t seed)
    {
      while (true) {
        request_t request = {};
        if (!receiveAll(request_descriptor, &request, sizeof(request_t))) {
          break;
        }
        if (request.shape_count < 0) {
          break;
        }
        const std::size_t shape_count = static_cast< std::size_t >(request.shape_count);
        std::vector< shape_data_t > data(shape_count);
        if (!receiveAll(request_descriptor, data.data(), shape_count * sizeof(shape_data_t))) {
          break;
        }
        std::vector< shape_t > shapes(shape_count);
        for (std::size_t index = 0; index < shape_count; ++index) {
          shapes[index].radius = data[index].radius;
          shapes[index].center.x = data[index].x;
          shapes[index].center.y = data[index].y;
        }
        const std::size_t calculation_seed = seed + static_cast< std::size_t >(request.calculation_id);
        const auto start = std::chrono::steady_clock::now();
        const double area = coveredArea(shapes, static_cast< std::size_t >(request.threads),
            static_cast< std::size_t >(request.tries), calculation_seed);
        const auto finish = std::chrono::steady_clock::now();
        const std::chrono::duration< double, std::milli > elapsed = finish - start;
        result_t result = {};
        result.calculation_id = request.calculation_id;
        result.milliseconds = elapsed.count();
        result.area = area;
        if (!sendAll(result_descriptor, &result, sizeof(result_t))) {
          break;
        }
      }
    }

  }

  process_t createProcess(const std::string& name, std::size_t seed, const std::vector< int >& inherited)
  {
    int request_pipe[pipe_end_count] = {};
    int result_pipe[pipe_end_count] = {};
    if (pipe(request_pipe) != 0) {
      throw std::runtime_error("cannot create a pipe");
    }
    if (pipe(result_pipe) != 0) {
      static_cast< void >(close(request_pipe[0]));
      static_cast< void >(close(request_pipe[1]));
      throw std::runtime_error("cannot create a pipe");
    }
    const pid_t pid = fork();
    if (pid < 0) {
      static_cast< void >(close(request_pipe[0]));
      static_cast< void >(close(request_pipe[1]));
      static_cast< void >(close(result_pipe[0]));
      static_cast< void >(close(result_pipe[1]));
      throw std::runtime_error("cannot create a process");
    }
    if (pid == 0) {
      for (const int descriptor : inherited) {
        static_cast< void >(close(descriptor));
      }
      static_cast< void >(close(request_pipe[1]));
      static_cast< void >(close(result_pipe[0]));
      runCalculator(request_pipe[0], result_pipe[1], seed);
      static_cast< void >(close(request_pipe[0]));
      static_cast< void >(close(result_pipe[1]));
      _exit(0);
    }
    static_cast< void >(close(request_pipe[0]));
    static_cast< void >(close(result_pipe[1]));
    setDescriptorMode(result_pipe[0], true);
    process_t process;
    process.name = name;
    process.pid = pid;
    process.request_descriptor = request_pipe[1];
    process.result_descriptor = result_pipe[0];
    return process;
  }

  bool sendCalculation(const process_t& process, const request_t& request, const std::vector< shape_t >& shapes)
  {
    if (!sendAll(process.request_descriptor, &request, sizeof(request_t))) {
      return false;
    }
    std::vector< shape_data_t > data(shapes.size());
    for (std::size_t index = 0; index < shapes.size(); ++index) {
      data[index].radius = shapes[index].radius;
      data[index].x = shapes[index].center.x;
      data[index].y = shapes[index].center.y;
    }
    return sendAll(process.request_descriptor, data.data(), data.size() * sizeof(shape_data_t));
  }

  bool receiveResult(process_t& process, result_t& result)
  {
    if (process.result_descriptor == invalid_descriptor) {
      return false;
    }
    const ssize_t received = read(process.result_descriptor, &result, sizeof(result_t));
    if (received == static_cast< ssize_t >(sizeof(result_t))) {
      return true;
    }
    if (received == 0) {
      closeDescriptor(process.result_descriptor);
    }
    return false;
  }

  bool receiveResultBlocking(process_t& process, result_t& result)
  {
    if (process.result_descriptor == invalid_descriptor) {
      return false;
    }
    setDescriptorMode(process.result_descriptor, false);
    const bool is_received = receiveAll(process.result_descriptor, &result, sizeof(result_t));
    setDescriptorMode(process.result_descriptor, true);
    if (!is_received) {
      closeDescriptor(process.result_descriptor);
    }
    return is_received;
  }

  void closeProcess(process_t& process)
  {
    closeDescriptor(process.request_descriptor);
    if (process.pid != 0) {
      static_cast< void >(kill(process.pid, SIGTERM));
      int status = 0;
      static_cast< void >(waitpid(process.pid, &status, 0));
      process.pid = 0;
    }
    closeDescriptor(process.result_descriptor);
  }

}
