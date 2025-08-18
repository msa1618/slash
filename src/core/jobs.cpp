#include "jobs.h"
#include "execution.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include <sys/wait.h>
#include <algorithm>

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

void JobCont::print_jobs() {
  for(auto& job : jobs) {
    // Placeholder for now
    std::string state;
    switch(job.jobstate) {
        case State::Completed:   state = "Completed"; break;
        case State::Running:     state = "Running";   break;
        case State::Stopped:     state = "Stopped";   break;
        case State::Interrupted: state = "Interrupted"; break;
        case State::Wakekill:    state = "Wakekill"; break;
        default:                 state = "Unknown";   break;
    }

    io::print(std::to_string(job.pgid) + "   " + job.name + "    " + state);
    io::print("\n");
  }
}