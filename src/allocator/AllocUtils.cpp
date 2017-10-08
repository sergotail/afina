#include <afina/allocator/AllocUtils.h>

namespace Afina {
	namespace Allocator {

		using std::size_t;
		using std::string;

		void * AllocUtils::addOffset(void * const ptr, size_t const offset, bool const is_positive) {
			byte * res = reinterpret_cast<byte *>(ptr);
			return is_positive ? res + offset : res - offset;
		}

		ptrdiff_t AllocUtils::ptrDiffInBytes(void * const left, void * const right) {
			return reinterpret_cast<byte *>(left) - reinterpret_cast<byte *>(right);
		}

	} // namespace Allocator
} // namespace Afina
