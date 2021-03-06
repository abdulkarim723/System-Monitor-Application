#include <unistd.h>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "linux_parser.h"
#include "process.h"
#include "processor.h"
#include "system.h"

using std::set;
using std::size_t;
using std::string;
using std::vector;

Processor& System::Cpu() { return cpu_; }

vector<Process> System::Processes() {
  processes_.clear();
  for (auto it : LinuxParser::Pids()) {
    Process process(it);
    // filter dummy processes
    if (process.Ram() != "" && process.Command() != "")
      processes_.emplace_back(process);
  }
  // sort processes decreasend according to their cpu utilization
  std::sort(processes_.begin(), processes_.end(), [](auto a, auto b){return b<a;});
  return processes_;
}

std::string System::Kernel() { return LinuxParser::Kernel(); }

float System::MemoryUtilization() { return LinuxParser::MemoryUtilization(); }

std::string System::OperatingSystem() { return LinuxParser::OperatingSystem(); }

int System::RunningProcesses() { return LinuxParser::RunningProcesses(); }

int System::TotalProcesses() { return LinuxParser::TotalProcesses(); }

long int System::UpTime() { return LinuxParser::UpTime(); }