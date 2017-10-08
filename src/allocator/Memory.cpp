#include <afina/allocator/Memory.h>

namespace Afina {
	namespace Allocator {

		using std::size_t;
		using std::string;

		// deprecated implementation of block info
		// leading bit of _block_info is allocation flag (0 - free, 1 - allocated)
		// other bits of _block_info is block size
		/*
		MemoryBlockInfo::MemoryBlockInfo() :
		_block_info(0) {}

		size_t MemoryBlockInfo::getBlockSize() {
		return _block_info & ~_usage_mask;
		}

		void MemoryBlockInfo::setBlockSize(size_t const new_size) {

		size_t size_mod = new_size % _alignment_size;
		size_t aligned_size = new_size + (size_mod ? _alignment_size - size_mod : 0);
		if (new_size & _usage_mask) {
		throw AllocError(AllocErrorType::MemoryBlockUsage, string("Too big block have been requested"));
		}
		_block_info = (_block_info & _usage_mask) | new_size;
		}

		MemoryBlockInfo::MemoryBlockUsage MemoryBlockInfo::getBlockUsage() {
		return !(_block_info & _usage_mask) ? FREE : ALLOCATED;
		}

		void MemoryBlockInfo::setBlockUsage(MemoryBlockUsage const block_usage) {
		_block_info = (block_usage == FREE) ? _block_info & ~_usage_mask : _block_info | _usage_mask;
		}
		*/

		size_t MemoryBlockInfo::getBlockSize() const {
			return _block_size;
		}
		
		void MemoryBlockInfo::setBlockSize(size_t const new_size) {
			if (new_size > _max_block_size) {

				throw AllocError(AllocErrorType::MemoryBlockUsage,
					string("Specified block size is greater than max (" + std::to_string(_max_block_size) + " bytes) size"));
			}
			if (new_size < _block_info_size) {
				throw AllocError(AllocErrorType::MemoryBlockUsage,
					string("Specified block size is less than min (" + std::to_string(_block_info_size) + " bytes) size"));
			}
			_block_size = new_size;
		}
		
		MemoryBlockInfo::MemoryBlockUsage MemoryBlockInfo::getBlockUsage() const {
			return _usage_flag ? MemoryBlockUsage::ALLOCATED : MemoryBlockUsage::FREE;
		}
		
		void MemoryBlockInfo::setBlockUsage(MemoryBlockUsage const block_usage) {
			switch (block_usage)
			{
			case MemoryBlockUsage::FREE:
				_usage_flag = 0;
				break;
			case MemoryBlockUsage::ALLOCATED:
				_usage_flag = 1;
				break;
			default:
				throw AllocError(AllocErrorType::MemoryBlockUsage,
					string("Failed to set memory block usage: unknown usage type specified"));
			}
		}
		
		MemoryBlockInfo * MemoryBlockInfo::getBlockEnd() {
				return static_cast<MemoryBlockInfo *>(AllocUtils::addOffset(this, _block_size));
			}

		uint64_t MemoryBlockInfo::getMaxBlockSize() {
			return _max_block_size;
		}
		
		size_t  MemoryBlockInfo::getBlockInfoSize() {
			return _block_info_size;
		}


		size_t const LinkedMemoryBlockInfo::_block_info_size = MemoryBlockInfo::getBlockInfoSize() + sizeof(LinkedMemoryBlockInfo *);

		LinkedMemoryBlockInfo * LinkedMemoryBlockInfo::getNext() const {
			return _next;
		}
		
		void LinkedMemoryBlockInfo::setNext(LinkedMemoryBlockInfo * const new_next) {
			_next = new_next;
		}
		
		void LinkedMemoryBlockInfo::setBlockSize(size_t const new_size) {
			if (new_size < _block_info_size) {
				throw AllocError(AllocErrorType::MemoryBlockUsage,
					string("Specified block size is less than min (" + std::to_string(_block_info_size) + " bytes) size"));
			}
			MemoryBlockInfo::setBlockSize(new_size);
		}
		
		size_t LinkedMemoryBlockInfo::getBlockInfoSize() {
			return _block_info_size;
		}


		LinkedMemoryBlockInfo *  LinkedMemoryBlockInfoOrderedForwardList::insertAfter(LinkedMemoryBlockInfo * const prev_elem, LinkedMemoryBlockInfo * const new_elem) {
			if (prev_elem == nullptr) {
				if (_head == nullptr) {
					new_elem->setNext(nullptr);
				}
				else {
					new_elem->setNext(_head);
				}
				_head = new_elem;
			}
			else {
				if (prev_elem->getNext() == nullptr) {
					prev_elem->setNext(new_elem);
					new_elem->setNext(nullptr);
				}
				else {
					new_elem->setNext(prev_elem->getNext());
					prev_elem->setNext(new_elem);
				}
			}
			_length++;
			if (_auto_coalesce) {
				return coalesce(prev_elem, new_elem);
			}
			return new_elem;
		}
		
		LinkedMemoryBlockInfo * LinkedMemoryBlockInfoOrderedForwardList::splitAfter(LinkedMemoryBlockInfo * const prev_elem, LinkedMemoryBlockInfo * const split_elem, size_t const first_size) {
			if (first_size > split_elem->getBlockSize()) {
				throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot split block to bigger size"));
			}
			size_t rest_size = split_elem->getBlockSize() - first_size;
			remove(split_elem, prev_elem, true);
			if (first_size < LinkedMemoryBlockInfo::getBlockInfoSize() ||
				rest_size < LinkedMemoryBlockInfo::getBlockInfoSize()) {
				return nullptr;
			}
			LinkedMemoryBlockInfo * new_elem = static_cast<LinkedMemoryBlockInfo *>(AllocUtils::addOffset(split_elem, first_size));
			new_elem->setBlockUsage(MemoryBlockInfo::MemoryBlockUsage::FREE);
			new_elem->setBlockSize(rest_size);
			insertAfter(prev_elem, new_elem);
			return new_elem;
		}

		LinkedMemoryBlockInfo * LinkedMemoryBlockInfoOrderedForwardList::removeAfter(LinkedMemoryBlockInfo * const prev_elem, LinkedMemoryBlockInfo * const del_elem) {
			if (prev_elem == nullptr) {
				if (del_elem->getNext() == nullptr) {
					_head = nullptr;
				}
				else {
					_head = del_elem->getNext();
				}
			}
			else {
				prev_elem->setNext(del_elem->getNext());
			}
			_length--;
			return prev_elem;
		}
		
		LinkedMemoryBlockInfo * LinkedMemoryBlockInfoOrderedForwardList::coalesce(LinkedMemoryBlockInfo * const prev_elem, LinkedMemoryBlockInfo * const new_elem) {
			LinkedMemoryBlockInfo * start_elem = (prev_elem == nullptr) ? new_elem : prev_elem;
			LinkedMemoryBlockInfo * stop_elem = (new_elem->getNext() == nullptr) ? new_elem : new_elem->getNext();
			if (start_elem == stop_elem) {
				return new_elem;
			}
			LinkedMemoryBlockInfo * cur_elem = start_elem;
			LinkedMemoryBlockInfo * next_elem = start_elem->getNext();
			LinkedMemoryBlockInfo * res = new_elem;
			while (next_elem != nullptr && next_elem <= stop_elem) {
				if (AllocUtils::addOffset(cur_elem, cur_elem->getBlockSize()) == next_elem) {
					cur_elem->setBlockSize(cur_elem->getBlockSize() + next_elem->getBlockSize());
					cur_elem->setNext(next_elem->getNext());
					res = cur_elem;
					next_elem = cur_elem->getNext();
					_length--;
				}
				else {
					cur_elem = next_elem;
					next_elem = next_elem->getNext();
				}
			}
			return res;
		}

		LinkedMemoryBlockInfoOrderedForwardList::LinkedMemoryBlockInfoOrderedForwardList(bool const auto_coalesce) :
			_head(nullptr),
			_length(0),
			_auto_coalesce(auto_coalesce) {}
	
		LinkedMemoryBlockInfo * LinkedMemoryBlockInfoOrderedForwardList::getHead() const {
			return _head;
		}
		
		size_t LinkedMemoryBlockInfoOrderedForwardList::getLength() const {
			return _length;
		}

		bool LinkedMemoryBlockInfoOrderedForwardList::getAutoCoalesce() {
			return _auto_coalesce;
		}

		void LinkedMemoryBlockInfoOrderedForwardList::setAutoCoalesce(bool const auto_coalesce) {
			_auto_coalesce = auto_coalesce;
		}
		
		bool LinkedMemoryBlockInfoOrderedForwardList::findByPtr(void * const ptr) {
			LinkedMemoryBlockInfo * to_find = static_cast<LinkedMemoryBlockInfo *>(ptr);
			LinkedMemoryBlockInfo * cur_ptr = _head;
			while (cur_ptr != nullptr) {
				if (cur_ptr == to_find) {
					return true;
				}
			}
			return false;
		}

		LinkedMemoryBlockInfo * LinkedMemoryBlockInfoOrderedForwardList::insert(LinkedMemoryBlockInfo * const new_elem, LinkedMemoryBlockInfo * const prev_elem, bool const insert_after) {
			if (new_elem == nullptr) {
				throw AllocError(AllocErrorType::MemoryBlockUsage,
					string("Inserting linked block failed: cannot insert nullptr"));
			}
			LinkedMemoryBlockInfo * _prev_elem = nullptr;
			if (insert_after) {
				// check order
				if ((prev_elem != nullptr && new_elem <= prev_elem) ||
					(prev_elem == nullptr && _head != nullptr && new_elem >= _head) ||
					(prev_elem != nullptr && prev_elem->getNext() != nullptr && new_elem >= prev_elem->getNext())) {
					throw AllocError(AllocErrorType::MemoryBlockUsage, string("Insert into free block list must be ordered"));
				}
				_prev_elem = prev_elem;
			}
			else {
				LinkedMemoryBlockInfo * cur_elem = _head;
				
				while (cur_elem != nullptr) {
					if (new_elem == cur_elem) {
						throw AllocError(AllocErrorType::MemoryBlockUsage,
							string("Inserting free block failed: free block already exists at specified address"));
					}
					if (new_elem < cur_elem) {
						break;
					}
					_prev_elem = cur_elem;
					cur_elem = cur_elem->getNext();
				}
			}
			
			return insertAfter(_prev_elem, new_elem);
		}
		
		LinkedMemoryBlockInfo * LinkedMemoryBlockInfoOrderedForwardList::split(LinkedMemoryBlockInfo * const split_elem, size_t const first_size, LinkedMemoryBlockInfo * const prev_elem, bool const split_after) {
			if (split_elem == nullptr) {
				throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot split nullptr"));
			}
			LinkedMemoryBlockInfo * _prev_elem = nullptr;
			if (split_after) {
				if (prev_elem != nullptr && prev_elem->getNext() != split_elem) {
					throw AllocError(AllocErrorType::MemoryBlockUsage, string("Linked block to split not found after specified block"));
				}
				_prev_elem = prev_elem;
			}
			else {
				LinkedMemoryBlockInfo * cur_elem = _head;
				while (cur_elem != nullptr) {
					if (split_elem == cur_elem) {
						break;
					}
					if (split_elem < cur_elem) {
						throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot find linked block to split it"));
					}
					_prev_elem = cur_elem;
					cur_elem = cur_elem->getNext();
				}
			}
			return splitAfter(_prev_elem, split_elem, first_size);
		}
		
		LinkedMemoryBlockInfo * LinkedMemoryBlockInfoOrderedForwardList::remove(LinkedMemoryBlockInfo * const del_elem, LinkedMemoryBlockInfo * const prev_elem, bool const remove_after) {
			if (del_elem == nullptr) {
				throw AllocError(AllocErrorType::MemoryBlockUsage,
					string("Removing linked block failed: cannot remove nullptr"));
			}
			LinkedMemoryBlockInfo * _prev_elem = nullptr;
			if (remove_after) {
				if (prev_elem != nullptr && prev_elem->getNext() != del_elem) {
					throw AllocError(AllocErrorType::MemoryBlockUsage, string("Linked block to remove not found after specified block"));
				}
				_prev_elem = prev_elem;
			}
			else {
				LinkedMemoryBlockInfo * cur_elem = _head;
				while (cur_elem != nullptr) {
					if (del_elem == cur_elem) {
						break;
					}
					if (del_elem < cur_elem) {
						throw AllocError(AllocErrorType::MemoryBlockUsage, string("Cannot find linked block to remove it"));
					}
					_prev_elem = cur_elem;
					cur_elem = cur_elem->getNext();
				}
			}
			return removeAfter(_prev_elem, del_elem);
		}

		void LinkedMemoryBlockInfoOrderedForwardList::clear() {
			_head = nullptr;
			_length = 0;
		}
	} // namespace Allocator
} // namespace Afina
