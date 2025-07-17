#ifndef SLASH_GIT_H
#define SLASH_GIT_H

#include <git2.h>
#include <string>

class GitRepo {
	private:
	git_repository* repo;
	std::string path;

	public:
	GitRepo(std::string repo_path);
	~GitRepo();

	bool has_git_repo();
	bool is_file_staged(const std::string& filepath, std::string dir_path);
	std::string get_branch_name();
};

#endif // SLASH_GIT_H
