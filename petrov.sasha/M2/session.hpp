#ifndef SESSION_HPP
#define SESSION_HPP

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "../common/shape.hpp"
#include "process.hpp"

namespace petrov {

  struct calculation_t {
    int id = 0;
    std::string process_name;
    bool is_finished = false;
    double milliseconds = 0.0;
    double area = 0.0;
  };

  class Session {
  public:
    Session(std::istream& input, std::ostream& output);
    Session(const Session& other) = delete;
    Session& operator=(const Session& other) = delete;
    ~Session();

    void runCommands();

  private:
    using command_handler_t = bool (Session::*)(const std::vector< std::string >&);

    const shape_t* findShape(const std::string& name) const;
    const std::vector< std::string >* findSet(const std::string& name) const;
    process_t* findProcess(const std::string& name);
    calculation_t* findCalculation(const std::string& name);
    bool collectShapes(const std::string& name, std::vector< shape_t >& shapes) const;

    void executeLine(const std::string& line);
    bool createCircle(const std::vector< std::string >& tokens);
    bool createSet(const std::vector< std::string >& tokens);
    bool showShape(const std::vector< std::string >& tokens);
    bool showSet(const std::vector< std::string >& tokens);
    bool showFrame(const std::vector< std::string >& tokens);
    bool showSetFrame(const std::vector< std::string >& tokens);
    bool spawnProcess(const std::vector< std::string >& tokens);
    bool startCalculation(const std::vector< std::string >& tokens);
    bool showStatus(const std::vector< std::string >& tokens);
    bool waitCalculation(const std::vector< std::string >& tokens);

    void updateResults();
    void storeResult(const result_t& result);
    void printCalculation(const calculation_t& calculation) const;

    void closeProcesses();

    std::istream& input_;
    std::ostream& output_;
    std::map< std::string, shape_t > shapes_;
    std::map< std::string, std::vector< std::string > > sets_;
    std::vector< process_t > processes_;
    std::map< std::string, calculation_t > calculations_;
    int next_calculation_id_ = 1;
    const std::map< std::string, command_handler_t > commands_;
  };

}

#endif
