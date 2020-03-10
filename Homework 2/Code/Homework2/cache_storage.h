#pragma once

#include "storage.h"

#define NOMINMAX
#include <scalable-cache.h>

template<typename E>
class CacheRecordStorage : public RecordStorage<E> {
private:
	typedef tstarling::ThreadSafeScalableCache<Board, Record<E>> CacheMap;
	mutable CacheMap m;
	const size_t sections;
	const size_t capacity;

protected:
	void safe_insert(const Board& standardBoard, const Record<E>& record) override {
		m.insert(standardBoard, record);
	}

	bool safe_lookup(const Board standardBoard, Record<E>& record) const override {
		typename CacheMap::ConstAccessor accessor;
		auto find = m.find(accessor, standardBoard);
		if (find) {
			record = *accessor;
			return true;
		} else {
			return false;
		}
	}

	void clear() override {
		std::unique_lock<recursive_mutex> l(this->lock);
		m.clear();
	}

	void serialize(ofstream& file) override {
		this->write_size(file, 0);
	}

	void deserialize(ifstream& file) override {
		m.clear();
		auto size = this->read_size(file);
		for (auto i = 0ULL; i < std::min(size, capacity); i++) {
			auto elem = this->read_record(file);
			m.insert(elem.first, elem.second);
		}
	}

public:
	CacheRecordStorage(const size_t _capacity, const size_t _sections) : capacity(_capacity), sections(_sections), m(_capacity, _sections){}

	RecordStorageType Type() const override {
		return RecordStorageType::Cache;
	}

	size_t size() const override {
		return m.size();
	}
};