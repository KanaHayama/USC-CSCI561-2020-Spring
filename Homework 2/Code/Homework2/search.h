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

class Record {
public:
	Action BestAction = Action::Pass;
	Step SelfStepToWin = std::numeric_limits<decltype(SelfStepToWin)>::max() - MAX_STEP;
	Step OpponentStepToWin = std::numeric_limits<decltype(OpponentStepToWin)>::max() - MAX_STEP;
	Record() {}
	Record(const Action _action, const Step _selfStepToWin, const Step _opponentStepToWin) : BestAction(_action), SelfStepToWin(_selfStepToWin), OpponentStepToWin(_opponentStepToWin) {}

	bool operator == (const Record& other) const {
		return BestAction == other.BestAction && SelfStepToWin == other.SelfStepToWin && OpponentStepToWin == other.OpponentStepToWin;
	}
};

static const array<string, MAX_STEP> DEFAULT_FILENAMES = {
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
	array<map<Board, Record>, MAX_STEP> Records;
	array<mutex, MAX_STEP> Mutexes;

	void Serialize(const Step step) {
		if (SkipSerialize[step]) {
			return;
		}
		auto& map = Records[step];
		std::unique_lock<mutex> lock(Mutexes.at(step));
		unsigned long long size = map.size();
		if (size == 0) {
			return;
		}
		ofstream file(Filenames[step], std::ios::binary);
		assert(file.is_open());
		file.write(reinterpret_cast<const char*>(&size), sizeof(size));
		for (auto it = map.begin(); it != map.end(); it++) {
			auto encodedAction = ActionMapping::ActionToEncoded(it->second.BestAction);
			file.write(reinterpret_cast<const char*>(&it->first), sizeof(it->first));//board
			file.write(reinterpret_cast<const char*>(&encodedAction), sizeof(encodedAction));//best action
			file.write(reinterpret_cast<const char*>(&it->second.SelfStepToWin), sizeof(it->second.SelfStepToWin));//self step to win
			file.write(reinterpret_cast<const char*>(&it->second.OpponentStepToWin), sizeof(it->second.OpponentStepToWin));//opponent step to win
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
			cout << "Skip deserialize step " << step + 1 << endl;
			return;
		}
		auto& map = Records[step];
		std::unique_lock<mutex> lock(Mutexes.at(step));
		assert(map.empty());
		unsigned long long size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size));
		//assert(size <= map.max_size());
		Board board;
		EncodedAction action;
		Step selfStepToWin, opponentStepToWin;
		for (auto i = 0ull; i < size; i++) {
			file.read(reinterpret_cast<char*>(&board), sizeof(board));
			file.read(reinterpret_cast<char*>(&action), sizeof(action));
			file.read(reinterpret_cast<char*>(&selfStepToWin), sizeof(selfStepToWin));
			file.read(reinterpret_cast<char*>(&opponentStepToWin), sizeof(opponentStepToWin));
			Records[step].emplace(board, Record(ActionMapping::EncodedToAction(action), selfStepToWin, opponentStepToWin));
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

	RecordManager() :Filenames(DEFAULT_FILENAMES){
		Deserialize();
	}

	void Serialize() {
		for (auto i = 0; i < MAX_STEP; i++) {
			Serialize(i);
		}
	}

	inline bool Get(const Step finishedStep, const Board board, Record& record) {
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

	inline void Set(const Step finishedStep, const Board board, const Record& record) {
		auto index = finishedStep - 1;
		if (SkipUpdate[index]) {
			return;
		}
		auto iso = Isomorphism::IndexBoard(board);
		auto& map = Records[index];
		std::unique_lock<mutex> lock(Mutexes.at(index));
		auto result = map.emplace(std::piecewise_construct, std::forward_as_tuple(iso), std::forward_as_tuple(record));
#ifdef _DEBUG
		if (!result.second) {
			auto& prev = map.at(iso);
			assert(prev == record);
		}
#endif
	}

	void Report() const {
		cout << "Report:" << endl;
		for (auto i = 0; i < MAX_STEP; i++) {
			cout << "\t" << i + 1 << ": \t" << Records[i].size() << (SkipSerialize[i] ? " \t(Skip Serialize)" : "") << (SkipQuery[i] ? " \t(Skip Query)" : "") << (SkipUpdate[i] ? " \t(Skip Update)" : "") << endl;
		}
	}

	bool Clear(const Step finishedStep) {//must stop the world
		auto index = finishedStep - 1;
		if (!SkipQuery[index]) {
			return false;
		}
		auto& map = Records[index];
		std::unique_lock<mutex> lock(Mutexes.at(index));
		map = std::map <Board, Record>();//clear too slow, will this faster?
		return true;
	}

	~RecordManager() {
		//Serialize();
	}
};

class SearchState {
private:
	int nextActionIndex = 0;
	vector<std::pair<Action, Board>> actions;
	const ActionSequence* actionSequencePtr = nullptr;
	Step finishedStep = INITIAL_FINISHED_STEP;
	Board currentBoard = EMPTY_BOARD;
	Action opponentAction = Action::Pass;

public:
	Record Rec;
	int Alpha = std::numeric_limits<decltype(Alpha)>::min();
	int Beta = std::numeric_limits<decltype(Beta)>::max();

	SearchState() = default;
	SearchState(const Action _opponent, const Step _finishedStep, const Board _lastBoard, const Board _currentBoard, const ActionSequence* _actionSequencePtr) : opponentAction(_opponent), finishedStep(_finishedStep), actions(LegalActionIterator::ListAll(TurnUtil::WhoNext(_finishedStep), _lastBoard, _currentBoard, _finishedStep == INITIAL_FINISHED_STEP, _actionSequencePtr)), actionSequencePtr(_actionSequencePtr), currentBoard(_currentBoard) {}

	Step GetFinishedStep() const {
		return finishedStep;
	}

	Board GetCurrentBoard() const {
		return currentBoard;
	}

	Action GetOpponentAction() const {
		return opponentAction;
	}

	bool Next(SearchState& next, bool& lose) {
		if (nextActionIndex >= actions.size()) {
			lose = actions.size() == 0;
			return false;
		}
		auto& a = actions[nextActionIndex];
		next = SearchState(a.first, finishedStep + 1, currentBoard, a.second, actionSequencePtr);
		nextActionIndex++;
		return true;
	}
};

class Searcher {
private:
	RecordManager& Store;
	const ActionSequence& actions;
	const volatile bool& Token;
public:
	Searcher(RecordManager& _store, const ActionSequence& _actions, const volatile bool& _tokan) : Store(_store), actions(_actions), Token(_tokan) {
	}

	void Start() {
		vector<SearchState> stack;
		stack.reserve(MAX_STEP);
		stack.emplace_back(Action::Pass, INITIAL_FINISHED_STEP, EMPTY_BOARD, EMPTY_BOARD, &actions);
		while (!stack.empty() && !Token) {
			auto& current = stack.back();
			Step finishedStep = current.GetFinishedStep();
			SearchState* ancestor = finishedStep >= 1 ? &stack.rbegin()[1] : nullptr;
			if (finishedStep == MAX_STEP) {//current == Black, min == White
				assert(TurnUtil::WhoNext(finishedStep) == Player::Black);
				auto winStatus = Score::Winner(current.GetCurrentBoard());
				if (winStatus.first == TurnUtil::WhoNext(finishedStep)) {
					current.Rec.SelfStepToWin = 0;
				} else {
					current.Rec.OpponentStepToWin = 0;
				}
			} else {
				SearchState after;
				bool noValidAction;
				if (current.Next(after, noValidAction)) {
					Record fetch;
					if (Store.Get(after.GetFinishedStep(), after.GetCurrentBoard(), fetch)) {//found record, update current
						auto trySelfStepToWin = fetch.OpponentStepToWin + 1;
						if (trySelfStepToWin < current.Rec.SelfStepToWin) {//with opponent's best reaction, I can still have posibility to win
							current.Rec.SelfStepToWin = trySelfStepToWin;
							current.Rec.OpponentStepToWin = fetch.SelfStepToWin + 1;
							current.Rec.BestAction = after.GetOpponentAction();
						}
					} else {//proceed
						stack.emplace_back(after);
						continue;
					}
				} else {
					if (noValidAction) {//dead end, opponent win
						current.Rec.OpponentStepToWin = 0;
					}
				}
			}
			//normal update ancestor
			auto tryOpponentStepToWin = current.Rec.OpponentStepToWin + 1;
			if (ancestor != nullptr && tryOpponentStepToWin < ancestor->Rec.SelfStepToWin) {
				ancestor->Rec.SelfStepToWin = tryOpponentStepToWin;
				ancestor->Rec.OpponentStepToWin = current.Rec.SelfStepToWin + 1;
				ancestor->Rec.BestAction = current.GetOpponentAction();
			}
			//store record
			Store.Set(current.GetFinishedStep(), current.GetCurrentBoard(), current.Rec);
			stack.pop_back();
		}
	}

};

class Thread {
private:
	RecordManager& Store;
	array<std::unique_ptr<thread>, MAX_NUM_THREAD> Threads{ nullptr };
	int ThreadNum = 0;

	void Search(int id) {
		cout << "Thread " << id + 1 << " started" << endl;
		srand(id);
		thread_local ActionSequence sequence = DEFAULT_ACTION_SEQUENCE;
		std::random_shuffle(sequence.begin(), sequence.end());
		Searcher searcher(Store, sequence, Tokens.at(id));
		searcher.Start();
		cout << "Thread " << id + 1 << " exit" << endl;
	}
public:
	Thread(RecordManager& _store) : Store(_store) {}

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