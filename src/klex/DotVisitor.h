#pragma once

#include <string_view>
#include <klex/State.h>

namespace klex {

class DotVisitor {
 public:
  virtual ~DotVisitor() {}

  virtual void start() = 0;
  virtual void visitNode(int number, bool start, bool accept) = 0;
  virtual void visitEdge(int from, int to, Symbol s) = 0;
  virtual void endVisitEdge(int from, int to) = 0;
  virtual void end() = 0;
};

} // namespace klex
