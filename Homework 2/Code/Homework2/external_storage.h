#pragma once

#include "storage.h"

#define STXXL_VERBOSE_LEVEL -10

#include <stxxl/map>

struct RecordCompareLess {
	bool operator () (const Board& a, const Board& b) const {
		return a < b;
	}
	static Board max_value() {
		return 1ULL << STEP_SHIFT;
	}
};

template<typename E>
class ExternalRecordStorage : public RecordStorage<E> {
public:
	const static int NODE_BLOCK_SIZE = 4096;
	const static int LEAF_BLOCK_SIZE = 4096;
private:
	typedef stxxl::map<Board, E, RecordCompareLess, NODE_BLOCK_SIZE, LEAF_BLOCK_SIZE> external_map;

	std::unique_ptr<external_map> m = nullptr;
	UINT64 node_num_blocks;
	UINT64 leaf_num_blocks;

protected:

	void safe_insert(const Board& standardBoard, const E& record) override {
		std::unique_lock<std::recursive_mutex> l(this->lock);
		m->insert(std::make_pair(standardBoard, record));
	}

	bool safe_lookup(const Board standardBoard, E& record) const override {
		std::unique_lock<recursive_mutex> l(this->lock);
		auto find = m->find(standardBoard);
		if (find == m->end()) {
			return false;
		} else {
			record = find->second;
			return true;
		}
	}

	void clear() override {
		std::unique_lock<recursive_mutex> l(this->lock);
		m->clear();
	}

	void serialize(ofstream& file) override {
		m->enable_prefetching();
		this->write_size(file, m->size());
		std::for_each(m->begin(), m->end(), [&](const std::pair<Board, E>& elem) {this->write_record(file, elem.first, elem.second); });
		m->disable_prefetching();
	}

	void deserialize(ifstream& file) override {
		auto iters = RecordStorage<E>::RecordFileIterator::Create(file);
		m = std::make_unique<external_map>(iters.first, iters.second, external_map::node_block_type::raw_size * node_num_blocks, external_map::leaf_block_type::raw_size * leaf_num_blocks, true);
	}
public:

	ExternalRecordStorage(const UINT64& _node_num_blocks, const UINT64& _leaf_num_blocks) :node_num_blocks(_node_num_blocks), leaf_num_blocks(_leaf_num_blocks) {
		m = std::make_unique<external_map>(external_map::node_block_type::raw_size * node_num_blocks, external_map::leaf_block_type::raw_size * leaf_num_blocks);
	}

	RecordStorageType Type() const override {
		return RecordStorageType::External;
	}

	size_t size() const override {
		std::unique_lock<recursive_mutex> l(this->lock);
		return m->size();
	}
};