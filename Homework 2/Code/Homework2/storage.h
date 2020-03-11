
#pragma once

#include "record.h"

#ifdef COLLECT_STORAGE_HIT_RATE
#include <atomic>
using std::atomic;
#endif

enum class RecordStorageType {
	Cache,
	Memory,
	External,
};

template <typename E>
class RecordStorage {
protected:
	mutable recursive_mutex lock;

	virtual void safe_insert(const Board& standardBoard, const Record<E>& record) = 0;
	virtual bool safe_lookup(const Board standardBoard, Record<E>& record) const = 0;
	virtual void clear() = 0;
	virtual void serialize(ofstream& file) = 0;
	virtual void deserialize(ifstream& file) = 0;
	

	inline static void write_size(ofstream& file, const UINT64& size) {
		file.write(reinterpret_cast<const char*>(&size), sizeof(size));
	}

	inline static void write_record(ofstream& file, const Board& standardBoard, const Record<E>& standardRecord) {
		file.write(reinterpret_cast<const char*>(&standardBoard), sizeof(standardBoard));//board
		file.write(reinterpret_cast<const char*>(&standardRecord), sizeof(standardRecord));
	}

	inline static UINT64 read_size(ifstream& file) {
		UINT64 size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size));
		return size;
	}

	inline static std::pair<Board, Record<E>> read_record(ifstream& file) {
		Board board;
		Record<E> record;
		file.read(reinterpret_cast<char*>(&board), sizeof(board));
		file.read(reinterpret_cast<char*>(&record), sizeof(record));
		return std::make_pair(board, record);
	}

#ifdef COLLECT_STORAGE_HIT_RATE
	atomic<UINT64> hit = 0;
	atomic<UINT64> total_query = 0;
#endif

public:
	bool EnableSerialize = true;

	virtual RecordStorageType Type() const = 0;

	virtual size_t size() const = 0;

	void Serialize(const string& filename) {
		if (!EnableSerialize) {
			return;
		}
		std::unique_lock<recursive_mutex> l(lock);
		unsigned long long s = size();
		if (s == 0) {
			return;
		}
		ofstream file(filename, std::ios::binary);
		assert(file.is_open());
		serialize(file);
		file.close();
		assert(!file.fail());
		cout << "Serialized " << filename << " with " << s << " entries" << endl;
	}

	void Deserialize(const string& filename) {
		if (!EnableSerialize) {
			return;
		}
		ifstream file(filename, std::ios::binary);
		if (!file.is_open()) {
			cout << filename << " not found, skip deserialization" << endl;
			return;
		}
		std::unique_lock<recursive_mutex> l(lock);
		deserialize(file);
		file.close();
		assert(!file.fail());
		cout << "Deserialized " << size() << " entries from " << filename << endl;
	}

	void Set(const Board board, const Record<E>& record) {
		auto temp = Isomorphism(board).IndexBoard(ActionMapping::EncodedToAction(record.BestAction));
		auto& standardBoard = temp.first;
		auto& standardAction = temp.second;
		auto encodedStandardAction = ActionMapping::ActionToEncoded(standardAction);
		safe_insert(standardBoard, Record<E>(encodedStandardAction, record.Eval));
		assert(standardAction == Action::Pass || BoardUtil::Empty(standardBoard, standardAction));
	}

	bool Get(const Board board, Record<E>& record) {
#ifdef COLLECT_STORAGE_HIT_RATE
		total_query++;
#endif
		auto iso = Isomorphism(board);
		auto standardBoard = iso.IndexBoard();
		Record<E> standardRecord;
		auto found = safe_lookup(standardBoard, standardRecord);
		if (!found) {
			return false;
		}
		auto standardAction = ActionMapping::EncodedToAction(standardRecord.BestAction);
		auto nonStandardAction = iso.ReverseAction(standardBoard, standardAction);
		auto encodedNonStandardAction = ActionMapping::ActionToEncoded(nonStandardAction);
		record = Record<E>(encodedNonStandardAction, standardRecord.Eval);
#ifdef SEARCH_MODE
		assert(record.Eval.Validate());
#endif
		assert(nonStandardAction == Action::Pass || BoardUtil::Empty(board, nonStandardAction));
#ifdef COLLECT_STORAGE_HIT_RATE
		hit++;
#endif
		return true;
	}

	void Clear() {
#ifdef COLLECT_STORAGE_HIT_RATE
		ClearHitRate();
#endif
		clear();
	}

#ifdef COLLECT_STORAGE_HIT_RATE
	void ClearHitRate() {
		hit = 0;
		total_query = 0;
	}

	double HitRate() const {
		return total_query == 0 ? std::numeric_limits<double>::quiet_NaN() : double(hit) / total_query;
	}
#endif

	class RecordFileIterator {
	private:
		typedef long long ssize_t;
		ifstream* file;
		ssize_t index;
		std::streampos begin;
		std::pair<decltype(index), std::pair<Board, Record<E>>> cache = std::make_pair(-1, std::pair<Board, Record<E>>());

		explicit RecordFileIterator(ifstream* _file, const ssize_t _index, const std::streampos _begin) : file(_file), index(_index), begin(_begin) {}

	public:
		typedef std::pair<Board, Record<E>> value_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::pair<Board, Record<E>>* pointer;
		typedef std::pair<Board, Record<E>>& reference;
		typedef std::input_iterator_tag iterator_category;

		static std::pair<RecordFileIterator, RecordFileIterator> Create(ifstream& file) {
			file.seekg(0);
			auto size = read_size(file);
			auto begin = file.tellg();
			return std::make_pair(RecordFileIterator(&file, 0, begin), RecordFileIterator(nullptr, size, begin));
		}

		value_type operator * () {
			if (cache.first == index) {
				return cache.second;
			}
			auto offset = static_cast<int>(begin) + index * (sizeof(Board) + sizeof(Record<E>));
			file->seekg(offset, ifstream::beg);
			auto result = read_record(*file);
			cache = std::make_pair(index, result);
			return result;
		}

		std::unique_ptr<value_type> operator -> () {
			auto result = std::unique_ptr<value_type>(new value_type());
			*result = **this;
			return result;
		}

		bool operator == (const RecordFileIterator& other) const {
			return index == other.index;

		}
		bool operator != (const RecordFileIterator& other) const {
			return !(*this == other);
		}

		RecordFileIterator operator ++ (int) {
			auto result = *this;
			++* this;
			return result;
		}

		RecordFileIterator& operator ++ (void) {
			index++;
			return *this;
		}
	};
};

#ifndef SEARCH_MODE
template <typename E>
class MemoryRecordStorage : public RecordStorage<E> {
private:
	map<Board, Record<E>> m;
protected:

	void safe_insert(const Board& standardBoard, const Record<E>& record) override {
		std::unique_lock<std::recursive_mutex> l(this->lock);
		m.emplace(standardBoard, record);
	}

	bool safe_lookup(const Board standardBoard, Record<E>& record) const override {
		auto find = m.find(standardBoard);
		if (find == m.end()) {
			std::unique_lock<recursive_mutex> l(this->lock);
			find = m.find(standardBoard);
			if (find == m.end()) {
				return false;//in multi thread context, search may duplicate
			}
		}
		record = find->second;
		return true;
	}

	void serialize(ofstream& file) override {
		this->write_size(file, m.size());
		std::for_each(m.begin(), m.end(), [&](const std::pair<Board, Record<E>>& elem) {this->write_record(file, elem.first, elem.second); });
	}

	void deserialize(ifstream& file) override {
		m.clear();
		auto size = this->read_size(file);
		for (auto i = 0ULL; i < size; i++) {
			auto elem = this->read_record(file);
			m.emplace(elem.first, elem.second);
		}
	}
public:
	RecordStorageType Type() const override {
		return RecordStorageType::Memory;
	}

	size_t size() const override {
		return m.size();
	}

	void clear() override {
		std::unique_lock<recursive_mutex> l(this->lock);
		m.clear();
	}
};
#else

#define NOMINMAX
#define TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS 1
#include <tbb/concurrent_map.h>
using tbb::concurrent_map;

template <typename E>
class MemoryRecordStorage : public RecordStorage<E> {
private:
	tbb::concurrent_map<Board, Record<E>> m;

protected:

	void safe_insert(const Board& standardBoard, const Record<E>& record) override {
		m.emplace(standardBoard, record);
	}

	bool safe_lookup(const Board standardBoard, Record<E>& record) const override {
		auto find = m.find(standardBoard);
		if (find == m.end()) {
			return false;
		} else {
			record = find->second;
			return true;
		}
	}

	void serialize(ofstream& file) override {
		this->write_size(file, m.size());
		std::for_each(m.begin(), m.end(), [&](const std::pair<Board, Record<E>>& elem) {this->write_record(file, elem.first, elem.second); });
	}

	void deserialize(ifstream& file) override {
		m.clear();
		auto size = this->read_size(file);
		for (auto i = 0ULL; i < size; i++) {
			auto elem = this->read_record(file);
			m.emplace(elem.first, elem.second);
		}
	}
public:
	RecordStorageType Type() const override {
		return RecordStorageType::Memory;
	}

	size_t size() const override {
		return m.size();
	}

	void clear() override {
		std::unique_lock<recursive_mutex> l(this->lock);
		m.clear();
	}
};

#endif