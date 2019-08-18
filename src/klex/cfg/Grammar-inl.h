// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <algorithm>

namespace klex::cfg {

inline bool _Symbols::empty() const noexcept
{
	return begin() == end();
}

inline size_t _Symbols::size() const noexcept
{
	return std::distance(begin(), end());
}

}  // namespace klex::cfg
