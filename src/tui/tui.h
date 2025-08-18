#ifndef TUI_H
#define TUI_H

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../abstractions/iofuncs.h"

void enable_raw_mode();
void disable_raw_mode();

#define ON true
#define OFF false

namespace Tui {
    void switch_to_alternate();
    void switch_to_normal();
    void clear();

    void cleanup_and_exit(int code);

    void turn_cursor(bool b);
    void move_cursor_to_last();
    void move_cursor_to_first();
    void move_cursor_to(int row, int col);
    void scroll_up();
    void scroll_down();

    void turn_linewrap(bool b);

    void print_in_center_of_terminal(std::string text);
    void print_separator();

    int get_terminal_width();
    int get_terminal_height();

    std::string get_keypress();
}

#endif // TUI_H
