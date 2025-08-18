#include <sstream>
#include <stack>
#include <vector>
#include <cstring>
#include <algorithm>
#include <numbers>
#include <cmath>
#include <iomanip>
#include <regex>
#include <termios.h>
#include <unistd.h>

#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "syntax_highlighting/helper.h"

termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disable_raw_mode);

  termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  raw.c_lflag &= ~ICRNL;
  raw.c_lflag &= ~(OPOST);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

class Cmsh {
  private:
    std::vector<std::string> supported_funcs = {
        "sin", "cos", "tan", "trunc", "opp", "abs", "ceil", "floor",
        "sqrt", "cbrt", "ln", "NOT"
    };
    std::vector<std::string> supported_consts = {
      "PI", "EU", "PHI", "TAU",
    };

    std::vector<std::string> history;
    int history_index = 0;
    int cursor_pos = 0;
    double variables[26]; // 26 meaning letters of the english alphabet

    void print_help() {
      static const std::string help = R"(cmsh - Calculator Minishell: A mini-shell used to calculate mathematical expressions
Usage: cmsh               Open the shell
       cmsh <expression>  Evaluate and exit

Functions:
  sin(x), cos(x), tan(x)
  abs(x), ceil(x), floor(x), trunc(x),
  sqrt(x), cbrt(x),
  ln(),
  opp(x): positive to negative and vice versa

  +, -, *, /: Basic arithmetic
  &, ^, |, NOT(x): Bitwise operators (Note: Bitwise operators do not work on decimals; it will be truncated)

Constants:
  PI, TAU,
  PHI
  EU

Commands:
  help: display this help message
  clear: clear the terminal
  exit
)";
      io::print(help);
    }

    void set_var(char a, double value) {
      //variables[index] = value;
    };

    double get_constant_value(std::string constant) {
      if(constant == "PI") return 3.14159265358979323846;
      if(constant == "EU") return 2.71828182;
      if(constant == "PHI") return 1.618033988;
      if(constant == "TAU") return 3.14159265358979323846 * 2;
      return 0;
    }

    std::string highl(std::string buffer) {
      boost::regex funcs("(" + io::join(supported_funcs, "|") + ")");
      boost::regex nums("[0-9.]");
      boost::regex constants("(" + io::join(supported_consts, "|") + ")");

      std::string cfuncs = "\033[38;2;255;183;3m";
      std::string cnums = "\033[38;2;88;129;87m";
      std::string cconsts = "\033[38;2;142;202;230m";

      std::vector<std::pair<boost::regex, std::string>> patterns = {
        {funcs, cfuncs},
        {nums, cnums},
        {constants, cconsts}
      };

      return shighlight(buffer, patterns);
    }
    std::string read_input() {
      char input;
      std::string buffer;

      while(true) {
        if(read(STDIN_FILENO, &input, 1) != 1) continue;

        if(input == '\n') {
          history.push_back(buffer);
          cursor_pos = 0;
          io::print("\n");
          break;
        }

        if(input == 8 || input == 127) {
          if (cursor_pos > 0 && cursor_pos <= buffer.size()) {
            buffer.erase(buffer.begin() + (cursor_pos - 1));
            cursor_pos--;

            io::print("\033[2K\r"); // Clear line
            io::print("$ ");
            io::print(highl(buffer));

             // Move cursor back to correct position
            if(!buffer.empty()) {
              int diff = buffer.length() - cursor_pos;
              if (diff > 0) {
                std::stringstream ss;
                ss << "\033[" << diff << "D";
                io::print(ss.str());
              }
            }
          }
          continue;
        }

        if(input == 27) {
          char seq[2];
          if(read(STDIN_FILENO, &seq[0], 1) != 1) continue;
          if(read(STDIN_FILENO, &seq[1], 1) != 1) continue;

          switch(seq[1]) {
            case 'A': {
              if(!history.empty() && history_index < history.size()) {
                buffer = history[history_index];
                io::print("\033[2K\r"); // Clear line and move cursor to the beginning
                io::print("$ ");
                io::print(highl(history[history_index]));
                cursor_pos = history[history_index].length();
                
                if(history_index < history.size()) history_index++;
              }

              break;
            }

            case 'B': {
              if(history.size() > 0 && history_index < history.size()) {
                buffer = history[history_index];
                io::print("\033[2K\r"); // Clear line and move cursor to the beginning
                io::print("$ ");

                io::print(highl(history[history_index]));
                cursor_pos = history[history_index].length();
                
                if(history_index > 0) history_index--;
              }

              break;
            }

            case 'C': {
              if(cursor_pos <= buffer.size()) {
                io::print("\033[1C");
                cursor_pos++;
              }

              break;
            }
            case 'D': {
              if(cursor_pos > 0) {
                io::print("\033[1D");
                cursor_pos--;
              }

              break;
            }
          }
          continue;
        }

        if(isprint(input)) {
          if (cursor_pos >= 0 && cursor_pos <= buffer.size()) {
            buffer.insert(buffer.begin() + cursor_pos, input);
            io::print("\033[2K\r"); // Clear line and move cursor to the beginning
            io::print("$ ");
            io::print(highl(buffer));

            cursor_pos++;

            int diff = buffer.length() - cursor_pos;
            if (diff > 0) {
              std::stringstream ss;
              ss << "\033[" << diff << "D";  // Move cursor left
              io::print(ss.str());
            }
          }      
          continue;
        }
      }

      return buffer;
    }

    int precedence(char op) {
        if (op == '+' || op == '-') return 1;
        if (op == '*' || op == '/') return 2;
        return 0; // For parentheses
    }
    int apply_op(std::stack<double>& operands, std::stack<std::string>& opers) { // int is status
      std::string opr = opers.top(); opers.pop();

      double res = 0;

      if(io::vecContains(supported_funcs, opr)) {
        if (operands.empty()) throw std::runtime_error("Missing operand for function " + opr);
        double x = operands.top(); operands.pop();

        if(opr == "sin") res = std::sin(x);
        if(opr == "cos") res = std::cos(x);
        if(opr == "tan") res = std::tan(x);
        if(opr == "abs") res = std::abs(x);
        if(opr == "ceil") res = std::ceil(x);
        if(opr == "floor") res = std::floor(x);
        if(opr == "sqrt") res = std::sqrt(x);
        if(opr == "cbrt") res = std::cbrt(x);
        if(opr == "trunc") res = std::trunc(x);
        if(opr == "ln") res = std::log(x);
        if(opr == "opp") res = x * -1;
        if(opr == "NOT") res = ~static_cast<int>(x);
      } else {
        if(!operands.empty()) {
          if (operands.size() < 2) throw std::runtime_error("Missing operands for operator " + opr);
          double b = operands.top(); operands.pop();
          double a = operands.top(); operands.pop();

          if(opr == "/" && b == 0) {
            throw std::runtime_error("Division by zero!");
          }

          if(opr == "+") res = a + b;
          if(opr == "-") res = a - b;
          if(opr == "*" || opr == "x") res = a * b;
          if(opr == "/") res = a / b;

          int a_int = static_cast<int>(a),
              b_int = static_cast<int>(b);
          if(opr == "&") res = a_int & b_int;
          if(opr == "^") res = a_int ^ b_int;
          if(opr == "|") res = a_int | b_int;
        }
      }

      operands.push(res);
    }
    double evaluate(std::string expr) {
      std::stack<double> operands;
      std::stack<std::string> operators;

      for(int i = 0; i < expr.size(); i++) {
        char c = expr[i];
        if (!isdigit(c) && !isspace(c) && c != '+' && c != '-' && c != '*' && c != '^' && c != '&' && c != '|' && c != '/' && c != '(' && c != ')' && !isalpha(c)) {
          throw std::runtime_error("Invalid token \"" + std::string(1, c) + "\"");
          continue;
        }
        if(isspace(c)) continue;
        if(isdigit(c) || c == '.') {
          int j = i;
          while (j < expr.size() && (isdigit(expr[j]) || expr[j] == '.')) j++;
          operands.push(std::stod(expr.substr(i, j - i)));
          i = j - 1;
        }
        if(c == '(') operators.push("(");
        if(c == ')') {
          while (!operators.empty() && operators.top() != "(") {
            apply_op(operands, operators);
          }

          operators.pop(); // pop the '('
        }
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '&' || c == '|' || c == '^') {
          std::string curr_op(1, c);
          while (!operators.empty() && precedence(operators.top()[0]) >= precedence(c)) {
            if (operators.top() == "(") break;
            apply_op(operands, operators);
          }
          operators.push(curr_op);
        }

        if (isalpha(c)) {
          int j = i;
          while (j < expr.size() && isalpha(expr[j])) j++;
          std::string word = expr.substr(i, j - i);

          if (io::vecContains(supported_funcs, word)) {
              operators.push(word);
          } else if (io::vecContains(supported_consts, word)) {
              operands.push(get_constant_value(word));
          } else {
              throw std::runtime_error("Invalid token: \"" + word + "\"");
          }

          i = j - 1;
      }
      }

    while(!operators.empty()) { apply_op(operands, operators); }

    return operands.top();
  }

  public:
    int exec(std::vector<std::string> args) {
      if(!args.empty()) {
        std::string expr;
        for(auto& arg : args) {
          expr += arg;
        }
        double result = evaluate(expr);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(10) << result;

        std::string res_str = ss.str();
        res_str.erase(res_str.find_last_not_of('0') + 1);

        if (res_str.back() == '.') res_str.pop_back();
        io::print(res_str + "\n");
         
        return 0;
      }

      io::print("Welcome to " + green + "Calculator Minishell!" + reset + " Type 'help' for instructions, and 'exit' to exit.\n");

      enable_raw_mode();

      while(true) {
        io::print("$ ");
        std::string input = read_input();
        if(input.empty()) continue;
        if(input == "exit") {
          disable_raw_mode();
          return 0;
        }
        if(input == "help") {
          print_help();
          continue;
        };
        if(input == "clear") {
          io::print("\033[2J\033[H");
          continue;
        }
        
        int leftpar_count = std::count(input.begin(), input.end(), '(');
        int rightpar_count = std::count(input.begin(), input.end(), ')');

        int leftcur_count = std::count(input.begin(), input.end(), '{');
        int rightcur_count = std::count(input.begin(), input.end(), '}');

        if(leftpar_count != rightpar_count || leftcur_count != rightcur_count) {
          info::error("Mismatched parentheses");
          continue;
        }

        double result = 0;
        try {
          result = evaluate(input);
        } catch(const std::runtime_error& e) {
          info::error(e.what());
          continue;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(10) << result;

        std::string res_str = ss.str();
        res_str.erase(res_str.find_last_not_of('0') + 1);

        if (res_str.back() == '.') res_str.pop_back();
        io::print(res_str + "\n");
      }
    }
};

int main(int argc, char* argv[]) {
  Cmsh cmsh;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  cmsh.exec(args);
  return 0;
}