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
#include <algorithm>
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

const int MAX_NUM_THREAD = 56;

typedef UINT64 RecordType;

class WinRecord {
public:
	RecordType Black;
	RecordType White;
	WinRecord() : Black(0), White(0) {}
	WinRecord(const RecordType _black, const RecordType _white) : Black(_black), White(_white) {}
};

static const array<string, MAX_STEP> DEFAULT_RECORD_FILENAMES = {
#ifdef _DEBUG
	"D:\\EE561HW2\\Debug\\step_01.data",
	"D:\\EE561HW2\\Debug\\step_02.data",
	"D:\\EE561HW2\\Debug\\step_03.data",
	"D:\\EE561HW2\\Debug\\step_04.data",
	"D:\\EE561HW2\\Debug\\step_05.data",
	"D:\\EE561HW2\\Debug\\step_06.data",
	"D:\\EE561HW2\\Debug\\step_07.data",
	"D:\\EE561HW2\\Debug\\step_08.data",
	"D:\\EE561HW2\\Debug\\step_09.data",
	"D:\\EE561HW2\\Debug\\step_10.data",
	"D:\\EE561HW2\\Debug\\step_11.data",
	"D:\\EE561HW2\\Debug\\step_12.data",
	"D:\\EE561HW2\\Debug\\step_13.data",
	"D:\\EE561HW2\\Debug\\step_14.data",
	"D:\\EE561HW2\\Debug\\step_15.data",
	"D:\\EE561HW2\\Debug\\step_16.data",
	"D:\\EE561HW2\\Debug\\step_17.data",
	"D:\\EE561HW2\\Debug\\step_18.data",
	"D:\\EE561HW2\\Debug\\step_19.data",
	"D:\\EE561HW2\\Debug\\step_20.data",
	"D:\\EE561HW2\\Debug\\step_21.data",
	"D:\\EE561HW2\\Debug\\step_22.data",
	"D:\\EE561HW2\\Debug\\step_23.data",
	"D:\\EE561HW2\\Debug\\step_24.data",
#else
	"D:\\EE561HW2\\Release\\step_01.data",
	"D:\\EE561HW2\\Release\\step_02.data",
	"D:\\EE561HW2\\Release\\step_03.data",
	"D:\\EE561HW2\\Release\\step_04.data",
	"D:\\EE561HW2\\Release\\step_05.data",
	"D:\\EE561HW2\\Release\\step_06.data",
	"D:\\EE561HW2\\Release\\step_07.data",
	"D:\\EE561HW2\\Release\\step_08.data",
	"D:\\EE561HW2\\Release\\step_09.data",
	"D:\\EE561HW2\\Release\\step_10.data",
	"D:\\EE561HW2\\Release\\step_11.data",
	"D:\\EE561HW2\\Release\\step_12.data",
	"D:\\EE561HW2\\Release\\step_13.data",
	"D:\\EE561HW2\\Release\\step_14.data",
	"D:\\EE561HW2\\Release\\step_15.data",
	"D:\\EE561HW2\\Release\\step_16.data",
	"D:\\EE561HW2\\Release\\step_17.data",
	"D:\\EE561HW2\\Release\\step_18.data",
	"D:\\EE561HW2\\Release\\step_19.data",
	"D:\\EE561HW2\\Release\\step_20.data",
	"D:\\EE561HW2\\Release\\step_21.data",
	"D:\\EE561HW2\\Release\\step_22.data",
	"D:\\EE561HW2\\Release\\step_23.data",
	"D:\\EE561HW2\\Release\\step_24.data",
#endif
};

class RecordManager {
private:
	array<map<Board, WinRecord>, MAX_STEP> Records;
	array<mutex, MAX_STEP> Mutexes;

	void Serialize(const Step step) {
		if (SkipSerialize[step]) {
			return;
		}
		auto& map = Records[step];
		unsigned long long size = map.size();
		if (size == 0) {
			return;
		}
		ofstream file(Filenames[step], std::ios::binary);
		assert(file.is_open());
		std::unique_lock<mutex> lock(Mutexes.at(step));
		
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

	RecordManager(const array<string, MAX_STEP>& _filenames = DEFAULT_RECORD_FILENAMES) :Filenames(_filenames) {
		//SkipSerialize[MAX_STEP - 1] = true;
		//SkipUpdate[MAX_STEP - 1] = true;
		//SkipQuery[MAX_STEP - 1] = true;

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
		auto iso = Isomorphism::IndexBoard(board);
		auto& map = Records[index];
		auto find = map.find(iso);
		if (find == map.end()) {
			std::unique_lock<mutex> lock(Mutexes.at(index));
			find = map.find(iso);
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
		auto iso = Isomorphism::IndexBoard(board);
		auto& map = Records[index];
		std::unique_lock<mutex> lock(Mutexes.at(index));
		auto result = map.emplace(std::piecewise_construct, std::forward_as_tuple(iso), std::forward_as_tuple(black, white));
#ifdef _DEBUG
		if (!result.second) {
			auto& prev = map.at(iso);
			assert(prev.Black == black && prev.White == white);
		}
#endif
	}

	void Report() const {
		cout << "Report:" << endl;
		for (auto i = 0; i < MAX_STEP; i++) {
			cout << "\t" << i + 1 << ": \t" << Records[i].size() << " \t-> " << Filenames[i] << (SkipSerialize[i] ? " \t(Skip Serialize)" : "") << (SkipQuery[i] ? " \t(Skip Query)" : "") << (SkipUpdate[i] ? " \t(Skip Update)" : "") << endl;
		}
	}

	bool Clear(const Step finishedStep) {//must stop the world
		auto index = finishedStep - 1;
		if (!SkipQuery[index]) {
			return false;
		}
		auto& map = Records[index];
		std::unique_lock<mutex> lock(Mutexes.at(index));
		map.clear();
		return true;
	}

	~RecordManager() {
		//Serialize();
	}
};

class SearchState {
private:
	
public:
	Step FinishedStep;
	LegalActionIterator Iter;
	WinRecord Accumulator;

	SearchState() :FinishedStep(), Iter(), Accumulator() {}
	SearchState(const Step _finishedStep, const Board _lastBoard, const Board _currentBoard, const ActionSequence* _actions) : FinishedStep(_finishedStep), Iter(TurnUtil::WhoNext(_finishedStep), _lastBoard, _currentBoard, _finishedStep == 0, _actions), Accumulator() {}

	bool Next(SearchState& next, bool& lose) {
		Action action;
		Board after;
		if (Iter.Next(action, after, lose)) {
			next = SearchState(FinishedStep + 1, Iter.GetCurrentBoard(), after, Iter.GetActionSequencePtr());
			return true;
		}
		return false;
	}
};

class Searcher {
private:
	RecordManager& Record;
	const ActionSequence& actions;
	const volatile bool& Token;
public:
	Searcher(RecordManager& _record, const ActionSequence& _actions, const volatile bool& _tokan) : Record(_record), actions(_actions), Token(_tokan) {
	}

	void Start() {
		vector<SearchState> stack;
		stack.reserve(MAX_STEP);
		stack.emplace_back(0, 0, 0, &actions);
		while (!stack.empty() && !Token) {
			auto& elem = stack.back();
			if (elem.FinishedStep == MAX_STEP) {
				auto winStatus = Score::Winner(elem.Iter.GetCurrentBoard());
				auto& last = stack.rbegin()[1].Accumulator;
				switch (winStatus.first) {
				case Player::Black:
					elem.Accumulator.Black = 1;
					break;
				case Player::White:
					elem.Accumulator.White = 1;
				}
			} else {
				SearchState next;
				bool lose;
				if (elem.Next(next, lose)) {
					WinRecord fetch;
					if (Record.Get(next.FinishedStep, next.Iter.GetCurrentBoard(), fetch)) {
						elem.Accumulator.Black += fetch.Black;
						elem.Accumulator.White += fetch.White;
					} else {
						stack.emplace_back(next);
					}
					continue;
				} else {
					if (lose) {
						switch (elem.Iter.GetPlayer()) {
						case Player::Black:
							elem.Accumulator.White = 1;
							break;
						case Player::White:
							elem.Accumulator.Black = 1;
						}
					}
				}
			}
			if (elem.FinishedStep > 0) {
				auto& last = stack.rbegin()[1];
				last.Accumulator.Black += elem.Accumulator.Black;
				last.Accumulator.White += elem.Accumulator.White;
			}
			Record.Set(elem.FinishedStep, elem.Iter.GetCurrentBoard(), elem.Accumulator.Black, elem.Accumulator.White);
			stack.pop_back();
		}
	}

};



class Thread {
private:
	RecordManager& Record;
	array<std::unique_ptr<thread>, MAX_NUM_THREAD> Threads{ nullptr };
	int ThreadNum = 0;

	void Search(int id) {
		cout << "Thread " << id + 1 << " started" << endl;
		srand(id);
		thread_local ActionSequence sequence = DEFAULT_ACTION_SEQUENCE;
		std::random_shuffle(sequence.begin(), sequence.end());
		Searcher searcher(Record, sequence, Tokens.at(id));
		searcher.Start();
		cout << "Thread " << id + 1 << " exit" << endl;
	}
public:
	Thread(RecordManager& _record) : Record(_record) {}

	array<volatile bool, MAX_NUM_THREAD> Tokens{ false };

	bool Resize(const unsigned char num) {
		if (num > MAX_NUM_THREAD) {
			return false;
		}
		if (num < ThreadNum) {
			for (auto i = num; i < ThreadNum; i++) {
				Tokens[i] = true;
			}
			for (auto i = num; i < ThreadNum; i++) {
				Threads[i]->join();
				Tokens[i] = false;
				Threads[i] = nullptr;
			}
		} else if (num > ThreadNum) {
			for (auto i = ThreadNum; i < num; i++) {
				Threads[i] = std::make_unique<thread>(&Thread::Search, this, i);
			}
		}
		ThreadNum = num;
		return true;
	}

};

class Print {
public:
	static void Help() {
		cout << "Commands:" << endl;
		cout << "\t" << "e: execute" << endl;
		cout << "\t" << "p: pause" << endl;
		cout << "\t" << "q: quit" << endl;
		cout << "\t" << "c: clear screen" << endl;
		cout << "\t" << "r: print report" << endl;
		cout << "\t" << "s: serialize" << endl;
		cout << "\t" << "t[0-XX]: thread num" << endl;
		cout << "\t" << "c[0-XX]: clear storage" << endl;
		cout << "\t" << "s[0-23][tf]: set skip serialize flag" << endl;
		cout << "\t" << "u[0-23][tf]: set skip update flag" << endl;
		cout << "\t" << "q[0-23][tf]: set skip query flag" << endl;
	}
	
	static void Illegal() {
		cout << "* ILLEGAL INPUT *" << endl;
	}
};

inline int run(int argc, char* argv[]) {
	RecordManager record;
	Thread threads(record);
	auto flagRe = std::regex("([suq])(\\d+)([tf])");
	auto threadRe = std::regex("t(\\d+)");
	auto clearRe = std::regex("c(\\d+)");
	
	while (true) {
		cout << "Input: ";
		string line;
		std::getline(std::cin, line);
		auto m = std::smatch();
		if (line.compare("") == 0) {

		} else if (line.compare("c") == 0) {
			system("CLS");
		} else if (line.compare("e") == 0) {
			threads.Resize(1);
		} else if (line.compare("p") == 0) {
			threads.Resize(0);
		} else if (line.compare("q") == 0) {
			threads.Resize(0);
			break;
		} else if (line.compare("r") == 0) {
			record.Report();
		} else if (line.compare("s") == 0) {
			record.Serialize();
		} else if (std::regex_search(line, m, flagRe)) {
			auto index = std::stoi(m.str(2)) - 1;
			auto sw = m.str(3).compare("t") == 0;
			if (index < 0 || index >= MAX_STEP) {
				Print::Illegal();
			}
			if (m.str(1).compare("s") == 0) {
				record.SkipSerialize[index] = sw;
			} else if (m.str(1).compare("u") == 0) {
				record.SkipUpdate[index] = sw;
			} else if (m.str(1).compare("q") == 0) {
				record.SkipQuery[index] = sw;
			}
		} else if (std::regex_search(line, m, threadRe)) {
			if (!threads.Resize(std::stoi(m.str(1)))) {
				Print::Illegal();
			}
		} else if (std::regex_search(line, m, clearRe)) {
			if (!record.Clear(std::stoi(m.str(1)))) {
				Print::Illegal();
			}
		} else {
			Print::Help();
		}
	}
	return 0;
}