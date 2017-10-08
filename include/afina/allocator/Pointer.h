#ifndef AFINA_ALLOCATOR_POINTER_H
#define AFINA_ALLOCATOR_POINTER_H

#include <cstddef>

namespace Afina {
	namespace Allocator {
		
		// Forward declaration. Do not include real class definition
		// to avoid expensive macros calculations and increase compile speed
		class MemoryBlockInfo;
		class Simple;
		

		class Pointer {
			friend class Simple;
			MemoryBlockInfo ** _ptr;
			static std::size_t const _block_info_size;
		public:
			Pointer(MemoryBlockInfo ** const ptr = nullptr);
			Pointer(Pointer const & other);
			//Pointer(Pointer &&);
			Pointer & operator =(Pointer const & right);
			//Pointer & operator =(Pointer &&);
			void * get() const;
			~Pointer();
		};

	} // namespace Allocator
} // namespace Afina

#endif // AFINA_ALLOCATOR_POINTER_H
