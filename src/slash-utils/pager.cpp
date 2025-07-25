#include <ftxui/component/component.hpp>       // for Renderer, CatchEvent, Vertical
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/dom/elements.hpp>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../command.h"

using namespace ftxui;

class Pager : public Command {
	private:
	int scroll_ = 0;
	int current_line = 0;
	std::string filename_;
	bool show_help = false;
	ScreenInteractive screen = ScreenInteractive::Fullscreen();

	public:
	Pager() : Command("", "", "") {}

	Component Render(std::vector<std::string> lines_, std::string filename = "") {
		for (auto &l : lines_) io::replace_all(l, "\t", "  ");

    int total_lines = 0;
    total_lines = lines_.size();

		std::string header_content = !filename.empty() ? "pager - " + filename : "pager";
		auto header = text(header_content) | center | inverted;

		auto list_component = Renderer([&] {
			int height = screen.dimy() - 4; // 4 because of header, footer, and separators
      if(height > lines_.size()) {
        while(height > lines_.size()) {
          lines_.push_back("");
        }
      }
			int start = scroll_;
			int end = std::min(scroll_ + height, (int)lines_.size());

			Elements displayed;
			for (int i = start; i < end; ++i) {
				displayed.push_back(text(lines_[i]));
			}

			auto content = vbox(std::move(displayed) | flex);
			current_line = height + scroll_;
      if(current_line > total_lines) current_line = total_lines;

			std::stringstream ss;
			float percent = (float)current_line / total_lines * 100;
			ss << "Ln " << current_line << "/" << total_lines << " " << (int)percent << "% read ";
			ss << "(Use ↑↓ to scroll)";
			auto footer = hbox({
													 text(ss.str()),
												 }) | inverted;

			return vbox({
										header,
										separator(),
										content,
										separator(),
										footer
									});
		});

		auto component = CatchEvent(list_component, [&](auto event){
      int height = screen.dimy() - 4;
      int max_scroll = std::max(0, (int)lines_.size() - height);

      if(event == Event::ArrowDown || (event.is_mouse() && event.mouse().motion == Mouse::WheelDown)) {
        if(scroll_ < max_scroll) scroll_++;
      }

      if(event == Event::ArrowUp || (event.is_mouse() && event.mouse().motion == Mouse::WheelUp)) {
        if(scroll_ > 0) scroll_--;
      }

      return true;
    });

		screen.Loop(component);
		return component;
	}

	int exec(std::vector<std::string> args) {
		if (args.empty()) {
			io::print("pager: use entire terminal to display large text\n"
								"flags:\n"
								"-t | --text: the argument is text and not a filename (useful for piping)\n");
			return 0;
		}

		std::vector<std::string> valid_args = {
			"-t",
			"--text"
		};
		std::string arg;
		bool using_text = false;
		for (auto &a : args) {
			if (!io::vecContains(valid_args, a) && a.starts_with("-")) {
				info::error("Invalid argument \"" + a + "\"");
				return -1;
			}

			if (a == "-t" || a == "--text") using_text = true;
			if (!a.starts_with("-")) arg = a;
		}

		if (using_text) {
			if(!arg.empty()) Render(io::split(arg, "\n"));
      else {
        // Read stdin if piped to
        std::stringstream buffer;
        buffer << std::cin.rdbuf();
        Render(io::split(buffer.str(), "\n"));
      }
    }
		else {
			auto content = io::read_file(arg);
			if (!std::holds_alternative<std::string>(content)) {
				std::string error = std::string("Failed to open \"" + arg + "\": ") + strerror(errno);
				info::error(error, errno);
				return -1;
			}

			Render(io::split(std::get<std::string>(content), "\n"), arg);
		}
		return 0;
	}
};

int main(int argc, char *argv[]) {
	Pager pager;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	pager.exec(args);
	return 0;
}
