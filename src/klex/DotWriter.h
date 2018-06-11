// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/DotVisitor.h>
#include <klex/State.h>

#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <map>
#include <string_view>

namespace klex {

class DotWriter : public DotVisitor {
 public:
  explicit DotWriter(std::ostream& os, std::string stateLabelPrefix = "n")
      : ownedStream_{},
        stream_{os},
        stateLabelPrefix_{stateLabelPrefix}
  {}

  explicit DotWriter(const std::string& filename, std::string stateLabelPrefix = "n")
      : ownedStream_{std::make_unique<std::ofstream>(filename)},
        stream_{*ownedStream_.get()},
        stateLabelPrefix_{stateLabelPrefix}
  {}

 public:
  void start() override;
  void visitNode(int number, bool start, bool accept) override;
  void visitEdge(int from, int to, Symbol s) override;
  void endVisitEdge(int from, int to) override;
  void end() override;

 private:
  std::unique_ptr<std::ostream> ownedStream_;
  std::ostream& stream_;
  std::string stateLabelPrefix_;
  std::map<StateId /*target state*/, std::vector<Symbol> /*transition symbols*/> transitionGroups_;
};

} // namespace klex
