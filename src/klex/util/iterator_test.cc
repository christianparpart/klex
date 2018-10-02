// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/util/iterator.h>
#include <klex/util/testing.h>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;
using namespace klex::util;

TEST(util_iterator_reversed, empty)
{
	const vector<int> v;
	auto x = reversed(v);
	auto i = begin(x);
	ASSERT_TRUE(i == end(x));
}

TEST(util_iterator_reversed, one)
{
	const vector<int> v { 1 };
	auto x = reversed(v);
	auto i = begin(x);
	ASSERT_EQ(1, *i);
	i++;
	ASSERT_TRUE(i == end(x));
}

TEST(util_iterator_reversed, many)
{
	const vector<int> v { 1, 2, 3 };
	auto x = reversed(v);
	auto i = begin(x);
	ASSERT_EQ(3, *i);
	i++;
	ASSERT_EQ(2, *i);
	i++;
	ASSERT_EQ(1, *i);
	i++;
	ASSERT_TRUE(i == end(x));
}

TEST(util_iterator_indexed, many_const)
{
	const vector<int> v { 10, 20, 30 };
	const auto x = indexed(v);
	static_assert(is_const<decltype(x)>::value);
	auto i = begin(x);

	ASSERT_EQ(0, (*i).first);
	ASSERT_EQ(10, (*i).second);
	i++;

	ASSERT_EQ(1, (*i).first);
	ASSERT_EQ(20, (*i).second);
	i++;

	ASSERT_EQ(2, (*i).first);
	ASSERT_EQ(30, (*i).second);
	i++;

	ASSERT_TRUE(i == end(x));
}

TEST(util_iterator_indexed, many)
{
	vector<string> v { "zero", "one", "two" };
	auto x = indexed(v);
	auto i = begin(x);

	ASSERT_EQ(0, (*i).first);
	ASSERT_EQ("zero", (*i).second);
	i++;

	ASSERT_EQ(1, (*i).first);
	ASSERT_EQ("one", (*i).second);
	i++;

	ASSERT_EQ(2, (*i).first);
	ASSERT_EQ("two", (*i).second);
	i++;

	ASSERT_TRUE(i == end(x));
}

TEST(util_iterator_indexed, range_based_for_loop)
{
	log("const:");
	const vector<int> v1 { 10, 20, 30 };
	for (const auto && [index, value] : indexed(v1))
		logf("index {}, value {}", index, value);

	log("non-const:");
	vector<int> v2 { 10, 20, 30 };
	for (const auto && [index, value] : indexed(v2))
		logf("index {}, value {}", index, value);
}

TEST(util_iterator_filter, for_range)
{
	const vector<int> nums = {1, 2, 3, 4};
	vector<int> odds;
	for (const int i : filter(nums, [](int x) { return x % 2 != 0; }))
		odds.push_back(i);

	ASSERT_EQ(2, odds.size());
	EXPECT_EQ(1, odds[0]);
	EXPECT_EQ(3, odds[1]);
}

TEST(util_iterator_filter, for_range_initializer_list)
{
	vector<int> odds;
	for (const int i : filter({1, 2, 3, 4}, [](int x) { return x % 2 != 0; }))
		odds.push_back(i);

	ASSERT_EQ(2, odds.size());
	EXPECT_EQ(1, odds[0]);
	EXPECT_EQ(3, odds[1]);
}
