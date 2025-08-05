#include <chrono>
#include <string>
#include <locale>
#include <iostream>
#include <iomanip>
#include "../abstractions/iofuncs.h"
#include "../command.h"
#include "../abstractions/info.h"

class Datetime : public Command {
	private:
		void print_locale_time() {
			std::time_t time = std::time(nullptr);
			std::tm* localtime = std::localtime(&time);

			std::cout.imbue(std::locale("")); // Use system locale
			std::cout << std::put_time(localtime, "%c") << "\n"; // %c means locale datetime
		}

		void print_help() {
			io::print("datetime: print current locale's datetime; can be formatted\n"
								"flags:\n"
								"-f <formatted-in-quotes>\n\n"
								"POSIX datetime flags (for formatting): \n");

			std::vector<std::pair<std::string, std::string>> datetime_flags = {
				// Dates
				{"%Y:", "Year (4 digits)"},
				{"%y:", "Year (2 digits)"},
				{"%m:", "Month (01–12)"},
				{"%B:", "Full month name"},
				{"%b:", "Abbreviated month name"},
				{"%h:", "Abbreviated month name (same as %b)"},
				{"%d:", "Day of month (01–31)"},
				{"%e:", "Day of month (1–31, space-padded)"},
				{"%j:", "Day of year (001–366)"},
				{"%U:", "Week number (Sunday as first day, 00–53)"},
				{"%W:", "Week number (Monday as first day, 00–53)"},
				{"%C:", "Century (year / 100)"},
				{"%g:", "ISO 8601 year (2-digit)"},
				{"%G:", "ISO 8601 year (4-digit)"},
				{"%V:", "ISO 8601 week number (01–53)"}, // 14th element

				// Time
				{"%H:", "Hour (00–23)"},
				{"%I:", "Hour (01–12)"},
				{"%M:", "Minute (00–59)"},
				{"%S:", "Second (00–60)"},
				{"%p:", "AM or PM"},
				{"%r:", "12-hour clock time (%I:%M:%S %p)"},
				{"%R:", "24-hour time (%H:%M)"},
				{"%T:", "24-hour time (%H:%M:%S)"},
				{"%X:", "Locale time representation"}, // 23rd element

				// Locale-based
				{"%c:", "Locale full datetime representation"},
				{"%x:", "Locale date representation"},
				{"%D:", "Date (%m/%d/%y)"},
				{"%F:", "Date (%Y-%m-%d, ISO 8601)"}, // 27th element

				// Weekdays
				{"%A:", "Full weekday name"},
				{"%a:", "Abbreviated weekday name"},
				{"%u:", "ISO weekday (1=Mon, 7=Sun)"},
				{"%w:", "Weekday (0=Sun, 6=Sat)"}, // 31st element

				// Timezones
				{"%Z:", "Timezone abbreviation"},
				{"%z:", "UTC offset (+hhmm or -hhmm)"}, // 33rd element

				// slash-utils Special
				{"%E:", "Seconds since epoch"}, // 34th element

				// Special
				{"%%:", "Literal %"},
				{"%n:", "Newline"},
				{"%t:", "Tab character"} // 37th element
			};

			auto print_flags = [datetime_flags](std::string title, std::string color, int starting_index, int ending_index){
				io::print(color + title + reset + "\n");
				io::print(color + std::string(title.size() + 1, '-') + reset);
				io::print("\n");

				for(int i = starting_index; i <= ending_index; i++) {
					std::string flag = color + datetime_flags[i].first + reset;
					std::string desc = datetime_flags[i].second;

					io::print(flag + " " + desc + "\n");
				}

				io::print("\n");
			};

			print_flags("Dates", green, 0, 14);
			print_flags("Time", blue, 15, 23);
			print_flags("Locale-based", yellow, 24, 27);
			print_flags("Weekdays", orange,  28, 31);
			print_flags("Timezones", cyan, 32, 33);
			print_flags("slash-utils Special", red, 34, 34);
			print_flags("Special", magenta, 35, 37);
		};

		void print_formatted(std::string formatted) {
			std::time_t now = std::time(nullptr);
			std::tm* time = std::localtime(&now);

			char buffer[1024];
			std::strftime(buffer, 1024, formatted.c_str(), time);
			std::string time_str = buffer;

			std::string epoch = std::to_string(now);
			std::string token = "%E";

			size_t pos = 0;
			while ((pos = time_str.find(token, pos)) != std::string::npos) {
				time_str.replace(pos, token.length(), epoch);
				pos += epoch.length(); // move past the replacement
			}

			io::print(time_str + "\n");
		}

	public:
		Datetime() : Command("datetime", "", "") {}

		int exec(std::vector<std::string> args) {
			if(args.empty()) {
				print_locale_time();
				return 0;
			}

			for(int i = 0; i < args.size(); i++) {
				if(args[i] == "--help") {
					print_help();
					return 0;
				}

				if(args[i] == "-f") {
					if (i + 1 < args.size()) {
						print_formatted(args[++i]);
					} else {
						info::error("Flag \"-f\" requires a quote next to it");
						return EINVAL;
					}
				}
			}
		}
};

int main(int argc, char* argv[]) {
	Datetime datetime;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	datetime.exec(args);
	return 0;
}
