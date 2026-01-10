#include <nlohmann/json.hpp>
#include <string>

class HistoryManager {
  private:
    std::vector<std::string> his;

  public:
    HistoryManager();
    ~HistoryManager();

    std::string get(int line);
    std::string from_last(int index);
    void add(std::string line);
    decltype(his) history();
    
    size_t size();
};