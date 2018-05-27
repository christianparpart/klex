#include <lexer/regexpr.h>

#include <cstdlib>
#include <iostream>
#include <string>

using namespace lexer;

int main(int argc, const char* argv[]) {
  if (argc == 2) {
    std::string input = argv[1];
    RegExprParser rep;
    std::unique_ptr<RegExpr> expr = rep.parse(input);
    std::string output = expr->to_string();
    std::cout << "INPUT  : " << input << "\n";
    std::cout << "OUTPUT : " << output << "\n";
    return EXIT_SUCCESS;
  }

  std::string input;
  std::istream& stream = std::cin;
  for (std::getline(stream, input); !stream.eof(); std::getline(stream, input)) {
    std::cout << "INPUT  : " << input << "\n";
    RegExprParser rep;
    std::unique_ptr<RegExpr> expr = rep.parse(input);
    std::string output = expr->to_string();
    std::cout << "OUTPUT : " << output << "\n";
  }

  return EXIT_SUCCESS;
}
