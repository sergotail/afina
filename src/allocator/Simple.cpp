#include <afina/allocator/Simple.h>
#include <afina/allocator/Pointer.h>


namespace Afina {
	namespace Allocator {

		using std::size_t;
		using std::string;

		Simple::Simple(void * const base, size_t const size) :
			_base(base),
			_base_len(size),
			_min_block_size(std::max(MemoryBlockInfo::getBlockInfoSize(), LinkedMemoryBlockInfo::getBlockInfoSize())),
			_block_ptr_size(sizeof(MemoryBlockInfo **)),
			_allocated_space(0),
			_ptr_area_has_spaces(false),
			_ptr_area_begin(nullptr),
			_ptr_area_end(nullptr),
			_last_used_ptr(nullptr) {
			LinkedMemoryBlockInfo * free_block = static_cast<LinkedMemoryBlockInfo *>(_base);
			free_block->setNext(nullptr);
			free_block->setBlockUsage(MemoryBlockInfo::MemoryBlockUsage::FREE);
			free_block->setBlockSize(size);
			_free_blocks_list.insert(free_block);
			_last_block = free_block;
		}
		
		Pointer Simple::alloc(size_t const N) {
			if (!N) {
				return Pointer();
			}
			size_t block_size = std::max(N + MemoryBlockInfo::getBlockInfoSize(), _min_block_size);
			checkAvailableSpace(block_size);
			MemoryBlockInfo ** mem_block_ptr = putMemoryBlockInfoPointer();
			LinkedMemoryBlockInfo * prev_free_block;
			LinkedMemoryBlockInfo * found_free_block;
			findFreeBlock(block_size, prev_free_block, found_free_block);
			MemoryBlockInfo * mem_block;
			putMemoryBlockInfo(block_size, prev_free_block, found_free_block, mem_block);
			*mem_block_ptr = mem_block;
			return Pointer(mem_block_ptr);
		}
		
		void Simple::realloc(Pointer & p, size_t const N) {
			if (!N) {
				free(p);
				return;
			}
			if (p._ptr == nullptr) {
				p = alloc(N);
			}
			MemoryBlockInfo * mem_block = *(p._ptr);
			if (mem_block == nullptr) {
				throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot realloc freed block"));
			}
			size_t const old_size = mem_block->getBlockSize() - mem_block->getBlockInfoSize();
			size_t const tmp_size = N + MemoryBlockInfo::getBlockInfoSize();
			size_t const size_diff = std::max(N, old_size) - std::min(N, old_size);
			if (!size_diff) {
				return;
			}
			if (N > old_size) {
				checkAvailableSpace(size_diff, false);
				MemoryBlockInfo * const prev_block = findPrevBlock(mem_block);
				MemoryBlockInfo * const next_block = mem_block == _last_block ? nullptr : static_cast<MemoryBlockInfo *>(AllocUtils::addOffset(mem_block, mem_block->getBlockSize()));
				// choose prev or next block for realloc, or none of them
				bool const next_block_fits = next_block != nullptr && next_block->getBlockUsage() == MemoryBlockInfo::MemoryBlockUsage::FREE &&
					next_block->getBlockSize() >= size_diff;
				bool const prev_block_fits = prev_block != nullptr && prev_block->getBlockUsage() == MemoryBlockInfo::MemoryBlockUsage::FREE &&
					prev_block->getBlockSize() >= size_diff;
				LinkedMemoryBlockInfo * fit_block = nullptr;
				if (!next_block_fits && prev_block_fits) {
					fit_block = static_cast<LinkedMemoryBlockInfo *>(prev_block);
				}
				else if (next_block_fits && !prev_block_fits) {
					fit_block = static_cast<LinkedMemoryBlockInfo *>(next_block);
				}
				else if (next_block_fits && prev_block_fits) {
					fit_block = next_block->getBlockSize() <= prev_block->getBlockSize() ? 
						static_cast<LinkedMemoryBlockInfo *>(next_block) : 
						static_cast<LinkedMemoryBlockInfo *>(prev_block);
				}
				if (fit_block != nullptr) {
					if (fit_block->getBlockSize() >= fit_block->getBlockInfoSize() + size_diff) {
						mem_block->setBlockSize(mem_block->getBlockSize() + size_diff);
						if (fit_block == next_block) {
							LinkedMemoryBlockInfo * new_free_block = static_cast<LinkedMemoryBlockInfo *>(AllocUtils::addOffset(fit_block, size_diff));
							new_free_block->setBlockUsage(MemoryBlockInfo::MemoryBlockUsage::FREE);
							new_free_block->setBlockSize(fit_block->getBlockSize() - size_diff);
							LinkedMemoryBlockInfo * prev_free_block = _free_blocks_list.remove(fit_block);
							_free_blocks_list.insert(new_free_block, prev_free_block, true);				
							if (fit_block == _last_block) {
								_last_block = new_free_block;
							}
						}
						else {
							fit_block->setBlockSize(fit_block->getBlockSize() - size_diff);
							MemoryBlockInfo * dst = static_cast<MemoryBlockInfo *>(AllocUtils::addOffset(fit_block, fit_block->getBlockSize()));
							moveMemoryBlock(dst, mem_block, true);
							*(p._ptr) = dst;
							if (mem_block == _last_block) {
								_last_block = dst;
							}
						}
						_allocated_space += size_diff;
					}
					else {
						_free_blocks_list.remove(fit_block);
						mem_block->setBlockSize(mem_block->getBlockSize() + fit_block->getBlockSize());
						if (fit_block == next_block) {		
							if (fit_block == _last_block) {
								_last_block = mem_block;
							}
						}
						else {
							moveMemoryBlock(fit_block, mem_block, true);
							*(p._ptr) = fit_block;
							if (mem_block == _last_block) {
								_last_block = fit_block;
							}
						}
						_allocated_space += fit_block->getBlockSize();
					}
				}
				else {
					LinkedMemoryBlockInfo * prev_free_block;
					LinkedMemoryBlockInfo * found_free_block;
					size_t block_size = std::max(tmp_size, _min_block_size);
					findFreeBlock(block_size, prev_free_block, found_free_block);
					MemoryBlockInfo * new_mem_block;
					putMemoryBlockInfo(block_size, prev_free_block, found_free_block, new_mem_block);
					mem_block->setBlockSize(mem_block->getBlockSize() + size_diff);
					moveMemoryBlock(new_mem_block, mem_block);
					*(p._ptr) = new_mem_block;
				}
			}
			else {
				if (size_diff >= _min_block_size && tmp_size >= _min_block_size) {
					// change block size and create new free block
					mem_block->setBlockSize(tmp_size);
					LinkedMemoryBlockInfo * new_free_block = static_cast<LinkedMemoryBlockInfo *>(AllocUtils::addOffset(mem_block, tmp_size));
					new_free_block->setBlockUsage(MemoryBlockInfo::MemoryBlockUsage::FREE);
					new_free_block->setBlockSize(size_diff);
					_free_blocks_list.insert(new_free_block);
					if (mem_block == _last_block) {
						_last_block = new_free_block;
					}
					_allocated_space -= size_diff;
				}
			}
		}

		void Simple::free(Pointer & p) {
			if (p._ptr == nullptr) {
				return;
			}
			LinkedMemoryBlockInfo * mem_block = static_cast<LinkedMemoryBlockInfo *>(*p._ptr);
			delMemoryBlockInfoPointer(p._ptr);
			putLinkedMemoryBlockInfo(mem_block);
			p._ptr = nullptr;
		}

		void Simple::defrag() {
			// defrag memory space
			MemoryBlockInfo * cur_block = static_cast<MemoryBlockInfo *>(_base);
			void * end_ptr = _ptr_area_end != nullptr ? _ptr_area_end : AllocUtils::addOffset(_base, _base_len);
			void * move_dst = cur_block;
			while (cur_block <= _last_block) {
				size_t cur_block_size = cur_block->getBlockSize();
				if (cur_block->getBlockUsage() == MemoryBlockInfo::MemoryBlockUsage::ALLOCATED) {
					if (move_dst != cur_block) {
						moveMemoryBlock(move_dst, cur_block, true);
						MemoryBlockInfo ** ptr = findMemoryBlockInfoPointer(cur_block);
						if (ptr == nullptr) {
							throw AllocError(AllocErrorType::MemoryBlockUsage, string("Canot find block pointer while defrag"));
						}
						*ptr = static_cast<MemoryBlockInfo *>(move_dst);
					}
					move_dst = AllocUtils::addOffset(move_dst, cur_block_size);
				}
				cur_block = static_cast<MemoryBlockInfo *>(AllocUtils::addOffset(cur_block, cur_block_size));
			}
			// defrag ptr area
			size_t ptr_area_free_space = 0;
			if (_last_used_ptr != nullptr) {
				ptr_area_free_space = AllocUtils::ptrDiffInBytes(_last_used_ptr, _ptr_area_end);
			}
			else if (_ptr_area_begin != nullptr) {
				ptr_area_free_space = AllocUtils::ptrDiffInBytes(_ptr_area_begin, _ptr_area_end) + sizeof(MemoryBlockInfo**);
			}
			if (_free_blocks_list.getLength() != 0 || ptr_area_free_space >= LinkedMemoryBlockInfo::getBlockInfoSize()) {
				_ptr_area_end = _last_used_ptr;
				if (_ptr_area_end == nullptr) {
					_ptr_area_begin = nullptr;
				}
				void * new_blocks_end = _last_used_ptr != nullptr ? _last_used_ptr : AllocUtils::addOffset(_base, _base_len);
				// update free blocks list
				_free_blocks_list.clear();
				LinkedMemoryBlockInfo * free_block = static_cast<LinkedMemoryBlockInfo *>(move_dst);
				free_block->setBlockUsage(MemoryBlockInfo::MemoryBlockUsage::FREE);
				size_t free_block_size = AllocUtils::ptrDiffInBytes(new_blocks_end, move_dst);
				free_block->setBlockSize(free_block_size);
				_free_blocks_list.insert(free_block);
				_last_block = free_block;
			}
		}

		void Simple::checkAvailableSpace(size_t const required_size, bool const ptr_needed) {
			size_t ptr_area_min_size = (_last_used_ptr == nullptr) ? 0 : _block_ptr_size * (_ptr_area_begin - _last_used_ptr + 1);
			size_t required_ptr_size = _ptr_area_has_spaces || !ptr_needed ? 0 : _block_ptr_size;
			bool space_available = (_base_len > (_allocated_space + ptr_area_min_size + required_ptr_size));
			if (!space_available) {
				throw AllocError(AllocErrorType::NoMemory, string("Not enough memory"));
			}
			size_t max_available_space = _base_len - _allocated_space - ptr_area_min_size - required_ptr_size;
			if (max_available_space < required_size) {
				throw AllocError(AllocErrorType::NoMemory, string("Not enough memory"));
			}
		}

		// size here is required size (requested + tech)
		// calling this function we assume that memory is enough
		void Simple::findFreeBlock(size_t const size, LinkedMemoryBlockInfo * & prev_block, LinkedMemoryBlockInfo * & found_block) {

			LinkedMemoryBlockInfo * cur = _free_blocks_list.getHead();
			LinkedMemoryBlockInfo * prev = nullptr;
			while (cur != nullptr) {
				if (cur->getBlockSize() >= size) {
					break;
				}
				prev = cur;
				cur = cur->getNext();
			}
			if (cur == nullptr) {
				throw AllocError(AllocErrorType::DefragmentationNeeded,
					string("Cannot find free block of required size"));
			}
			prev_block = prev;
			found_block = cur;
		}

		MemoryBlockInfo * Simple::findPrevBlock(MemoryBlockInfo * const mem_block) {
			if (mem_block == nullptr) {
				throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot find prev block of nullptr"));
			}
			MemoryBlockInfo * cur_block = static_cast<MemoryBlockInfo *>(_base);
			MemoryBlockInfo * prev_block = nullptr;
			while (cur_block != mem_block) {
				prev_block = cur_block;
				cur_block = cur_block->getBlockEnd(); //static_cast<MemoryBlockInfo *>(AllocUtils::addOffset(cur_block, cur_block->getBlockSize()));
			}
			if (cur_block > mem_block) {
				throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot find prev block of specified block"));
			}
			return prev_block;
		}

		void Simple::putMemoryBlockInfo(size_t const size, LinkedMemoryBlockInfo * & prev_block, LinkedMemoryBlockInfo * & found_block, MemoryBlockInfo * & new_block) {
			// prepare free blocks list
			LinkedMemoryBlockInfo * tail = _free_blocks_list.split(found_block, size, prev_block, true);
			// update _last_block if needed and handle first splitted elem size
			if (tail != nullptr) {
				if (tail > _last_block) {
					_last_block = tail;
				}
			}
			// process res block
			new_block = static_cast<MemoryBlockInfo *>(found_block);
			new_block->setBlockUsage(MemoryBlockInfo::MemoryBlockUsage::ALLOCATED);
			new_block->setBlockSize(tail == nullptr ? found_block->getBlockSize() : size);
			// update _allocated_space
			_allocated_space += new_block->getBlockSize();
		}

		void Simple::putLinkedMemoryBlockInfo(MemoryBlockInfo * const mem_block) {
			_allocated_space -= mem_block->getBlockSize();
			LinkedMemoryBlockInfo * free_block = static_cast<LinkedMemoryBlockInfo *>(mem_block);
			free_block->setBlockUsage(MemoryBlockInfo::MemoryBlockUsage::FREE);
			LinkedMemoryBlockInfo * new_free_block = _free_blocks_list.insert(free_block);
			if (AllocUtils::addOffset(new_free_block, new_free_block->getBlockSize()) > _last_block) {
				_last_block = new_free_block;
			}
		}

		// assumes that other allocated blocks will not be affected after moving
		void * Simple::moveMemoryBlock(void * const dst, MemoryBlockInfo * const mem_block, bool const allow_overlap) {
			if (mem_block == nullptr || dst == nullptr) {
				throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot move block from/to nullptr"));
			}
			if (dst == mem_block) {
				return dst;
			}
			void * const dst_end = AllocUtils::addOffset(dst, mem_block->getBlockSize());
			size_t const diff_size = std::abs(AllocUtils::ptrDiffInBytes(mem_block, dst));
			if (dst < _base ||
				_ptr_area_end != nullptr && dst_end > _ptr_area_end ||
				_ptr_area_end == nullptr && dst_end > AllocUtils::addOffset(_base, _base_len)) {
				throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot move block outside of memory"));
			}
			if (diff_size < mem_block->getBlockSize()) {
				if (!allow_overlap) {
					throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot overlap block while moving it"));
			}
				size_t overlap_size = mem_block->getBlockSize() - diff_size;
				if (dst > mem_block) {
					std::memcpy(AllocUtils::addOffset(dst, diff_size), AllocUtils::addOffset(mem_block, diff_size), overlap_size);
					std::memcpy(dst, mem_block, diff_size);
				}
				else {
					std::memcpy(dst, mem_block, diff_size);
					std::memcpy(AllocUtils::addOffset(dst, diff_size), AllocUtils::addOffset(mem_block, diff_size), overlap_size);
				}
			}
			else {
				std::memcpy(dst, mem_block, mem_block->getBlockSize());
			}
			return dst;
		}

		void Simple::delMemoryBlockInfoPointer(MemoryBlockInfo ** const ptr) {
			if (ptr == nullptr) {
				throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot delete nullptr pointer to memory block"));
			}
			*ptr = nullptr;
			if (ptr == _last_used_ptr) {
				// reassign _last_used_ptr
				for (MemoryBlockInfo ** cur_ptr = _last_used_ptr; cur_ptr <= _ptr_area_begin; ++cur_ptr) {
					if (*cur_ptr != nullptr) {
						_last_used_ptr = cur_ptr;
						break;
					}
				}
				// if there no used ptrs left, _last_used_ptr assign to nullptr, has_spaces flag to false
				if (_last_used_ptr == ptr) {
					_last_used_ptr = nullptr;
					_ptr_area_has_spaces = false;
				}
			}
			else if (_last_used_ptr < ptr) {
				_ptr_area_has_spaces = true;
			}
		}

		MemoryBlockInfo ** Simple::putMemoryBlockInfoPointer() {
			MemoryBlockInfo ** res_ptr = nullptr;
			// if there is free spaces in ptr area, put ptr into first space and check if an spaces left
			// and return found unused ptr
			if (_ptr_area_has_spaces) {
				for (MemoryBlockInfo ** cur_ptr = _ptr_area_begin; cur_ptr > _last_used_ptr; --cur_ptr) {
					if (*cur_ptr == nullptr) {
						if (res_ptr == nullptr) {
							res_ptr = cur_ptr;
							_ptr_area_has_spaces = false;
						}
						else if (cur_ptr != res_ptr) {
							_ptr_area_has_spaces = true;
							break;
						}
					}
				}
				return res_ptr;
			}
			// else we have or empty ptr area, or no spaces in it, or whole ptr area is unused
			// if ptr area is not empty
			if (_ptr_area_begin != nullptr) {
				// ptr area is totally unused
				if (_last_used_ptr == nullptr) {
					_last_used_ptr = _ptr_area_begin;
					return _last_used_ptr;
				}
				// ptr area is not totally unused
				else if (_ptr_area_end < _last_used_ptr) {
					// shift last used ptr by 1 to left, and return it
					return --_last_used_ptr;
				}
			}
			// at now ptr area only is empty or totally used
			// expansion needed, check available memory for it
			if (_last_block->getBlockUsage() == MemoryBlockInfo::MemoryBlockUsage::ALLOCATED) {
				throw AllocError(AllocErrorType::DefragmentationNeeded,
					string("No memory for memory block pointer allocation, defragmentation is needed"));
			}
			else if (_last_block->getBlockSize() < LinkedMemoryBlockInfo::getBlockInfoSize() + _block_ptr_size) {
				throw AllocError(AllocErrorType::DefragmentationNeeded,
					string("No memory for memory block pointer allocation, defragmentation is needed"));
			}
			// at now we have totally used or empty ptr area and enough memory for its expansion
			_last_block->setBlockSize(_last_block->getBlockSize() - _block_ptr_size);
			res_ptr = static_cast<MemoryBlockInfo **>(AllocUtils::addOffset(_last_block, _last_block->getBlockSize()));
			*res_ptr = nullptr;
			_ptr_area_end = res_ptr;
			_last_used_ptr = res_ptr;
			if (_ptr_area_begin == nullptr) {
				_ptr_area_begin = res_ptr;
			}
			return res_ptr;
		}

		MemoryBlockInfo ** Simple::findMemoryBlockInfoPointer(MemoryBlockInfo * const mem_block) {
			if (_last_used_ptr == nullptr) {
				return nullptr;
			}
			for (MemoryBlockInfo ** cur_ptr = _ptr_area_begin; cur_ptr >= _last_used_ptr; --cur_ptr) {
				if (*cur_ptr == mem_block) {
					return cur_ptr;
				}
			}
			return nullptr;
		}

		string Simple::dump(string const & sep) const { 
			string blocks_info = sep;
			blocks_info += "Base address: " + std::to_string((size_t)_base) + ", ";
			blocks_info += "All memory size: " + std::to_string(_base_len) + ", ";
			blocks_info += "Allocated: " + std::to_string(_allocated_space) + "\n";
			
			blocks_info += "Free blocks: ";
			blocks_info += std::to_string(_free_blocks_list.getLength()) + " free blocks\n";
			LinkedMemoryBlockInfo * cur = _free_blocks_list.getHead();
			if (cur == nullptr) {
				blocks_info += "None\n";
			}
			else {
				while (cur != nullptr) {
					blocks_info += "Offset: " + std::to_string(AllocUtils::ptrDiffInBytes(cur, _base));
					blocks_info += " Size: " + std::to_string(cur->getBlockSize());
					blocks_info += " Usage: " + std::to_string(cur->getBlockUsage());
					blocks_info += " Is last: " + std::to_string((cur == _last_block));
					blocks_info += "\n";
					cur = cur->getNext();
				}
			}
			blocks_info += "\n";
			blocks_info += "All blocks:\n";
			MemoryBlockInfo * cur_2 = static_cast<MemoryBlockInfo *>(_base);
			if (cur_2 == nullptr) {
				blocks_info += "None\n";
			}
			else {
				void * blocks_end = _last_block ? AllocUtils::addOffset(_last_block, _last_block->getBlockSize()) : AllocUtils::addOffset(_base, _base_len);
				while (AllocUtils::ptrDiffInBytes(cur_2, _base) < AllocUtils::ptrDiffInBytes(blocks_end, _base)) {
					blocks_info += string("| ")
						+ "Offset: " + std::to_string(AllocUtils::ptrDiffInBytes(cur_2, _base))
						+ ", Size: " + std::to_string(cur_2->getBlockSize())
						+ ", Usage: " + std::to_string(cur_2->getBlockUsage())
						+ ", Is last: " + std::to_string(cur_2 == _last_block)
						+ " |";
					cur_2 = static_cast<MemoryBlockInfo *>(AllocUtils::addOffset(cur_2, cur_2->getBlockSize()));
				}
			}
			MemoryBlockInfo ** cur_3 = _ptr_area_end;
			if (cur_3 == nullptr) {
				blocks_info += "||";
			}
			else {
				while (cur_3 <= _ptr_area_begin) {
					blocks_info += string("| ")
						+ "Offset: " + std::to_string(AllocUtils::ptrDiffInBytes(cur_3, _base))
						+ ", Usage: " + std::to_string(*cur_3 != nullptr)
						+ " |";
					cur_3 = static_cast<MemoryBlockInfo **>(AllocUtils::addOffset(cur_3, _block_ptr_size));
				}
				blocks_info += "\n";
			}
			blocks_info += sep;
			return blocks_info;
		}

	} // namespace Allocator
} // namespace Afina
