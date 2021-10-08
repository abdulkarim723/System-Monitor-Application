#include <dirent.h>
#include <unistd.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>

#include <iostream>
#include "format.h"
#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

long LinuxParser::GetSystemValue(string key, string path) {
  string line;
  string value;
  long val = 0;
  std::ifstream stream(path);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      // value = cpu
      linestream >> value;
      if (value == key) {
        linestream >> value;
        val = stol(value);
        return val;
      }
    }
  }
  return val;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, kernel, version;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

// TODO: Read and return the system memory utilization
float LinuxParser::MemoryUtilization() {
  long memTotal =
      GetSystemValue("MemTotal:", kProcDirectory + kMeminfoFilename);
  long memFree = GetSystemValue("MemFree:", kProcDirectory + kMeminfoFilename);
  return static_cast<float>(memTotal - memFree) / memTotal;
}

// TODO: Read and return the system uptime
long LinuxParser::UpTime() {
  string line;
  long uptime = 0;
  std::ifstream stream(kProcDirectory + kUptimeFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> uptime;
    return uptime;
  }
  return uptime;
}

// TODO: Read and return the number of jiffies for the system
long LinuxParser::Jiffies() { return sysconf(_SC_CLK_TCK) * UpTime(); }

// TODO: Read and return the number of active jiffies for a PID
long LinuxParser::ActiveJiffies(int pid) {
  string line;
  string value;
  long sum = 0;
  std::ifstream stream(kProcDirectory + to_string(pid) + "/stat");
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    for (int cnt = 0; cnt <= 16; cnt++) {
      linestream >> value;
      if (cnt > 12) sum += stol(value);
    }
  }
  return sum;
}

// TODO: Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() {
  string line;
  string value;
  long sum = 0;
  vector<long> activeJiffies{};
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> value;
      if (value == "cpu") {
        while (linestream >> value) {
          activeJiffies.push_back(std::stol(value));
          sum = stol(value);
        }
        // calculate the sum of active jiffies
        sum = activeJiffies[CPUStates::kUser_] +
              activeJiffies[CPUStates::kNice_] +
              activeJiffies[CPUStates::kSystem_] +
              activeJiffies[CPUStates::kIRQ_] +
              activeJiffies[CPUStates::kSoftIRQ_];
        return sum;
      }
    }
  }
  return sum;
}

// TODO: Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() {
  string line;
  string value;
  long sum = 0;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    // value = cpu
    linestream >> value;
    for (int cnt = 0; cnt < 5; cnt++) {
      linestream >> value;
      if (cnt == 3 || cnt == 4) sum += stol(value);
    }
  }
  return sum;
}

// Read and return CPU utilization
float LinuxParser::CpuUtilization() {
  long active = ActiveJiffies();
  long idle = IdleJiffies();
  usleep(1000000);
  // read cpu usage after 100 ms time difference
  long activeNew = ActiveJiffies();
  long idleNew = IdleJiffies();

  long total = active + idle;
  long totalNew = activeNew + idleNew;

  long totald = totalNew - total;
  long idled = idleNew - idle;
  return static_cast<float>((totald - idled)) / totald;
}

// Read and return the total number of processes
int LinuxParser::TotalProcesses() {
  return GetSystemValue("processes", kProcDirectory + kStatFilename);
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses() {
  return GetSystemValue("procs_running", kProcDirectory + kStatFilename);
}

// Read and return the command associated with a process
string LinuxParser::Command(int pid) {
  string cmd;
  std::fstream stream(kProcDirectory + std::to_string(pid) + "/cmdline");
  if (stream.is_open()) std::getline(stream, cmd);
  return cmd;
}

// TODO: Read and return the memory used by a process
string LinuxParser::Ram(int pid) {
  // reference for the way how the double was changed to a string is:
  // https://stackoverflow.com/questions/332111/how-do-i-convert-a-double-into-a-string-in-c
  long siztInKiB = GetSystemValue(
      "VmSize:", kProcDirectory + std::to_string(pid) + "/status");
  std::ostringstream outStr;
  outStr << static_cast<double>(siztInKiB) / 1024;
  return outStr.str();
}

// TODO: Read and return the user ID associated with a process
string LinuxParser::Uid(int pid) {
  long uid =
      GetSystemValue("Uid:", kProcDirectory + std::to_string(pid) + "/status");
  return to_string(uid);
}

// TODO: Read and return the user associated with a process
string LinuxParser::User(int pid) {
  string line, username, passwd, uid;
  string uid_ = Uid(pid);
  std::ifstream stream(kPasswordPath);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      linestream >> username >> passwd >> uid;
      if (uid == uid_) return username;
    }
  }
  return username;
}

// TODO: Read and return the uptime of a process
long LinuxParser::UpTime(int pid) {
  long uptimeError = -1;
  string line, val;
  std::ifstream stream(kProcDirectory + to_string(pid) + "/stat");
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    for (auto cnt = 0; cnt < 22; cnt++) {
      linestream >> val;
      if (cnt == 21) return std::stoi(val);
    }
  }
  return uptimeError;
}
