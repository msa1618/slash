#ifndef SLASH_GIT_H
#define SLASH_GIT_H

#include <git2.h>
#include <string>
#include <vector>

class GitRepo {
  private:
  git_repository* repo;
  std::string path;

  public:
  GitRepo(std::string repo_path);
  ~GitRepo();

  bool has_git_repo();
  std::string get_root_path();
  std::string get_file_status(std::string filepath);
  std::vector<std::pair<std::string, std::string>> get_file_changes(std::string filepath);
  std::string get_branch_name();

  git_repository* get_repo();
};

#endif // SLASH_GIT_H
