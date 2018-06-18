#include <klex/util/testing.h>
#include <klex/Symbols.h>

using klex::SymbolSet;

TEST(SymbolSet, s0) {
  SymbolSet s0;
  ASSERT_EQ(0, s0.size());
  ASSERT_TRUE(s0.empty());
}

TEST(SymbolSet, s1) {
  SymbolSet s1;

  // first add
  s1.insert('a');
  ASSERT_EQ(1, s1.size());
  ASSERT_FALSE(s1.empty());

  // overwrite
  s1.insert('a');
  ASSERT_EQ(1, s1.size());
  ASSERT_FALSE(s1.empty());
}

TEST(SymbolSet, initializer_list) {
  SymbolSet a {'a'};
  EXPECT_EQ(1, a.size());
  EXPECT_TRUE(a.contains('a'));

  SymbolSet s2 {'a', 'b', 'b', 'c'};
  EXPECT_EQ(3, s2.size());
  EXPECT_EQ("abc", s2.to_string());
}

TEST(SymbolSet, dot) {
  SymbolSet dot{SymbolSet::Dot};
  EXPECT_TRUE(dot.isDot());
  EXPECT_FALSE(dot.contains('\n'));
  EXPECT_EQ(".", dot.to_string());
}

TEST(SymbolSet, range) {
  SymbolSet r;
  r.insert(std::make_pair('a', 'f'));

  EXPECT_EQ(6, r.size());
  EXPECT_EQ("a-f", r.to_string());

  r.insert(std::make_pair('0', '9'));
  EXPECT_EQ(16, r.size());
  EXPECT_EQ("0-9a-f", r.to_string());
}

TEST(SymbolSet, fmt_format) {
  SymbolSet s;
  s.insert(std::make_pair('0', '9'));
  s.insert(std::make_pair('a', 'f'));

  EXPECT_EQ("0-9a-f", fmt::format("{}", s));
}

TEST(SymbolSet, hash_map) {
  SymbolSet s0;
  SymbolSet s1 { 'a' };
  SymbolSet s2 { 'a', 'b' };

  std::unordered_map<SymbolSet, int> map;
  map[s0] = 0;
  map[s1] = 1;
  map[s2] = 2;

  EXPECT_EQ(0, map[s0]);
  EXPECT_EQ(1, map[s1]);
  EXPECT_EQ(2, map[s2]);
}
