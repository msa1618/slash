#include "git.h"
#include "../abstractions/info.h"
#include <iostream>
#include <filesystem>

GitRepo::GitRepo(std::string repo_path) : repo(nullptr), path(repo_path) {
    git_libgit2_init();

    std::filesystem::path current = repo_path;
    while (true) {
        if (git_repository_open_ext(&repo, current.c_str(), 0, nullptr) == 0) {
            path = current.string();
            break;
        }

        if (current == current.root_path()) {
            repo = nullptr;
            break;
        }

        current = current.parent_path();
    }
}

GitRepo::~GitRepo() {
	if (repo) git_repository_free(repo);
	git_libgit2_shutdown();
}

git_repository* GitRepo::get_repo() {
	return repo;
}

bool GitRepo::has_git_repo() {
	std::filesystem::path current = path;
	while(true) {
		if (git_repository_open_ext(&repo, current.c_str(), 0, nullptr) == 0) {
			return true;
		}

		if(current == "/") break;
		current = current.parent_path();
	}

	return false;
}

std::string GitRepo::get_root_path() {
    const char* root = git_repository_workdir(this->repo);
    if (root) return std::string(root);
    return "";  // or throw or handle error
}

std::string GitRepo::get_file_status(std::string filepath) {

	// Characters:
	// N: Untracked
	// U: Unmodified
	// M: Modified
	// S: Staged
	// I: Ignored

	unsigned int status_flags = 0;
	int error = git_status_file(&status_flags, repo, filepath.c_str());
if (error != 0) {
    return "";
}

if (status_flags & GIT_STATUS_CURRENT) {
    return "U";
}
if (status_flags & GIT_STATUS_WT_NEW) {
    return "N";
}
if (status_flags & GIT_STATUS_WT_MODIFIED) {
    return "M";
}
if (status_flags & (GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_INDEX_NEW |
                    GIT_STATUS_INDEX_DELETED | GIT_STATUS_INDEX_RENAMED)) {
    return "S";
}
if (status_flags & GIT_STATUS_IGNORED) {
    return "I";
}
if (status_flags & GIT_STATUS_WT_RENAMED) {
    return "R";
}

return "U";
}

std::vector<std::pair<std::string, std::string>> GitRepo::get_file_changes(std::string filepath) {
		if(get_file_status(filepath) == "N") {
			info::error("File is not tracked.");
			return {{}};
		}
    std::vector<std::pair<std::string, std::string>> lines_with_status;

    struct Payload {
        std::string filepath;
        std::vector<std::pair<std::string, std::string>>* out;
        const git_diff_line* last_deleted = nullptr;
    };

    Payload payload { filepath, &lines_with_status, nullptr };

    git_diff* diff;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    git_diff_index_to_workdir(&diff, repo, NULL, &opts);

    int res =
			git_diff_foreach(
        diff,
        nullptr, nullptr,
        [](const git_diff_delta* delta, const git_diff_hunk* hunk, void* payload) {
            return 0;
        },
        [](const git_diff_delta* delta, const git_diff_hunk* hunk, const git_diff_line* line, void* payload) {
            auto p = static_cast<Payload*>(payload);
            std::string current_path = delta->new_file.path ? delta->new_file.path : delta->old_file.path;

            if (current_path != p->filepath)
                return 0;

            if (line->origin == GIT_DIFF_LINE_DELETION) {
                p->last_deleted = line;
						} else if (line->origin == GIT_DIFF_LINE_CONTEXT) {
							 p->out->emplace_back(" ", std::string(line->content, line->content_len));
					  }
						 else if (line->origin == GIT_DIFF_LINE_ADDITION) {
                if (p->last_deleted) {
                    // Modified line (deletion + addition)
                    std::string add_line(line->content, line->content_len);
                    p->out->emplace_back(yellow + "~" + reset, add_line);
                    p->last_deleted = nullptr;
                } else {
                    p->out->emplace_back(green + "+"  + reset, std::string(line->content, line->content_len));
                }
            } else {
                // Context or unchanged lines reset last_deleted
                p->last_deleted = nullptr;
            }
            return 0;
        },
        &payload);	

    git_diff_free(diff);

    return lines_with_status;
}

std::string GitRepo::get_branch_name() {
	const char* branch_name = nullptr;
	git_reference* head = nullptr;
	std::filesystem::path current = path;

	while (true) {
		git_repository* temp_repo = nullptr;
		if (git_repository_open(&temp_repo, current.c_str()) == 0) {
			if (git_repository_head(&head, temp_repo) == 0) {
				if (git_branch_name(&branch_name, head) == 0 && branch_name) {
					std::string result(branch_name);
					git_reference_free(head);
					git_repository_free(temp_repo);
					return result;
				}
				git_reference_free(head);
			}
			git_repository_free(temp_repo);
		}

		if (current == "/") break;
		current = current.parent_path();
	}

	return "";
}

