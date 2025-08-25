#include "jobs.h"
#include "execution.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include <sys/wait.h>
#include <algorithm>

std::string strstate(JobCont::State state) {
  std::string res;
  switch(state) {
    case JobCont::State::Completed:   res = "Completed"; break;
    case JobCont::State::Running:     res = "Running"; break;
    case JobCont::State::Stopped:     res = "Stopped"; break;
    case JobCont::State::Interrupted: res = "Interrupted"; break;
    case JobCont::State::Terminated:  res = "Terminated"; break;
    case JobCont::State::Wakekill:    res = "Wakekill"; break;
    default:                          res = "Unknown"; break;
  }

  return res;
}

std::vector<JobCont::Job> JobCont::jobs;

void JobCont::add_job(int pgid, std::string name, State state, ExecFlags info, std::chrono::_V2::system_clock::time_point start) {
  jobs.push_back({pgid, name, state, info, start});
  setpgid(pgid, pgid);
}

void JobCont::update_job(int pgid, State state) {
  for(auto& job : jobs) {
    if(job.pgid == pgid) {
      job.pgid = pgid;
      job.jobstate = state;
    }
  }
}

void JobCont::remove_job(int pgid) {
  for(int i = 0; i < jobs.size(); i++) {
    if(jobs[i].pgid == pgid) jobs.erase(jobs.begin() + i);
  }
}

void JobCont::clear_jobs() {
  jobs.clear();
}

int JobCont::get_running_jobs() {
  int res = 0;
  std::vector<State> inactive_states = {
    State::Terminated, State::Interrupted, State::Wakekill, State::Completed
  };

  for(auto& job : jobs) {
    if(!io::vecContains(inactive_states, job.jobstate)) res++;
  }

  return res;
}

int JobCont::resume_job(int pgid) {
    auto it = std::find_if(jobs.begin(), jobs.end(),
                           [&](const Job& j){ return j.pgid == pgid; });
    int index = (int)std::distance(jobs.begin(), it);
    std::string name = (it != jobs.end() ? it->name : "job");

    struct sigaction old_ttou{};
    signal(SIGTTOU, SIG_IGN);

    if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
        info::error("tcsetpgrp failed: " + std::string(strerror(errno)), errno);
        signal(SIGTTOU, SIG_DFL);
        return -1;
    }

    if (kill(-pgid, SIGCONT) != 0) {
        info::error("kill(SIGCONT) failed: " + std::string(strerror(errno)), errno);
        tcsetpgrp(STDIN_FILENO, getpgrp());
        signal(SIGTTOU, SIG_DFL);
        return -1;
    }

    JobCont::update_job(pgid, JobCont::State::Running);

    int rc = wait_foreground_job(pgid, jobs[index].name, {jobs[index].flags.exit_code, jobs[index].flags.repeat, jobs[index].flags.time}, jobs[index].start);

    tcsetpgrp(STDIN_FILENO, getpgrp());
    signal(SIGTTOU, SIG_DFL);

    return rc;
}

void JobCont::kill_job(int pgid) {
  kill(-pgid, SIGKILL);
  JobCont::update_job(pgid, State::Wakekill);
}

std::string JobCont::get_jobs_in_csv() {
  if(jobs.empty()) return "";

  std::stringstream ss;
  ss << "pid,name,state\n";
  for(auto& job : jobs) {
    ss << job.pgid << "," << job.name << "," << strstate(job.jobstate) << "\n";
  }
  return ss.str();
}

void JobCont::print_jobs() {
    if (jobs.empty()) {
        io::print(yellow + "<No jobs found>\n" + reset);
        return;
    }
    if(!isatty(STDOUT_FILENO)) {
      io::print(get_jobs_in_csv());
      return;
    }

    int pad = 1; // spaces on each side
    int pid_l = 5;    // minimum PID width
    int state_l = 11; // minimum State width
    int pname_l = 4;  // minimum Name width

    // Compute dynamic name width
    for (auto& job : jobs) {
        if ((int)job.name.length() > pname_l) pname_l = job.name.length();
    }

    // Add padding to column widths
    pid_l += 2 * pad;
    pname_l += 2 * pad;
    state_l += 2 * pad;

    auto get_line = [](int len) {
      std::string res;
      for(int i = 0; i < len; i++) res += "─";
      return res;
    };

    // Header
    std::string header_pid = std::string(pad, ' ') + "PID" + std::string(pid_l - 2*pad - 3, ' ') + std::string(pad, ' ');
    std::string header_name = std::string(pad, ' ') + "Name" + std::string(pname_l - 2*pad - 4, ' ') + std::string(pad, ' ');
    std::string header_state = std::string(pad, ' ') + "State" + std::string(state_l - 2*pad - 5, ' ') + std::string(pad, ' ');

    io::print("┌" + get_line(pid_l) + "┬" + get_line(pname_l) + "┬" + get_line(state_l) + "┐\n");
    io::print("│" + header_pid + "│" + header_name + "│" + header_state + "│\n");
    io::print("├" + get_line(pid_l) + "┼" + get_line(pname_l) + "┼" + get_line(state_l) + "┤\n");

    // Rows
    for (auto& job : jobs) {
        std::string state = strstate(job.jobstate);

        // Determine color
        std::string state_color;
        if(state == "Completed") state_color = green;
        else if(state == "Running") state_color = cyan;
        else if(state == "Stopped" || state == "Interrupted") state_color = yellow;
        else if(state == "Wakekill" || state == "Terminated") state_color = red;
        else state_color = gray;

        std::string pid = std::to_string(job.pgid);
        std::string name = job.name;

        // Resize for padding
        pid.resize(pid_l - 2*pad, ' ');
        name.resize(pname_l - 2*pad, ' ');
        state.resize(state_l - 2*pad, ' ');

        // Add padding
        pid = std::string(pad, ' ') + pid + std::string(pad, ' ');
        name = std::string(pad, ' ') + name + std::string(pad, ' ');
        state = std::string(pad, ' ') + state + std::string(pad, ' ');

        // Apply colors
        pid = cyan + pid + reset;
        state = state_color + state + reset;

        io::print("│" + pid + "│" + name + "│" + state + "│\n");
    }

    // Footer
    io::print("└" + get_line(pid_l) + "┴" + get_line(pname_l) + "┴" + get_line(state_l) + "┘\n");
}
