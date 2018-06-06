#include <klex/alphabet.h>
#include <sstream>

namespace klex {

std::string Alphabet::to_string() const {
  std::stringstream sstr;

  sstr << '{';

  for (Symbol c : alphabet_) {
    if (c == '\0')
      sstr << "ε";
    else
      sstr << c;
  }

  sstr << '}';

  return std::move(sstr.str());
}

} // namespace klex
