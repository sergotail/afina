#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <unordered_map>
#include <list>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
	namespace Backend {

		/**
		* # Map based implementation with global lock
		*
		*
		*/
		class MapBasedGlobalLockImpl : public Afina::Storage {
		public:
			MapBasedGlobalLockImpl(std::size_t const max_size = 1024) : 
				_max_size(max_size) {}
			~MapBasedGlobalLockImpl() {}

			// Implements Afina::Storage interface
			bool Put(std::string const & key, std::string const & value) override;

			// Implements Afina::Storage interface
			bool PutIfAbsent(std::string const & key, std::string const & value) override;

			// Implements Afina::Storage interface
			bool Set(std::string const & key, std::string const & value) override;

			// Implements Afina::Storage interface
			bool Delete(std::string const & key) override;

			// Implements Afina::Storage interface
			bool Get(std::string const & key, std::string & value) const override;

		private:
			mutable std::mutex _lock;
			std::size_t const _max_size;
			mutable std::list<std::pair<std::string, std::string>> _items_list;
			std::unordered_map<std::string, decltype(_items_list.begin())> _hash_table;
			void eraseLRU();
			bool isKeyExists(std::string const & key) const;
			void pushFront(std::string const & key, std::string const & value);
		};

	} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
