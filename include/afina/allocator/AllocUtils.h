#ifndef AFINA_ALLOCATOR_ALLOCUTILS_H
#define AFINA_ALLOCATOR_ALLOCUTILS_H

#include <cstddef>
#include <limits>
#include <string>
#include <climits>

namespace Afina {
	namespace Allocator {

		class AllocUtils {
		protected:
			AllocUtils() = delete;
			AllocUtils(AllocUtils const &) = delete;
			AllocUtils & operator =(AllocUtils const &) = delete;
			AllocUtils(AllocUtils &&) = delete;
			AllocUtils & operator =(AllocUtils &&) = delete;
		public:
			typedef unsigned char byte;
			static void * addOffset(void * const ptr, size_t const offset, bool const is_positive = true);
			static ptrdiff_t ptrDiffInBytes(void * const left, void * const right);
		};

	} // namespace Allocator
} // namespace Afina
#endif //AFINA_ALLOCATOR_ALLOCUTILS_H
