// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <string>

namespace klex {

struct SourceLocation {
	std::string filename;
	size_t offset;
	size_t count;

	long long int compare(const SourceLocation& other) const noexcept
	{
		if (filename == other.filename)
			return offset - other.offset;
		else if (filename < other.filename)
			return -1;
		else
			return 1;
	}

	std::string source() const;

	bool operator==(const SourceLocation& other) const noexcept { return compare(other) == 0; }
	bool operator<=(const SourceLocation& other) const noexcept { return compare(other) <= 0; }
	bool operator>=(const SourceLocation& other) const noexcept { return compare(other) >= 0; }
	bool operator<(const SourceLocation& other) const noexcept { return compare(other) < 0; }
	bool operator>(const SourceLocation& other) const noexcept { return compare(other) > 0; }
};

}  // namespace klex
