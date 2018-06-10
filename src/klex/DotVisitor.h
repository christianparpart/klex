#pragma once

#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>

namespace klex {

class DotVisitor {
 public:
  virtual ~DotVisitor() {}

  virtual void start() = 0;
  virtual void visitNode(int number, bool start, bool accept) = 0;
  virtual void visitEdge(int from, int to, std::string_view edge_text) = 0;
  virtual void end() = 0;
};

class DotfileWriter : public DotVisitor {
 public:
  explicit DotfileWriter(std::ostream& os, std::string stateLabelPrefix = "n")
      : ownedStream_{},
        stream_{os},
        stateLabelPrefix_{stateLabelPrefix}
  {}

  explicit DotfileWriter(const std::string& filename, std::string stateLabelPrefix = "n")
      : ownedStream_{std::make_unique<std::ofstream>(filename)},
        stream_{*ownedStream_.get()},
        stateLabelPrefix_{stateLabelPrefix}
  {}

 public:
  void start() override;
  void visitNode(int number, bool start, bool accept) override;
  void visitEdge(int from, int to, std::string_view edge_text) override;
  void end() override;

 private:
  std::unique_ptr<std::ostream> ownedStream_;
  std::ostream& stream_;
  std::string stateLabelPrefix_;
};

} // namespace klex
