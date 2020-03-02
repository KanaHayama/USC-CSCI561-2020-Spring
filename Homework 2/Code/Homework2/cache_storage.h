#pragma once

#include "storage.h"

#define NOMINMAX
#include <scalable-cache.h>

class CacheRecordStorage : public RecordStorage {
private:
	mutable tstarling::ThreadSafeScalableCache<Board, Record> m;
	const size_t capacity;

protected:
	void safe_insert(const Board& standardBoard, const Record& record) override {
		m.insert(standardBoard, record);
	}

	bool safe_lookup(const Board standardBoard, Record& record) const override {
		decltype(m)::ConstAccessor accessor;
		auto find = m.find(accessor, standardBoard);
		if (find) {
			record = *accessor;
			return true;
		} else {
			return false;
		}
	}

	void clear() override {
		std::unique_lock<recursive_mutex> l(lock);
		m.clear();
	}

	void serialize(ofstream& file) override {
		write_size(file, 0);
	}

	void deserialize(ifstream& file) override {
		m.clear();
		auto size = read_size(file);
		for (auto i = 0ULL; i < std::min(size, capacity / 2); i++) {
			auto elem = read_record(file);
			m.insert(elem.first, elem.second);
		}
	}

public:
	CacheRecordStorage(const size_t _capacity) : capacity(_capacity), m(_capacity){}

	RecordStorageType Type() const override {
		return RecordStorageType::Cache;
	}

	size_t size() const override {
		return m.size();
	}
};