#include <history_manager.h>
#include <unistd.h>
#include <global.h>
#include <fstream>
#include <sstream>
#include <iofuncs.h>

HistoryManager::HistoryManager() {
  std::string filename = std::string(getenv("HOME")) + "/.slash_history";
  if(Global::interpreter.get_scope().has("HISTFILE")) filename = Global::interpreter.get_scope().get("HISTFILE");

  std::ifstream file(filename);
  if(!file) {
    io::perror("failed to open history file: " + filename);
    exit(1);
  }
  
  std::stringstream ss;
  ss << file.rdbuf();
  
  nlohmann::json j = nlohmann::json::parse(ss.str());

  if(!j.is_array()) return;
  
  for(auto& e : j) {
    if(!e.is_string()) continue;
    his.push_back(e.get<std::string>());
  }
}

HistoryManager::~HistoryManager() {
  std::string filename = std::string(getenv("HOME")) + "/.slash_history";
  if(Global::interpreter.get_scope().has("HISTFILE")) filename = Global::interpreter.get_scope().get("HISTFILE");

  FILE* f = fopen(filename.c_str(), "w");
  if(!f) {
    io::perror("failed to open history file: " + filename);
    exit(1);
  }

  nlohmann::json array = his;
  fprintf(f, "%s", array.dump(2).c_str());

  fclose(f);
}

std::string HistoryManager::get(int line) {
  if(size() >= line - 1) {} // todo

  return his[line - 1];
}
std::string HistoryManager::from_last(int index) {
    int n = size();
    if(n == 0) return "";

    index = index % n;
    if(index < 0) index += n;

    return his[n - index - 1];
}

void HistoryManager::add(std::string line) {
  his.push_back(line);
}

decltype(HistoryManager::his) HistoryManager::history() {
  return his;
}

size_t HistoryManager::size() {
  return his.size();
}
