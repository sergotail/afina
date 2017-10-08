#ifndef  AFINA_ALLOCATOR_MEMORY_H
#define  AFINA_ALLOCATOR_MEMORY_H

#include <afina/allocator/Error.h>
#include <afina/allocator/AllocUtils.h>

namespace Afina {
	namespace Allocator {

		class MemoryBlockInfo {
			// deprecated implementation of block info
			/*
			static uint64_t const _usage_mask = static_cast<uint64_t>(1) << (sizeof(uint64_t) * CHAR_BIT - 1);
			uint64_t _block_info;
			*/
			//static size_t const _alignment_size = sizeof(size_t);
			// block usage flag: 0 - free, 1 - allocated
			uint64_t _usage_flag : 1;
			uint64_t _block_size : sizeof(uint64_t) * CHAR_BIT - 1;
			static size_t const _block_info_size = sizeof(uint64_t);
			static uint64_t const _max_block_size = std::numeric_limits<uint64_t>::max() / 2;
		protected:
			MemoryBlockInfo() = delete;
			MemoryBlockInfo(MemoryBlockInfo const &) = delete;
			MemoryBlockInfo & operator =(MemoryBlockInfo const &) = delete;
			MemoryBlockInfo(MemoryBlockInfo &&) = delete;
			MemoryBlockInfo & operator =(MemoryBlockInfo &&) = delete;
		public:
			enum MemoryBlockUsage { FREE, ALLOCATED };
			size_t getBlockSize() const;
			void setBlockSize(size_t const new_size);
			MemoryBlockUsage getBlockUsage() const;
			void setBlockUsage(MemoryBlockUsage const block_usage);
			MemoryBlockInfo * getBlockEnd();
			static uint64_t getMaxBlockSize();
			static size_t  getBlockInfoSize();
		};

		class LinkedMemoryBlockInfo : public MemoryBlockInfo {
			LinkedMemoryBlockInfo * _next = nullptr;
			static size_t const _block_info_size;
		public:
			LinkedMemoryBlockInfo * getNext() const;
			void setNext(LinkedMemoryBlockInfo * const new_next);
			void setBlockSize(size_t const new_size);
			static size_t getBlockInfoSize();
		};

		// class for ordered by address forward list of LinkedMemoryBlockInfo
		// with auto blocks coalesce (merging) support
		// auto-compute sum of blocks size (assume that we don`t change blocks sizes after inserting to a list)
		class LinkedMemoryBlockInfoOrderedForwardList {
			LinkedMemoryBlockInfo * _head;
			size_t _length;
			bool _auto_coalesce;
			LinkedMemoryBlockInfo * insertAfter(LinkedMemoryBlockInfo * const prev_elem, LinkedMemoryBlockInfo * const new_elem);
			LinkedMemoryBlockInfo * splitAfter(LinkedMemoryBlockInfo * const prev_elem, LinkedMemoryBlockInfo * const split_elem, size_t const first_size);
			LinkedMemoryBlockInfo * removeAfter(LinkedMemoryBlockInfo * const prev_elem, LinkedMemoryBlockInfo * const del_elem);
			LinkedMemoryBlockInfo * coalesce(LinkedMemoryBlockInfo * const prev_elem, LinkedMemoryBlockInfo * const new_elem);
		public:
			LinkedMemoryBlockInfoOrderedForwardList(bool const auto_coalesce = true);
			LinkedMemoryBlockInfo * getHead() const;
			size_t getLength() const;
			bool getAutoCoalesce();
			void setAutoCoalesce(bool const auto_coalesce);
			bool findByPtr(void * const ptr);
			LinkedMemoryBlockInfo * insert(LinkedMemoryBlockInfo * const new_elem, LinkedMemoryBlockInfo * const prev_elem = nullptr, bool const insert_after = false);
			LinkedMemoryBlockInfo * split(LinkedMemoryBlockInfo * const split_elem, size_t const first_size, LinkedMemoryBlockInfo * const prev_elem = nullptr, bool const split_after = false);
			LinkedMemoryBlockInfo * remove(LinkedMemoryBlockInfo * const del_elem, LinkedMemoryBlockInfo * const prev_elem = nullptr, bool const remove_after = false);
			void clear();
		};

	} // namespace Allocator
} // namespace Afina
#endif // AFINA_ALLOCATOR_MEMORY_H
