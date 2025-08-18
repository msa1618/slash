#ifndef SLASH_JOBS_H
#define SLASH_JOBS_H

#include <vector>
#include <string>

int resume_pid(int pid);
int kill_pid(int pid);

int jobs(std::vector<std::string>& args);

#endif // SLASH_JOBS_H
