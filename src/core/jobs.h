#ifndef SLASH_JOB_H
#define SLASH_JOB_H

#include <unistd.h>
#include <vector>
#include <string>
#include <csignal>
#include <chrono>

#include "execution.h"

namespace JobCont {
  enum class State {Stopped, Running, Completed, Interrupted, Terminated, Wakekill};

  struct Job {
    int pgid;
    std::string name;
    State jobstate;
    ExecFlags flags;
    std::chrono::_V2::system_clock::time_point start;
  };

  extern volatile sig_atomic_t job_changed;
  extern std::vector<Job> jobs;

  void add_job(int pgid, std::string name, State state, ExecFlags flags, std::chrono::_V2::system_clock::time_point start);
  void update_job(int pgid, State state);
  void remove_job(int pgid);
  void clear_jobs();
  int get_running_jobs();

  int resume_job(int pgid);
  void kill_job(int pgid);

  std::string get_jobs_in_csv();

  void print_jobs();
}

#endif // SLASH_JOB_H