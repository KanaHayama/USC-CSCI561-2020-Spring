
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

class RecordStorage {
protected:
	mutable recursive_mutex lock;

	virtual void safe_insert(const Board& standardBoard, const Record& record) = 0;
	virtual bool safe_lookup(const Board standardBoard, Record& record) const = 0;
	virtual void clear() = 0;
	virtual void serialize(ofstream& file) = 0;
	virtual void deserialize(ifstream& file) = 0;
	

	inline static void write_size(ofstream& file, const UINT64& size) {
		file.write(reinterpret_cast<const char*>(&size), sizeof(size));
	}

	inline static void write_record(ofstream& file, const Board& standardBoard, const Record& standardRecord) {
		file.write(reinterpret_cast<const char*>(&standardBoard), sizeof(standardBoard));//board
		file.write(reinterpret_cast<const char*>(&standardRecord.BestAction), sizeof(standardRecord.BestAction));//best action
		file.write(reinterpret_cast<const char*>(standardRecord.SelfStepToWin > MAX_STEP ? &INFINITY_STEP : &standardRecord.SelfStepToWin), sizeof(standardRecord.SelfStepToWin));//self step to win
		file.write(reinterpret_cast<const char*>(standardRecord.OpponentStepToWin > MAX_STEP ? &INFINITY_STEP : &standardRecord.OpponentStepToWin), sizeof(standardRecord.OpponentStepToWin));//opponent step to win
	}

	inline static UINT64 read_size(ifstream& file) {
		UINT64 size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size));
		return size;
	}

	inline static std::pair<Board, Record> read_record(ifstream& file) {
		Board board;
		Record record;
		file.read(reinterpret_cast<char*>(&board), sizeof(board));
		file.read(reinterpret_cast<char*>(&record.BestAction), sizeof(record.BestAction));
		file.read(reinterpret_cast<char*>(&record.SelfStepToWin), sizeof(record.SelfStepToWin));
		file.read(reinterpret_cast<char*>(&record.OpponentStepToWin), sizeof(record.OpponentStepToWin));
		return std::make_pair(board, record);
	}

#ifdef COLLECT_STORAGE_HIT_RATE
	atomic<UINT64> hit = 0;
	atomic<UINT64> total_query = 0;
#endif

public:
	bool EnableSerialize = true;
	bool EnableInsert = true;
	bool EnableLookup = true;

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

	void Set(const Board board, const Record& record) {
		if (!EnableInsert) {
			return;
		}
		auto temp = Isomorphism(board).IndexBoard(ActionMapping::EncodedToAction(record.BestAction));
		auto& standardBoard = temp.first;
		auto& standardAction = temp.second;
		auto encodedStandardAction = ActionMapping::ActionToEncoded(standardAction);
		safe_insert(standardBoard, Record(encodedStandardAction, record.SelfStepToWin, record.OpponentStepToWin));
		assert(standardAction == Action::Pass || BoardUtil::Empty(standardBoard, standardAction));
	}

	bool Get(const Board board, Record& record) {
		if (!EnableLookup) {
			return false;
		}
#ifdef COLLECT_STORAGE_HIT_RATE
		total_query++;
#endif
		auto iso = Isomorphism(board);
		auto standardBoard = iso.IndexBoard();
		Record standardRecord;
		auto found = safe_lookup(standardBoard, standardRecord);
		if (!found) {
			return false;
		}
		auto standardAction = ActionMapping::EncodedToAction(standardRecord.BestAction);
		auto nonStandardAction = iso.ReverseAction(standardBoard, standardAction);
		auto encodedNonStandardAction = ActionMapping::ActionToEncoded(nonStandardAction);
		record = Record(encodedNonStandardAction, standardRecord.SelfStepToWin, standardRecord.OpponentStepToWin);
		assert(record.SelfStepToWin <= MAX_STEP || record.OpponentStepToWin <= MAX_STEP);
		assert(nonStandardAction == Action::Pass || BoardUtil::Empty(board, nonStandardAction));
#ifdef COLLECT_STORAGE_HIT_RATE
		hit++;
#endif
		return true;
	}

	void Clear() {
#ifdef COLLECT_STORAGE_HIT_RATE
		hit = 0;
		total_query = 0;
#endif
		clear();
	}

#ifdef COLLECT_STORAGE_HIT_RATE
	double HitRate() const {
		return double(hit) / total_query;
	}
#endif

	class RecordFileIterator {
	private:
		typedef long long ssize_t;
		ifstream* file;
		ssize_t index;
		std::streampos begin;
		std::pair<decltype(index), std::pair<Board, Record>> cache = std::make_pair(-1, std::pair<Board, Record>());

		explicit RecordFileIterator(ifstream* _file, const ssize_t _index, const std::streampos _begin) : file(_file), index(_index), begin(_begin) {}

	public:
		typedef std::pair<Board, Record> value_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::pair<Board, Record>* pointer;
		typedef std::pair<Board, Record>& reference;
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
			auto offset = static_cast<int>(begin) + index * (sizeof(Board) + sizeof(Record));
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
class MemoryRecordStorage : public RecordStorage {
private:
	map<Board, Record> m;
protected:

	void safe_insert(const Board& standardBoard, const Record& record) override {
		std::unique_lock<std::recursive_mutex> l(lock);
		m.emplace(standardBoard, record);
	}

	bool safe_lookup(const Board standardBoard, Record& record) const override {
		auto find = m.find(standardBoard);
		if (find == m.end()) {
			std::unique_lock<recursive_mutex> l(lock);
			find = m.find(standardBoard);
			if (find == m.end()) {
				return false;//in multi thread context, search may duplicate
			}
		}
		record = find->second;
		return true;
	}

	void serialize(ofstream& file) override {
		write_size(file, m.size());
		std::for_each(m.begin(), m.end(), [&file](const std::pair<Board, Record>& elem) {write_record(file, elem.first, elem.second); });
	}

	void deserialize(ifstream& file) override {
		m.clear();
		auto size = read_size(file);
		for (auto i = 0ULL; i < size; i++) {
			auto elem = read_record(file);
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
		std::unique_lock<recursive_mutex> l(lock);
		m.clear();
	}
};
#else

#define NOMINMAX
#define TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS 1
#include <tbb/concurrent_map.h>
using tbb::concurrent_map;

class MemoryRecordStorage : public RecordStorage {
private:
	tbb::concurrent_map<Board, Record> m;

protected:

	void safe_insert(const Board& standardBoard, const Record& record) override {
		m.emplace(standardBoard, record);
	}

	bool safe_lookup(const Board standardBoard, Record& record) const override {
		auto find = m.find(standardBoard);
		if (find == m.end()) {
			return false;
		} else {
			record = find->second;
			return true;
		}
	}

	void serialize(ofstream& file) override {
		write_size(file, m.size());
		std::for_each(m.begin(), m.end(), [&file](const std::pair<Board, Record>& elem) {write_record(file, elem.first, elem.second); });
	}

	void deserialize(ifstream& file) override {
		m.clear();
		auto size = read_size(file);
		for (auto i = 0ULL; i < size; i++) {
			auto elem = read_record(file);
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
		std::unique_lock<recursive_mutex> l(lock);
		m.clear();
	}
};

#endif