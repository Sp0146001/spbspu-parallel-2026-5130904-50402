#include <csignal>
#include <iostream>

#include "session.hpp"

int main()
{
  std::signal(SIGPIPE, SIG_IGN);
  petrov::Session session(std::cin, std::cout);
  session.runCommands();
  return 0;
}
