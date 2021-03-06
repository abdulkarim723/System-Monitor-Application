#include <unistd.h>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "linux_parser.h"
#include "process.h"
#include "format.h"

using std::string;
using std::to_string;
using std::vector;

Process::Process(const Process& rhs){
  data_ = std::make_unique<processStruct>();
  data_->command = rhs.data_->command;
  data_->cpuUti = rhs.data_->cpuUti;
  data_->pid = rhs.data_->pid;
  data_->ram = rhs.data_->ram;
  data_->upTime = rhs.data_->upTime;
  data_->user = rhs.data_->user;
  pid_ = rhs.pid_;
}
Process::Process(Process&& rhs){
  data_ = std::move(rhs.data_);
  pid_ = rhs.pid_;
  rhs.pid_ = 0;
}
Process& Process::operator=(const Process& rhs){
  if(this == &rhs) return *this;
  data_->command = rhs.data_->command;
  data_->cpuUti = rhs.data_->cpuUti;
  data_->pid = rhs.data_->pid;
  data_->ram = rhs.data_->ram;
  data_->upTime = rhs.data_->upTime;
  data_->user = rhs.data_->user;
  pid_ = rhs.pid_;
  return *this;
}
Process& Process::operator=(Process&& rhs){
  data_ = std::move(rhs.data_);
  pid_ = rhs.pid_;
  rhs.pid_ = 0;
  return *this;
}

// Return this process's ID
int Process::Pid() { return pid_; }

// Return this process's CPU utilization
// calculation method is taken from
// https://stackoverflow.com/questions/16726779/how-do-i-get-the-total-cpu-usage-of-an-application-from-proc-pid-stat/16736599#16736599
float Process::CpuUtilization() const {
  float cpuUti = 0;
  vector<string> statVal{};
  std::ifstream stream(LinuxParser::kProcDirectory + std::to_string(pid_) +
                       "/stat");
  if (stream.is_open()) {
    string line;
    string val;
    std::getline(stream, line);
    std::istringstream linestream(line);
    for (int cnt = 0; cnt < 22; cnt++) {
      linestream >> val;
      statVal.push_back(val);
    }
    long sysFreq = sysconf(_SC_CLK_TCK);
    float upTimeSys = LinuxParser::UpTime();
    long upTimeProc = std::stol(statVal[13]);
    long sTimeKern = std::stol(statVal[14]);
    long cutime = std::stol(statVal[15]);
    long cstime = std::stol(statVal[16]);
    long startTime = std::stol(statVal[21]);
    long totalTime = upTimeProc + sTimeKern + cutime + cstime;
    float processUptime = upTimeSys - (static_cast<float>(startTime) / sysFreq);
    cpuUti = ((static_cast<float>(totalTime) / sysFreq) / processUptime);
  }
  return cpuUti;
}

string Process::Command() {
  string command = LinuxParser::Command(pid_);
  if (command.length() > 30) return command.substr(0, 30) + "...";
  return command;
}

string Process::Ram() { return LinuxParser::Ram(pid_); }

string Process::User() { return LinuxParser::User(pid_); }

long Process::UpTime() { return LinuxParser::UpTime() - LinuxParser::UpTime(pid_); }

bool Process::operator<(Process const& a) const {
  return CpuUtilization() < a.CpuUtilization();
}

void Process::calcProcessValues(){
  data_->pid = Pid();
  data_->user = User();
  data_->command = Command();
  data_->cpuUti = CpuUtilization();
  data_->ram = Ram();
  data_->upTime = Format::ElapsedTime(UpTime());
}