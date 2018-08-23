// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//	 (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

template<typename Container>
struct _reversed {
	const Container& container;

	auto begin() { return container.crbegin(); }
	auto end() { return container.crend(); }
};

template<typename Container>
inline auto reversed(const Container& c) { return _reversed<Container>{c}; }

// TODO: create INDEXED_ITERATORS indexed(container); so something like that's possible:
//
// for (auto && [index, value] : container) { ... }
