#pragma once

#include <string_view>
#include <klex/regular/State.h>

namespace klex::regular {

class DotVisitor {
 public:
  virtual ~DotVisitor() {}

  virtual void start(StateId initialState) = 0;
  virtual void visitNode(StateId number, bool start, bool accept) = 0;
  virtual void visitEdge(StateId from, StateId to, Symbol s) = 0;
  virtual void endVisitEdge(StateId from, StateId to) = 0;
  virtual void end() = 0;
};

} // namespace klex::regular
