#include "git.h"
#include <iostream>
#include <filesystem>

GitRepo::GitRepo(std::string repo_path) : repo(nullptr), path(repo_path) {
	git_libgit2_init();
	if (git_repository_open(&repo, repo_path.c_str()) != 0) {
		repo = nullptr;
	}
}

GitRepo::~GitRepo() {
	if (repo) git_repository_free(repo);
	git_libgit2_shutdown();
}

bool GitRepo::has_git_repo() {
	std::filesystem::path current = path;
	while(true) {
		if(git_repository_open(&repo, current.c_str())) {
			return true;
		}

		if(current == "/") break;
		current = current.parent_path();
	}

	return false;
}

bool GitRepo::is_file_staged(const std::string& filepath, std::string dir_path) {
	if (git_repository_open(&repo, dir_path.c_str()) != 0) return false;

	git_index* index = nullptr;
	if (git_repository_index(&index, repo) != 0) {
		return false;
	}

	const git_index_entry* entry = git_index_get_bypath(index, path.c_str(), 0);
	bool staged = (entry != nullptr);

	git_index_free(index);
	return staged;
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

