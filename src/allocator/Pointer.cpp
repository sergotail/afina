#include <afina/allocator/Pointer.h>
#include <afina/allocator/Memory.h>

namespace Afina {
	namespace Allocator {

		using std::size_t;
		using std::string;
		
		size_t const Pointer::_block_info_size = MemoryBlockInfo::getBlockInfoSize();

		Pointer::Pointer(MemoryBlockInfo ** const ptr) :
			_ptr(ptr) {}

		Pointer::Pointer(Pointer const & other) {
			_ptr = other._ptr;
		}

		//Pointer(Pointer &&);

		Pointer & Pointer::operator =(Pointer const & right) {
			_ptr = right._ptr;
			return *this;
		}

		//Pointer & operator =(Pointer &&);

		void * Pointer::get() const {
			if (_ptr == nullptr) {
				return nullptr;
			}
			void * mem_block = static_cast<void *>(AllocUtils::addOffset(*_ptr, _block_info_size));
			return mem_block;
		}

		Pointer::~Pointer() {}
	} // namespace Allocator
} // namespace Afina
