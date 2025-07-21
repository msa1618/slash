#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../abstractions/colors.h"
#include "../command.h"

#include <vector>
#include <string>

class Csv : public Command {
	private:
	std::string print_csv_table(std::string csv, std::string separator) {
		std::vector<std::vector<std::string>> table;
		auto rows = io::split(csv, "\n");
		for(auto& row : rows) {
			std::vector<std::string> data = io::split(row, separator);
			table.push_back(data);
		}

		int longest_cell_length = 0;

		for(int i = 0; i < table.size(); i++) {
			auto row = table[i];
			for(int j = 0; j < row.size(); j++) {
				if(row[j].length() > longest_cell_length) longest_cell_length = row[j].length(); // I know that it depends on the column but for now
			}
		}

		for(int i = 0; i < table.size(); i++) {
			auto row = table[i];
			for(int j = 0; j < table[i].size(); j++) {
				auto cell = row[j];
				cell.resize(longest_cell_length, ' ');
				io::print(cell + " │ ");
			}
			io::print("\n");
		}
	}

	public:
		Csv() : Command("csv", "", "") {}

		int exec(std::vector<std::string> args) {
			if (args.empty()) {
				io::print("csv: print raw csv in a pretty format\n"
									"flags:\n"
									"-s | --separator: specify the srator\n"
									"-t | --text     : use text as input\n");
				return 0;
			}

			std::vector<std::string> valid_args = {
				"-s", "-t",
				"--separator", "--text"
			};

			bool is_text = false;
			std::string separator = ",";
			std::string arg;

			for (int i = 0; i < args.size(); i++) {
				if (!io::vecContains(valid_args, args[i]) && args[i].starts_with("-")) {
					info::error("Invalid argument \"" + args[i] + "\"\n");
					return -1;
				}

				if (args[i] == "-s" || args[i] == "--separator") {
					separator = args[++i];
					++i;
				}

				if (args[i] == "-t" || args[i] == "--text") {
					arg = args[++i];
					is_text = true;
				}

				if (!args[i].starts_with("-") && !is_text) {
					arg = args[i];
				}
			}

			if (is_text) {
				print_csv_table(arg, separator);
				return 0;
			} else {
				auto content = io::read_file(arg);
				if (!std::holds_alternative<std::string>(content)) {
					std::string error = std::string("Failed to open file: ") + strerror(errno);
					info::error(error, errno);
					return -1;
				}
				print_csv_table(std::get<std::string>(content), separator);
			}
		}
};

	int main(int argc, char* argv[]) {
		Csv csv;

		std::vector<std::string> args;
		for (int i = 1; i < argc; ++i) {
			args.emplace_back(argv[i]);
		}

		csv.exec(args);
		return 0;
	}