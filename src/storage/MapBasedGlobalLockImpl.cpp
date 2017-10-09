#include "MapBasedGlobalLockImpl.h"

namespace Afina {
	namespace Backend {

		void MapBasedGlobalLockImpl::eraseLRU() {
			std::unique_lock<std::mutex> guard(_lock);
			while (_hash_table.size() > _max_size) {
				auto lru = _items_list.end();
				_hash_table.erase((--lru)->first);
				_items_list.pop_back();
			}
		}
		
		bool MapBasedGlobalLockImpl::isKeyExists(std::string const & key) const {
			std::unique_lock<std::mutex> guard(_lock);
			return _hash_table.count(key) > 0;
		}
		
		void MapBasedGlobalLockImpl::pushFront(std::string const & key, std::string const & value) {
			std::unique_lock<std::mutex> guard(_lock);
			_items_list.push_front(make_pair(key, value));
			_hash_table.insert(make_pair(key, _items_list.begin()));
		}

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
