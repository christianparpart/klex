#include <klex/DotVisitor.h>
#include <sstream>
#include <fmt/format.h>

namespace klex {

template<typename StringType>
static std::string escapeString(const StringType& str) {
  std::stringstream stream_;
  for (char ch : str) {
    switch (ch) {
      case '\\':
        stream_ << "\\\\";
        break;
      case '\r':
        stream_ << "\\r";
        break;
      case '\n':
        stream_ << "\\n";
        break;
      case '\t':
        stream_ << "\\t";
        break;
      case '"':
        stream_ << "\\\"";
        break;
      case ' ':
        stream_ << ' ';
        break;
      default:
        if (std::isprint(ch)) {
          stream_ << ch;
        } else {
          stream_ << fmt::format("\\x{:02x}", ch);
        }
    }
  }
  return stream_.str();
}

void DotfileWriter::start() {
  stream_ << "digraph {\n";
  stream_ << "  rankdir=LR;\n";
  stream_ << "  label=\"" << escapeString("FA" /*TODO*/) << "\";\n";
}

void DotfileWriter::visitNode(int number, bool start, bool accept) {
  if (start) {
    stream_ << "    \"\" [shape=plaintext];\n";
    stream_ << "    node [shape=circle];\n";
    stream_ << "    \"\" -> " << stateLabelPrefix_ << number << ";\n";
  } else if (accept) {
    stream_ << "    node [shape=doublecircle]; " << stateLabelPrefix_ << number << ";\n";
  } else {
    stream_ << "    " << stateLabelPrefix_ << number << ";\n";
  }
}

void DotfileWriter::visitEdge(int from, int to, std::string_view label) {
  stream_ << "    " << stateLabelPrefix_ << from
          << " -> " << stateLabelPrefix_ << to;
  stream_ << "   [label=\"" << escapeString(label) << "\"]";
  stream_ << ";\n";
}

void DotfileWriter::end() {
  stream_ << "}\n";
}

} // namespace klex
