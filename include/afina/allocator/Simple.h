#ifndef AFINA_ALLOCATOR_SIMPLE_H
#define AFINA_ALLOCATOR_SIMPLE_H

#include <cstring>

#include <afina/allocator/Memory.h>

namespace Afina {
	namespace Allocator {

		// Forward declaration. Do not include real class definition
		// to avoid expensive macros calculations and increase compile speed
		class Pointer;

		/**
		* Wraps given memory area and provides defagmentation allocator interface on
		* the top of it.
		*
		* Allocator instance doesn't take ownership of wrapped memmory and do not delete it
		* on destruction. So caller must take care of resource cleaup after allocator stop
		* being needs
		*/

		class Simple {
		public:
			Simple(void * const base, size_t const size);
			Pointer alloc(size_t const N);
			void realloc(Pointer & p, size_t const N);
			void free(Pointer & p);
			void defrag();
			std::string dump(std::string const & sep = std::string("\n")) const;
		private:
			void * const _base;
			size_t const _base_len;
			size_t const _min_block_size;
			size_t const _block_ptr_size;
			size_t _allocated_space;
			bool _ptr_area_has_spaces;
			MemoryBlockInfo ** _ptr_area_begin; // most right ptr in memory
			MemoryBlockInfo ** _ptr_area_end; // most left ptr in memory
			MemoryBlockInfo ** _last_used_ptr; // most left used ptr in ptr area
			MemoryBlockInfo * _last_block;
			LinkedMemoryBlockInfoOrderedForwardList _free_blocks_list;

			void checkAvailableSpace(size_t const requested_size, bool const ptr_needed = true);
			// size here is required size (requested + tech)
			// calling this function we assume that memory is enough
			void findFreeBlock(size_t const size, LinkedMemoryBlockInfo * & prev_block, LinkedMemoryBlockInfo * & found_block);
			MemoryBlockInfo * findPrevBlock(MemoryBlockInfo * const mem_block);
			void putMemoryBlockInfo(size_t const size, LinkedMemoryBlockInfo * & prev_block, LinkedMemoryBlockInfo * & found_block, MemoryBlockInfo * & new_block);
			void putLinkedMemoryBlockInfo(MemoryBlockInfo * const mem_block);
			void * moveMemoryBlock(void * const dst, MemoryBlockInfo * const mem_block, bool const allow_overlap = false);
			void delMemoryBlockInfoPointer(MemoryBlockInfo ** const ptr);

			// in delMemoryBlockInfoPointer function free spaces flag automatically must be updated
			MemoryBlockInfo ** putMemoryBlockInfoPointer();
			MemoryBlockInfo ** findMemoryBlockInfoPointer(MemoryBlockInfo * const mem_block);
		};

	} // namespace Allocator
} // namespace Afina
#endif // AFINA_ALLOCATOR_SIMPLE_H
