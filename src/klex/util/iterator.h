// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/util/iterator-detail.h>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace klex::util {

template <typename Container>
inline auto reversed(Container&& c)
{
	if constexpr (std::is_reference<Container>::value)
		return detail::reversed<Container&>{std::forward<Container>(c)};
	else
		return detail::reversed<Container>{std::forward<Container>(c)};
}

template <typename Container>
inline auto indexed(const Container& c)
{
	return typename std::add_const<detail::indexed<const Container>>::type{c};
}

template <typename Container>
inline auto indexed(Container& c)
{
	return detail::indexed<Container>{c};
}

// template <typename Container>
// inline std::string join(const Container& container, const std::string& separator = ", ")
// {
// 	std::stringstream out;
//
// 	for (const auto && [i, v] : indexed(container))
// 		if (i)
// 			out << separator << v;
// 		else
// 			out << v;
//
// 	return out.str();
// }

template <typename T, typename Lambda>
inline auto filter(std::initializer_list<T> && c, Lambda proc)
{
	return typename std::add_const<detail::filter<const std::initializer_list<T>, Lambda>>::type{c, proc};
}

template <typename Container, typename Lambda>
inline auto filter(const Container& c, Lambda proc)
{
	return typename std::add_const<detail::filter<const Container, Lambda>>::type{c, proc};
}

template <typename Container, typename Lambda>
inline auto filter(Container& c, Lambda proc)
{
	return detail::filter<Container, Lambda>{c, proc};
}

}  // namespace klex::util
