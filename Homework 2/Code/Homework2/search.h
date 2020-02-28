#pragma once

#include <array>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <set>
#include <vector>
#include <regex>
#include <cstdlib>
#include <atomic>
#include "simulation.h"

using std::array;
using std::map;
using std::string;
using std::ifstream;
using std::ofstream;
using std::mutex;
using std::cout;
using std::endl;
using std::thread;
using std::set;
using std::vector;
using std::regex;
using std::atomic;

typedef UINT64 RecordType;

class WinRecord {
public:
	RecordType Black;
	RecordType White;
	WinRecord() : Black(0), White(0) {}
	WinRecord(const RecordType _black, const RecordType _white) : Black(_black), White(_white) {}
};

static const array<string, MAX_STEP> DEFAULT_RECORD_FILENAMES = {
	"D:\\EE561HW2\\step_01.data",
	"D:\\EE561HW2\\step_02.data",
	"D:\\EE561HW2\\step_03.data",
	"D:\\EE561HW2\\step_04.data",
	"D:\\EE561HW2\\step_05.data",
	"D:\\EE561HW2\\step_06.data",
	"D:\\EE561HW2\\step_07.data",
	"D:\\EE561HW2\\step_08.data",
	"D:\\EE561HW2\\step_09.data",
	"D:\\EE561HW2\\step_10.data",
	"D:\\EE561HW2\\step_11.data",
	"D:\\EE561HW2\\step_12.data",
	"D:\\EE561HW2\\step_13.data",
	"D:\\EE561HW2\\step_14.data",
	"D:\\EE561HW2\\step_15.data",
	"D:\\EE561HW2\\step_16.data",
	"D:\\EE561HW2\\step_17.data",
	"D:\\EE561HW2\\step_18.data",
	"D:\\EE561HW2\\step_19.data",
	"D:\\EE561HW2\\step_20.data",
	"D:\\EE561HW2\\step_21.data",
	"D:\\EE561HW2\\step_22.data",
	"D:\\EE561HW2\\step_23.data",
	"D:\\EE561HW2\\step_24.data",
};

class RecordManager {
private:
	array<map<Board, WinRecord>, MAX_STEP> Records;
	array<mutex, MAX_STEP> Mutexes;

	void Serialize(const Step step) {
		if (SkipSerialize[step]) {
			return;
		}
		ofstream file(Filenames[step], std::ios::binary);
		assert(file.is_open());
		std::unique_lock<mutex> lock(Mutexes.at(step));
		auto& map = Records[step];
		unsigned long long size = map.size();
		file.write(reinterpret_cast<const char*>(&size), sizeof(size));
		for (auto it = map.begin(); it != map.end(); it++) {
			file.write(reinterpret_cast<const char*>(&it->first), sizeof(it->first));
			file.write(reinterpret_cast<const char*>(&it->second.Black), sizeof(it->second.Black));
			file.write(reinterpret_cast<const char*>(&it->second.White), sizeof(it->second.White));
		}
		file.close();
		assert(!file.fail());
		cout << "Serialized " << Filenames[step] << " with " << map.size() << " entries" << endl;
	}

	void Deserialize(const Step step) {
		if (SkipSerialize[step]) {
			return;
		}
		ifstream file(Filenames[step], std::ios::binary);
		if (!file.is_open()) {
			cout << "Skip deserialize " << Filenames[step] << endl;
			return;
		}
		auto& map = Records[step];
		assert(map.empty());
		unsigned long long size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size));
		//assert(size <= map.max_size());
		Board board;
		RecordType black;
		RecordType white;
		for (auto i = 0ull; i < size; i++) {
			file.read(reinterpret_cast<char*>(&board), sizeof(board));
			file.read(reinterpret_cast<char*>(&black), sizeof(black));
			file.read(reinterpret_cast<char*>(&white), sizeof(white));
			Records[step].emplace(std::piecewise_construct, std::forward_as_tuple(board), std::forward_as_tuple(black, white));
		}
		assert(size == map.size());
		file.close();
		assert(!file.fail());
		cout << "Deserialized " << size << " entries from " << Filenames[step] << endl;
	}

	void Deserialize() {
		for (auto i = 0; i < MAX_STEP; i++) {
			Deserialize(i);
		}
	}

public:
	array<string, MAX_STEP> Filenames;
	array<bool, MAX_STEP> SkipSerialize{ false };
	array<bool, MAX_STEP> SkipUpdate{ false };
	array<bool, MAX_STEP> SkipQuery{ false };
	volatile bool Shutdown = false;

	RecordManager(const array<string, MAX_STEP>& _filenames = DEFAULT_RECORD_FILENAMES) :Filenames(_filenames) {
		SkipSerialize[MAX_STEP - 1] = true;
		SkipUpdate[MAX_STEP - 1] = true;
		SkipQuery[MAX_STEP - 1] = true;
		Deserialize();
	}

	void Serialize() {
		for (auto i = 0; i < MAX_STEP; i++) {
			Serialize(i);
		}
	}

	inline bool Get(const Step finishedStep, const Board board, WinRecord& record) {
		auto index = finishedStep - 1;
		if (SkipQuery[index]) {
			return false;
		}
		auto& map = Records[index];
		auto find = map.find(board);
		if (find == map.end()) {
			std::unique_lock<mutex> lock(Mutexes.at(index));
			find = map.find(board);
			if (find == map.end()) {
				return false;//in multi thread context, search may duplicate
			}
		}
		record = find->second;
		return true;
	}

	inline void Set(const Step finishedStep, const Board board, const RecordType black, const RecordType white) {
		auto index = finishedStep - 1;
		if (SkipUpdate[index]) {
			return;
		}
		auto& map = Records[index];
		std::unique_lock<mutex> lock(Mutexes.at(index));
		auto result = map.emplace(std::piecewise_construct, std::forward_as_tuple(board), std::forward_as_tuple(black, white));
		assert(result.second);
	}

	void Report() const {
		cout << "Report:" << endl;
		for (auto i = 0; i < MAX_STEP; i++) {
			cout << "\t" << i << ": \t" << Records[i].size() << " \t-> " << Filenames[i] << (SkipSerialize[i] ? " \t(Skip Serialize)" : "") << (SkipQuery[i] ? " \t(Skip Query)" : "") << (SkipUpdate[i] ? " \t(Skip Update)" : "") << endl;
		}
	}

	~RecordManager() {
		//Serialize();
	}
};

class SearchState {
private:
	SearchState(const Step _finishedStep, const Board _lastBoard, const Board _currentBoard) : FinishedStep(_finishedStep), Iter(TurnUtil::WhoNext(_finishedStep), _lastBoard, _currentBoard, _finishedStep == 0), Accumulator() {}
public:
	Step FinishedStep;
	LegalActionIterator Iter;
	WinRecord Accumulator;

	SearchState() : SearchState(0, Isomorphism::IndexBoard(0), Isomorphism::IndexBoard(0)) {}

	bool Next(SearchState& next) {
		Action action;
		Board after;
		if (Iter.Next(action, after)) {
			auto iso = Isomorphism::IndexBoard(after);
			next = SearchState(FinishedStep + 1, Iter.GetCurrentBoard(), iso);
			return true;
		}
		return false;
	}
};

class Searcher {
private:
	RecordManager& Record;
	
public:
	Searcher(RecordManager& _record) : Record(_record) {
		
	}

	void Start() {
		vector<SearchState> stack;
		stack.reserve(MAX_STEP);
		stack.emplace_back(SearchState());
		while (!stack.empty() && !Record.Shutdown) {
			auto& elem = stack.back();
			if (elem.FinishedStep == MAX_STEP) {
				auto winStatus = Score::Winner(elem.Iter.GetCurrentBoard());
				auto& last = stack.rbegin()[1].Accumulator;
				switch (winStatus.first) {
				case Player::Black:
					last.Black++;
					break;
				case Player::White:
					last.White++;
				}
				stack.pop_back();
			} else {
				SearchState next;
				if (elem.Next(next)) {
					WinRecord fetch;
					if (Record.Get(next.FinishedStep, next.Iter.GetCurrentBoard(), fetch)) {
						elem.Accumulator.Black += fetch.Black;
						elem.Accumulator.White += fetch.White;
					} else {
						stack.emplace_back(next);
					}
				} else {
					if (elem.FinishedStep > 0) {
						auto& last = stack.rbegin()[1];
						last.Accumulator.Black += elem.Accumulator.Black;
						last.Accumulator.White += elem.Accumulator.White;
					}
					Record.Set(elem.FinishedStep, elem.Iter.GetCurrentBoard(), elem.Accumulator.Black, elem.Accumulator.White);
					stack.pop_back();
				}
			}
		}
	}

};

static RecordManager record;

void Search() {
	Searcher searcher(record);
	searcher.Start();
	cout << "Search finished" << endl;
}

class Print {
public:
	static void Help() {
		cout << "Commands:" << endl;
		cout << "\t" << "e: execute" << endl;
		cout << "\t" << "q: quit" << endl;
		cout << "\t" << "c: clear screen" << endl;
		cout << "\t" << "r: print report" << endl;
		cout << "\t" << "s: serialize" << endl;
		cout << "\t" << "s[0-23][tf]: set skip serialize flag" << endl;
		cout << "\t" << "u[0-23][tf]: set skip update flag" << endl;
		cout << "\t" << "q[0-23][tf]: set skip query flag" << endl;
	}
	
	static void Illegal() {
		cout << "* ILLEGAL INPUT *" << endl;
	}
};

inline int run(int argc, char* argv[]) {
	auto re = std::regex("([suq])(\\d+)([tf])");
	std::unique_ptr<thread> tp = nullptr;
	while (true) {
		cout << "Input: ";
		string line;
		std::getline(std::cin, line);
		auto m = std::smatch();
		if (line.compare("") == 0) {

		} else if (line.compare("c") == 0) {
			system("CLS");
		}  else if (line.compare("e") == 0) {
			if (tp == nullptr) {
				tp = std::make_unique<thread>(Search);
			} else {
				Print::Illegal();
			}
		} else if (line.compare("q") == 0) {
			record.Shutdown = true;
			break;
		} else if (line.compare("r") == 0) {
			record.Report();
		} else if (line.compare("s") == 0) {
			record.Serialize();
		} else if (std::regex_search(line, m, re)) {
			auto index = std::stoi(m.str(2));
			auto sw = m.str(3).compare("t") == 0;
			if (index >= MAX_STEP) {
				Print::Illegal();
			}
			if (m.str(1).compare("s") == 0) {
				record.SkipSerialize[index] = sw;
			} else if (m.str(1).compare("u") == 0) {
				record.SkipUpdate[index] = sw;
			} else if (m.str(1).compare("q") == 0) {
				record.SkipQuery[index] = sw;
			}
		} else {
			Print::Help();
		}
	}
	if (tp != nullptr) {
		tp->join();
	}
	return 0;
}