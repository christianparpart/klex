#include <algorithm>

namespace klex::cfg {

inline size_t _Symbols::size() const noexcept
{
	return std::distance(begin(), end());
}

}  // namespace klex::cfg
