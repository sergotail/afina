#include "MapBasedGlobalLockImpl.h"

namespace Afina {
	namespace Backend {

		// See MapBasedGlobalLockImpl.h
		bool MapBasedGlobalLockImpl::Put(std::string const & key, std::string const & value) {
			Delete(key);
			pushFront(key, value);
			eraseLRU();
			return true;
		}

		// See MapBasedGlobalLockImpl.h
		bool MapBasedGlobalLockImpl::PutIfAbsent(std::string const & key, std::string const & value) {
			if (!isKeyExists(key)) {
				pushFront(key, value);
				eraseLRU();
				return true;
			}
			return false;
		}

		// See MapBasedGlobalLockImpl.h
		bool MapBasedGlobalLockImpl::Set(std::string const & key, std::string const & value) {
			if (isKeyExists(key)) {
				Delete(key);
				pushFront(key, value);
				return true;
			}
			return false;
		}

		// See MapBasedGlobalLockImpl.h
		bool MapBasedGlobalLockImpl::Delete(std::string const & key) {
			std::unique_lock<std::mutex> guard(_lock);
			auto it = _hash_table.find(key);
			if (it != _hash_table.end()) {
				_items_list.erase(it->second);
				_hash_table.erase(it);
				return true;
			}
			return false;
		}

		// See MapBasedGlobalLockImpl.h
		bool MapBasedGlobalLockImpl::Get(std::string const & key, std::string & value) const {
			std::unique_lock<std::mutex> guard(*const_cast<std::mutex *>(&_lock));
			auto it = _hash_table.find(key);
			if (it != _hash_table.end()) {
				_items_list.splice(_items_list.begin(), _items_list, it->second);
				value = it->second->second;
				return true;
			}
			return false;
		}

	} // namespace Backend
} // namespace Afina
