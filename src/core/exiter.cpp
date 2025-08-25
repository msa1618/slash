#include "exiter.h"
#include "jobs.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "startup.h"
#include <algorithm>
#include <unistd.h>
#include <cctype>

int slash_exit(bool dont_exit_yet) {
    if (JobCont::get_running_jobs() == 0) {
        enable_canonical_mode();
        _exit(0);
    }

    bool good_input = false;

    do {
        int size = JobCont::jobs.size();
        std::string first_part = (size > 1 ? "There are " + std::to_string(size) + " running jobs."
                                            : "There is 1 running job.");

        info::warning(first_part + " Are you sure you want to exit slash? (Y/N): ");

        char c;
        std::string buffer;

        while (true) {
            if (read(STDIN_FILENO, &c, 1) != 1) continue;

            if (c == '\r' || c == '\n') {
                io::print("\n");
                break;
            }

            if (c == 127 || c == 8) {
                if(!buffer.empty()) {
                    io::print("\b \b");
                    buffer.pop_back();
                }
            }

            if (isprint(static_cast<unsigned char>(c))) {
                io::print(std::string(1, c));
                buffer.push_back(c);
            }
        }

        std::transform(buffer.begin(), buffer.end(), buffer.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        if (!buffer.starts_with("y") && !buffer.starts_with("n")) {
            info::error("Please type 'y' or 'n'\n");
            continue;
        }

        good_input = true;

        if (buffer.starts_with("n")) continue;
        else {
            if (dont_exit_yet) return 1; // ok you can exit now
            else {
                enable_canonical_mode();
                _exit(0);
            }
        }
    } while (!good_input);

    return 0; // fallback
}
