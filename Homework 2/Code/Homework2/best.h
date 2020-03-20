#pragma once

#include "go_abstract.h"
#include "eval.h"

const static string SEPARATOR = "_";
const static string BASE_FILENAME = "truth";
const static string EXTENSION = ".txt";
const static string INDEX_NAME = "index";
const static string ACTION_NAME = "action";

struct Index {
	UINT64 Begin = 0;
	UINT64 End = 0;
};

using ActionMask = Board;

const static ActionMask PASS_MASK = 1ull << EMPTY_SHIFT;

class Best {
private:
	using Volume = UINT64;

	static string IndexFilename(const Step finishedStep) {
		return BASE_FILENAME + SEPARATOR + INDEX_NAME + SEPARATOR + std::to_string(finishedStep) + EXTENSION;
	}

	static string ActionFilename(const Step finishedStep, const Volume volume) {
		return BASE_FILENAME + SEPARATOR + ACTION_NAME + SEPARATOR + std::to_string(finishedStep) + SEPARATOR + std::to_string(volume) + EXTENSION;
	}

	static pair<bool, Volume> FindVolume(const Step finishedStep, const Board standardBoard) {
		assert(Isomorphism(standardBoard).StandardBoard() == standardBoard);
		ifstream file(IndexFilename(finishedStep), std::ios::binary);
		if (!file.is_open()) {
			return std::make_pair(false, 0);
		}
		UINT64 size;
		auto found = false;
		Volume volume;
		file.read(reinterpret_cast<char*>(&size), sizeof(size));
		for (auto i = 0ull; i < size; i++) {
			Index index;
			file.read(reinterpret_cast<char*>(&index), sizeof(index));
			assert(index.Begin < index.End);
			if (index.Begin <= standardBoard && standardBoard < index.End) {
				found = true;
				volume = i;
				break;
			} else if (index.Begin > standardBoard) {
				break;
			}
		}
		file.close();
		return std::make_pair(found, volume);
	}

	static void TestAction(const ActionMask mask, const Action action, vector<Action>& container) {
		bool flag;
		if (action == Action::Pass) {
			flag = (mask & PASS_MASK) != 0;
		} else {
			flag = (mask & static_cast<ActionMask>(action)) != 0;
		}
		if (flag) {
			container.push_back(action);
		}
	}

	static pair<bool, vector<Action>> FindAction(const Step finishedStep, const Volume volume, const Board standardBoard) {
		assert(Isomorphism(standardBoard).StandardBoard() == standardBoard);
		ifstream file(ActionFilename(finishedStep, volume), std::ios::binary);
		assert(file.is_open());
		if (!file.is_open()) {
			cout << "WARNNING: missing file step " << int(finishedStep) << " volume " << volume << endl;
			return std::make_pair(false, vector<Action>());
		}
		UINT64 size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size));
		auto found = false;
		auto result = vector<Action>();
		for (auto i = 0ull; i < size; i++) {
			Board b;
			ActionMask a;
			file.read(reinterpret_cast<char*>(&b), sizeof(b));
			file.read(reinterpret_cast<char*>(&a), sizeof(a));
			if (b == standardBoard) {
				found = true;
				TestAction(a, Action::P00, result);
				TestAction(a, Action::P01, result);
				TestAction(a, Action::P02, result);
				TestAction(a, Action::P03, result);
				TestAction(a, Action::P04, result);
				TestAction(a, Action::P10, result);
				TestAction(a, Action::P11, result);
				TestAction(a, Action::P12, result);
				TestAction(a, Action::P13, result);
				TestAction(a, Action::P14, result);
				TestAction(a, Action::P20, result);
				TestAction(a, Action::P21, result);
				TestAction(a, Action::P22, result);
				TestAction(a, Action::P23, result);
				TestAction(a, Action::P24, result);
				TestAction(a, Action::P30, result);
				TestAction(a, Action::P31, result);
				TestAction(a, Action::P32, result);
				TestAction(a, Action::P33, result);
				TestAction(a, Action::P34, result);
				TestAction(a, Action::P40, result);
				TestAction(a, Action::P41, result);
				TestAction(a, Action::P42, result);
				TestAction(a, Action::P43, result);
				TestAction(a, Action::P44, result);
				TestAction(a, Action::Pass, result);//make pass last elem to reduce compute complexity
				break;
			} else if (b > standardBoard) {
				break;
			}
		}
		file.close();
		return std::make_pair(found, result);
	}

	static void WriteIndex(const Step finishedStep, const vector<Index>& volumes) {
		if (volumes.size() == 0) {
			return;
		}
		ofstream file(IndexFilename(finishedStep), std::ios::binary);
		assert(file.is_open());
		UINT64 size = volumes.size();
		file.write(reinterpret_cast<const char*>(&size), sizeof(size));
		for (auto it = volumes.begin(); it != volumes.end(); it++) {
			file.write(reinterpret_cast<const char*>(&*it), sizeof(*it));
		}
		file.close();
	}

	static void WriteAction(const Step finishedStep, const Volume volume, const vector<pair<Board, ActionMask>>& items) {
		assert(items.size() > 0);
		ofstream file(ActionFilename(finishedStep, volume), std::ios::binary);
		assert(file.is_open());
		UINT64 size = items.size();
		file.write(reinterpret_cast<const char*>(&size), sizeof(size));
		for (auto it = items.begin(); it != items.end(); it++) {
			file.write(reinterpret_cast<const char*>(&it->first), sizeof(it->first));
			file.write(reinterpret_cast<const char*>(&it->second), sizeof(it->second));
		}
		file.close();
	}
public:
	static pair<bool, vector<Action>> FindAction(const Step finishedStep, const Board board) {
		auto iso = Isomorphism(board);
		const auto standardBoard = iso.StandardBoard();
		auto vol = FindVolume(finishedStep, standardBoard);
		if (!vol.first) {
			return std::make_pair(false, vector<Action>());
		}
		auto act = FindAction(finishedStep, vol.second, standardBoard);
		if (!act.first) {
			return std::make_pair(false, vector<Action>());
		}
		auto result = vector<Action>();
		for (const auto standardAction : act.second) {
			auto a = iso.ReverseAction(standardBoard, standardAction);
			result.push_back(a);
		}
		return std::make_pair(true, result);
	}

	static void Write(const Step finishedStep, const map<Board, ActionMask>& m, const int volumeSizeLimit) {
		vector<Index> volumes;
		vector<pair<Board, ActionMask>> items;
		Index index;
		auto iter = m.begin();
		while (iter != m.end()) {
			const auto& b = iter->first;
			const auto& a = iter->second;
			items.emplace_back(std::make_pair(b, a));
			if (items.size() >= volumeSizeLimit) {
				WriteAction(finishedStep, volumes.size(), items);

				auto sep = b + 1;
				index.End = sep;
				volumes.push_back(index);

				index.Begin = sep;
				items.clear();
			}
			iter++;
		}
		if (items.size() > 0) {
			WriteAction(finishedStep, volumes.size(), items);

			index.End = 1ull << STEP_SHIFT;
			volumes.push_back(index);
		}
		WriteIndex(finishedStep, volumes);
	}
};

class BestConverter {
private:
	using E = FullSearchEvaluation;

	static string Filename(const string& prefix, const Step finishedStep) {
		return prefix + std::to_string(finishedStep);
	}

	static map<Board, E> Read(const string& filename) {
		map<Board, E> result;
		ifstream file(filename, std::ios::binary);
		assert(file.is_open());
		UINT64 size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size));
		for (auto i = 0ull; i < size; i++) {
			Board board;
			E record;
			file.read(reinterpret_cast<char*>(&board), sizeof(board));
			file.read(reinterpret_cast<char*>(&record), sizeof(record));
			result[board] = record;
		}
		file.close();
		return result;
	}

	static map<Board, E> Read(const string& prefix, const Step finishedStep) {
		return Read(Filename(prefix, finishedStep));
	}

	static map<Board, ActionMask> Calc(const Step finishedStep, const map<Board, E>& source, const map<Board, E>& lookup) {
		map<Board, ActionMask> result;
		auto player = TurnUtil::WhoNext(finishedStep);
		UINT64 total = 0;
		UINT64 invalid = 0;
		UINT64 incomplete = 0;
		UINT64 lose = 0;
		UINT64 multiple = 0;
		UINT64 win = 0;
		for (auto it = source.begin(); it != source.end(); it++) {
			total++;
			const Board& b = it->first;
			assert(Isomorphism(b).StandardBoard() == b);
			const E& e = it->second;
			auto actions = LegalActionIterator::ListAll(player, EMPTY_BOARD, b, b == EMPTY_BOARD, &DEFAULT_ACTION_SEQUENCE);
			auto complete = true;
			E bestE;
			ActionMask bestA = EMPTY_BOARD;
			for (const auto& a : actions) {
				const auto& action = a.first;
				const auto& next = a.second;
				const auto standard = Isomorphism(next).StandardBoard();
				auto find = lookup.find(standard);
				if (find == lookup.end()) {
					complete = false;
					continue;
				}
				const auto& nextB = find->first;
				const auto& origionalE = find->second;
				auto e = origionalE.OpponentView();
				auto cmp = bestE.Compare(e);
				if (cmp < 0) {
					bestE = e;
					bestA = action == Action::Pass ? PASS_MASK : static_cast<ActionMask>(action);
				}
				if (cmp == 0) {
					if (action == Action::Pass) {//reduce comp complexity
						bestA |= PASS_MASK;
					} else {
						bestA |= static_cast<ActionMask>(action);
					}
				}
			}
			if (!complete) {
				incomplete++;
				//continue;
			}
			if (!bestE.Win()) {
				lose++;
				continue;
			}
			auto bestCount = __builtin_popcountll(bestA);
			if (bestCount == 0) {
				invalid++;
				continue;
			}
			if (bestCount > 1) {
				multiple++;
			}
			win++;
			result[b] = bestA;
		}
		cout << "finished step " << int(finishedStep) << " player " << (player == Player::Black ? "X" : "O") << " total/invalid/incomplete/lose/multiple/win: " << total << "/" << invalid << "/" << incomplete << "/" << lose << "/" << multiple << "/" << win << endl;
		return result;
	}
public:
	static void Convert(const string& prefix, const int begin, const int end, const int limit) {
		auto next = Read(prefix, begin);
		for (auto step = begin; step < end; step++) {
			auto current = std::move(next);
			next = Read(prefix, step + 1);

			auto result = Calc(step, current, next);
			Best::Write(step, result, limit);
		}
	}
};
