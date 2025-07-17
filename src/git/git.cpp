#include "git.h"
#include <iostream>

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
	return git_repository_open(&repo, path.c_str()) == 0;
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

	if (git_repository_open(&repo, path.c_str()) != 0) return "";

	if (git_repository_head(&head, repo) == 0) {
		if (git_branch_name(&branch_name, head) == 0 && branch_name) {
			return std::string(branch_name);
		}
		git_reference_free(head);
	}
	return "";
}
