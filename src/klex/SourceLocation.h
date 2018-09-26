#pragma once

#include <string>

namespace klex {

struct SourceLocation {
	std::string filename;
	size_t offset;
	size_t count;

	int compare(const SourceLocation& other) const noexcept
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
