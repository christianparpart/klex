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
