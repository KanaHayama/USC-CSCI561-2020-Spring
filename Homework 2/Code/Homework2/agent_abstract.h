#pragma once

#include "go_abstract.h"

class Agent {
protected:
	inline static bool IsFirstStep(const Player player, const Board lastBoard, const Board currentBoard) {
		return player == FIRST_PLAYER && lastBoard == currentBoard && currentBoard == 0;
	}

	inline static Player MyPlayer(const Step finishedStep) {
		return TurnUtil::WhoNext(finishedStep);
	}

	inline static vector<std::pair<Action, Board>> AllActions(const Player player, const Board lastBoard, const Board currentBoard, const bool isFirstStep, const ActionSequence* actions = &DEFAULT_ACTION_SEQUENCE) {
		return LegalActionIterator::ListAll(player, lastBoard, currentBoard, isFirstStep, actions);
	}

public:
	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) = 0;//Why step is not provided?
};

class RandomAgent : public Agent {
public:
	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		auto player = MyPlayer(finishedStep);
		auto isFirstStep = IsFirstStep(player, lastBoard, currentBoard);
		auto allActions = AllActions(player, lastBoard, currentBoard, isFirstStep);
		return Random(allActions);
	}

	static Action Random(const std::vector<std::pair<Action, Board>>& allActions) {
		std::random_device dev;
		auto rng = std::mt19937(dev());
		auto dist = std::uniform_int_distribution<std::mt19937::result_type>(0, static_cast<UINT64>(allActions.size()) - 1);
		auto selected = allActions.at(dist(rng));
		return std::get<0>(selected);
	}
};

class GreedyAgent : public Agent {
public:
	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		auto player = MyPlayer(finishedStep);
		auto isFirstStep = IsFirstStep(player, lastBoard, currentBoard);
		return Greedy(player, lastBoard, currentBoard, isFirstStep);
	}
	
	static Action Greedy(const Player player, const Board lastBoard, const Board currentBoard, const bool isFirstStep) {
		auto opponent = TurnUtil::Opponent(player);
		auto allActions = AllActions(player, lastBoard, currentBoard, isFirstStep);
		auto opponentStone = PlainState(lastBoard).CountStones(opponent);
		auto bestAction = Action::Pass;
		auto maxTake = std::numeric_limits<int>::min();
		for (const auto& action : allActions) {
			auto nextOpponentStone = PlainState(action.second).CountStones(opponent);
			auto take = opponentStone - nextOpponentStone;
			if (take > maxTake) {
				maxTake = take;
				bestAction = action.first;
			}
		}
		return bestAction;
	}
};

class AggressiveAgent : public Agent {
public:
	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		auto player = MyPlayer(finishedStep);
		auto opponent = TurnUtil::Opponent(player);
		auto isFirstStep = IsFirstStep(player, lastBoard, currentBoard);
		auto allActions = AllActions(player, lastBoard, currentBoard, isFirstStep);
		auto opponentStone = PlainState(lastBoard).CountStones(opponent);
		auto bestAction = Action::Pass;
		auto maxTake = std::numeric_limits<int>::min();
		for (const auto& action : allActions) {
			auto opponentAction = GreedyAgent::Greedy(opponent, currentBoard, action.second, false);
			Board nextNext;
			LegalActionIterator::TryAction(currentBoard, action.second, opponent, false, opponentAction, nextNext);
			auto nextNextOpponentStone = PlainState(nextNext).CountStones(opponent);
			auto take = opponentStone - nextNextOpponentStone;
			if (take > maxTake) {
				maxTake = take;
				bestAction = action.first;
			}
		}
		return bestAction;
	}
};

template<typename T>
class AlphaBetaAgent : public Agent {
private:
	class Record {
	public:
		bool HasValue = false;
		T Value;
		Record(): Value() {}
	};

	inline static bool GameFinished(const Step finishedStep, const bool isFirstStep, const bool consecutivePass) {
		return finishedStep == MAX_STEP || (!isFirstStep && consecutivePass);
	}

	T SearchMax(const int depth, const Step finishedStep, const bool isFirstStep, const Player player, const Board lastBoard, const Board currentBoard, const bool consecutivePass, Record alpha, Record beta, Action& bestAction) {
		assert(!alpha.HasValue || !beta.HasValue || alpha.Value.Compare(beta.Value) == -1);
		T get;
		if (Get(finishedStep, currentBoard, get)) {
			return get;
		}
		if (GameFinished(finishedStep, isFirstStep, consecutivePass)) {
			return T(true, finishedStep, currentBoard);
		}
		if (depth >= DepthLimit) {
			return T(false, finishedStep, currentBoard);
		}
		const auto allActions = AllActions(player, lastBoard, currentBoard, isFirstStep);
		const auto nextFinishedStep = finishedStep + 1;
		Record bestValue;
		for (const auto& action : allActions) {
			auto nextConsecutivePass = (!isFirstStep && lastBoard == currentBoard) && action.first == Action::Pass;
			Action opponentBestAction;
			auto value = SearchMin(depth + 1, nextFinishedStep, false, TurnUtil::Opponent(player), currentBoard, action.second, nextConsecutivePass, alpha, beta, opponentBestAction);
			if (!bestValue.HasValue || bestValue.Value.Compare(value) < 0) {
				bestValue.Value = value;
				bestValue.HasValue = true;
				bestAction = action.first;
			}
			if (!alpha.HasValue || alpha.Value.Compare(bestValue.Value) < 0) {
				alpha = bestValue;
			}
			if (beta.HasValue && alpha.HasValue && beta.Value.Compare(alpha.Value) <= 0) {
				break;
			}
		}
		Set(finishedStep, currentBoard, bestValue.Value);
		return bestValue.Value;
	}

	T SearchMin(const int depth, const Step finishedStep, const bool isFirstStep, const Player player, const Board lastBoard, const Board currentBoard, const bool consecutivePass, Record alpha, Record beta, Action& bestAction) {
		assert(!alpha.HasValue || !beta.HasValue || alpha.Value.Compare(beta.Value) == -1);
		T get;
		if (Get(finishedStep, currentBoard, get)) {
			return get;
		}
		if (GameFinished(finishedStep, isFirstStep, consecutivePass)) {
			return T(true, finishedStep, currentBoard);
		}
		if (depth >= DepthLimit) {
			return T(false, finishedStep, currentBoard);
		}
		const auto allActions = AllActions(player, lastBoard, currentBoard, isFirstStep);
		const auto nextFinishedStep = finishedStep + 1;
		Record bestValue;
		for (const auto& action : allActions) {
			auto nextConsecutivePass = (!isFirstStep && lastBoard == currentBoard) && action.first == Action::Pass;
			Action opponentBestAction;
			auto value = SearchMax(depth + 1, nextFinishedStep, false, TurnUtil::Opponent(player), currentBoard, action.second, nextConsecutivePass, alpha, beta, opponentBestAction);
			if (!bestValue.HasValue || bestValue.Value.Compare(value) > 0) {
				bestValue.Value = value;
				bestValue.HasValue = true;
				bestAction = action.first;
			}
			if (!beta.HasValue || beta.Value.Compare(bestValue.Value) > 0) {
				beta = bestValue;
			}
			if (beta.HasValue && alpha.HasValue && beta.Value.Compare(alpha.Value) <= 0) {
				break;
			}
		}
		Set(finishedStep, currentBoard, bestValue.Value);
		return bestValue.Value;
	}
protected:
	int DepthLimit = std::numeric_limits<int>::max();

	virtual void StepInit(const Step finishedStep, const Board board) {

	}

	virtual bool Get(const Step finishedStep, const Board board, T& get) const {
		return false;
	}

	virtual void Set(const Step finishedStep, const Board board, const T& set) {

	}
public:
	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		auto player = MyPlayer(finishedStep);
		auto opponent = TurnUtil::Opponent(player);
		auto isFirstStep = IsFirstStep(player, lastBoard, currentBoard);
		StepInit(finishedStep, currentBoard);
		Action bestAction;
		auto get = SearchMax(0, finishedStep, isFirstStep, player, lastBoard, currentBoard, false, Record(), Record(), bestAction);
		return bestAction;
	}
};

class StoneCountAlphaBetaResult {
private:
	int PartialScoreAdvantage = 0;
public:
	StoneCountAlphaBetaResult() {}

	StoneCountAlphaBetaResult(const bool gameFinished, const Step finishedStep, const Board currentBoard) {
		const auto player = TurnUtil::WhoNext(finishedStep);
		const auto& score = Score::CalcPartialScore(currentBoard);
		switch (player) {
		case Player::Black:
			PartialScoreAdvantage = score.Black - score.White;
			break;
		case Player::White:
			PartialScoreAdvantage = score.White - score.Black;
			break;
		default:
			break;
		}
	}

	int Compare(const StoneCountAlphaBetaResult& other) const {
		return (PartialScoreAdvantage < other.PartialScoreAdvantage) ? -1 : (PartialScoreAdvantage > other.PartialScoreAdvantage);
	}
};

class StoneCountAlphaBetaAgent : public AlphaBetaAgent<StoneCountAlphaBetaResult> {
private:
	typedef StoneCountAlphaBetaResult T;
	array<map<Board, T>, MAX_STEP + 1> caches;

protected:
	virtual void StepInit(const Step finishedStep, const Board board) override {
		std::for_each(caches.begin(), caches.end(), [](auto& cache) {
			cache.clear();
		});
	}

	virtual bool Get(const Step finishedStep, const Board board, T& get) const override {
		auto find = caches[finishedStep].find(board);
		if (find == caches[finishedStep].end()) {
			return false;
		} else {
			get = find->second;
			return true;
		}
	}

	virtual void Set(const Step finishedStep, const Board board, const T& get) override {
		if (finishedStep != MAX_STEP) {
			caches[finishedStep][board] = get;
		}
	}
public:
	StoneCountAlphaBetaAgent(const int _depthLimit) {
		DepthLimit = _depthLimit;
	}
};