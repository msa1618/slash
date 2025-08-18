#include "jobs.h"
#include "../core/jobs.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include <algorithm>
#include "../help_helper.h"

int resume_pid(int pid) { 
    JobCont::resume_job(pid); 
    return 0;
}

int resume_index(int index) { 
    JobCont::resume_job(JobCont::jobs[index].pgid); 
    return 0;
}

int kill_pid(int pid) { 
    JobCont::kill_job(pid); 
    return 0;
}

int kill_index(int index) { 
    JobCont::kill_job(JobCont::jobs[index].pgid); 
    return 0;
}

int jobs(std::vector<std::string>& args) {
    if(args.empty()) {
        JobCont::print_jobs();
        return 0;
    }

    for(int i = 0; i < args.size(); i++) {
        if(args[i] == "-h" || args[i] == "--help") {
            std::stringstream custom;
            custom << green << "States\n" << reset;
            custom << yellow << "  • Running:     " << reset << "The job is currently running\n";
            custom << yellow << "  • Stopped:     " << reset << "The job has been stopped with " << blue << "Ctrl+Z\n" << reset;
            custom << yellow << "  • Completed:   " << reset << "The job has finished execution\n";
            custom << yellow << "  • Interrupted: " << reset << "The job has been interrupted by a signal like SIGINT\n";
            custom << yellow << "  • Wakekill:    " << reset << "The job has been killed by a deadly signal like SIGKILL,\n                 so resuming it will result in its death\n";

            io::print(get_helpmsg({
                "Manage, kill, and resume slash jobs",
                {
                    "jobs",
                    "jobs [option] <pid>"
                },
                {
                    {"-r", "--resume-pid", "Resume a job by its PID"},
                    {"-k", "--kill-pid", "Kill a job by its PID"}
                },
                {
                    {"jobs", "Display all jobs"},
                    {"jobs -r 12345", "Resume job with PID 12345"},
                },
                custom.str(),
                ""
            }));
            return 0;
        }

        auto parse_number = [&](int idx) -> int {
            if(idx + 1 >= args.size()) {
                info::error("No number specified after " + args[idx]);
                return -1;
            }
            if(!std::all_of(args[idx + 1].begin(), args[idx + 1].end(), ::isdigit)) {
                info::error("Argument must be a number after " + args[idx]);
                return -1;
            }
            return std::stoi(args[idx + 1]);
        };

        if(args[i] == "-k" || args[i] == "--kill-pid") {
            int pid = parse_number(i);
            if(pid != -1) kill_pid(pid);
            i++; // skip next argument
        } else if(args[i] == "-r" || args[i] == "--resume-pid") {
            int pid = parse_number(i);
            if(pid != -1) resume_pid(pid);
            i++;
        } else if(args[i] == "-i" || args[i] == "--resume-index") {
            int idx = parse_number(i);
            if(idx != -1) {
                if(idx < 0 || idx >= (int)JobCont::jobs.size()) {
                    info::error("Invalid job index: " + std::to_string(idx));
                } else {
                    resume_index(idx);
                }
            }
            i++;
        } else {
            info::error("Unknown argument: " + args[i]);
            return -1;
        }
    }

    return 0;
}
