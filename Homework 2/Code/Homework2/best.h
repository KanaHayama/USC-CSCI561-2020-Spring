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

	static pair<bool, Action> FindAction(const Step finishedStep, const Volume volume, const Board standardBoard) {
		assert(Isomorphism(standardBoard).StandardBoard() == standardBoard);
		ifstream file(ActionFilename(finishedStep, volume), std::ios::binary);
		assert(file.is_open());
		if (!file.is_open()) {
			cout << "WARNNING: missing file step " << int(finishedStep) << " volume " << volume << endl;
			return std::make_pair(false, Action::Pass);
		}
		UINT64 size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size));
		auto found = false;
		Action standardAction;
		for (auto i = 0ull; i < size; i++) {
			Board b;
			EncodedAction a;
			file.read(reinterpret_cast<char*>(&b), sizeof(b));
			file.read(reinterpret_cast<char*>(&a), sizeof(a));
			if (b == standardBoard) {
				found = true;
				standardAction = ActionMapping::EncodedToAction(a);
				break;
			} else if (b > standardBoard) {
				break;
			}
		}
		file.close();
		return std::make_pair(found, standardAction);
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

	static void WriteAction(const Step finishedStep, const Volume volume, const vector<pair<Board, EncodedAction>>& items) {
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
	static pair<bool, Action> FindAction(const Step finishedStep, const Board board) {
		auto iso = Isomorphism(board);
		const auto standardBoard = iso.StandardBoard();
		auto vol = FindVolume(finishedStep, standardBoard);
		if (!vol.first) {
			return std::make_pair(false, Action::Pass);
		}
		auto act = FindAction(finishedStep, vol.second, standardBoard);
		if (!act.first) {
			return std::make_pair(false, Action::Pass);
		}
		auto result = iso.ReverseAction(standardBoard, act.second);
		return std::make_pair(true, result);
	}

	static void Write(const Step finishedStep, const map<Board, EncodedAction>& m, const int volumeSizeLimit) {
		vector<Index> volumes;
		vector<pair<Board, EncodedAction>> items;
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

	static map<Board, EncodedAction> Calc(const Step finishedStep, const map<Board, E>& source, const map<Board, E>& lookup) {
		map<Board, EncodedAction> result;
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
			Action bestA;
#ifdef _DEBUG
			vector<Action> bestAs;
#endif
			int bestCount = 0;
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
					bestA = action;
					bestCount = 1;
#ifdef _DEBUG
					bestAs.clear();
					bestAs.push_back(action);
#endif
				}
				if (cmp == 0) {
					if (bestA == Action::Pass) {//reduce comp complexity
						bestA = action;
					}
#ifdef _DEBUG
					bestAs.push_back(action);
#endif
					bestCount++;
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
			if (bestCount == 0) {
				invalid++;
				continue;
			}
			if (bestCount > 1) {
				multiple++;
			}
			win++;
			auto action = ActionMapping::ActionToEncoded(bestA);
			result[b] = action;
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
