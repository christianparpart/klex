// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include <utility>

namespace klex::util {

template <typename Container>
struct _reversed {
	const Container container;

	auto begin() { return container.crbegin(); }
	auto end() { return container.crend(); }
};

template <typename Container>
inline auto reversed(const Container& c)
{
	return _reversed<Container>{c};
}

template <typename Container>
struct _indexed {
	Container& container;

	struct iterator {
		typename Container::iterator iter;
		std::size_t index = 0;

		iterator& operator++()
		{
			++iter;
			++index;
			return *this;
		}

		iterator& operator++(int)
		{
			++*this;
			return *this;
		}

		auto operator*() const { return std::make_pair(index, *iter); }

		bool operator==(const iterator& rhs) const noexcept { return iter == rhs.iter; }
		bool operator!=(const iterator& rhs) const noexcept { return iter != rhs.iter; }
	};

	struct const_iterator {
		typename Container::const_iterator iter;
		std::size_t index = 0;

		const_iterator& operator++()
		{
			++iter;
			++index;
			return *this;
		}

		const_iterator& operator++(int)
		{
			++*this;
			return *this;
		}

		auto operator*() const { return std::make_pair(index, *iter); }

		bool operator==(const const_iterator& rhs) const noexcept { return iter == rhs.iter; }
		bool operator!=(const const_iterator& rhs) const noexcept { return iter != rhs.iter; }
	};

	auto begin() const
	{
		if constexpr (std::is_const<Container>::value)
			return const_iterator{container.cbegin()};
		else
			return iterator{container.begin()};
	}

	auto end() const
	{
		if constexpr (std::is_const<Container>::value)
			return const_iterator{container.cend()};
		else
			return iterator{container.end()};
	}
};

template <typename Container>
inline auto indexed(const Container& c)
{
	return typename std::add_const<_indexed<const Container>>::type{c};
}

template <typename Container>
inline auto indexed(Container& c)
{
	return _indexed<Container>{c};
}

}  // namespace klex::util
