#pragma once

#include "go_abstract.h"
#include "storage_manager.h"
#include "eval.h"

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

template<typename E>
class AlphaBetaAgent : public Agent {
protected:
	class Limit {
	public:
		bool HasValue = false;
		E Evaluation;
		Limit() : Evaluation() {}
		Limit(const E& _evaluation) : HasValue(true), Evaluation(_evaluation) {}
	};
private:
	const ActionSequence& actionSequence;

	inline static bool GameFinished(const Step finishedStep, const bool isFirstStep, const bool consecutivePass) {
		return finishedStep == MAX_STEP || (!isFirstStep && consecutivePass);
	}

	Limit SearchMiniMax(
		const bool max, const int depth,
		const Player me, const Player opponent, 
		const Step finishedStep, const bool isFirstStep, 
		const Board lastBoard, const Board currentBoard, 
		const bool consecutivePass, 
		Limit alpha, Limit beta) 
	{
		assert(!alpha.HasValue || !beta.HasValue || alpha.Evaluation.Compare(beta.Evaluation) == -1);
		bool hasKoAction;
		const auto allActions = LegalActionIterator::ListAll(max ? me : opponent, lastBoard, currentBoard, isFirstStep, &actionSequence, hasKoAction);
		if (!hasKoAction) {
			E getEval;
			if (Get(finishedStep, currentBoard, getEval)) {
				return Limit(getEval);
			}
		}
		if (GameFinished(finishedStep, isFirstStep, consecutivePass)) {
			return Limit(E(true, finishedStep, me, currentBoard));
		}
		auto localEvaluation = E(false, finishedStep, me, currentBoard);
		if (depth >= DepthLimit && lastBoard != currentBoard) {
			return Limit(localEvaluation);
		}
		Limit best;
		bool bestIsConsecutivePass = false;
		const auto nextFinishedStep = finishedStep + 1;
		for (const auto& action : allActions) {
			const auto nextConsecutivePass = (!isFirstStep && lastBoard == currentBoard) && action.first == Action::Pass;
			auto value = SearchMiniMax(!max, depth + 1, me, opponent, nextFinishedStep, false, currentBoard, action.second, nextConsecutivePass, alpha, beta);
			auto cmp = best.Evaluation.Compare(value.Evaluation);
			auto updateBest = !best.HasValue || (max ? cmp < 0 : cmp > 0);
			if (updateBest) {
				best.Evaluation = value.Evaluation;
				best.HasValue = true;
				bestIsConsecutivePass = nextConsecutivePass;
			}
			if (max) {
				if (!alpha.HasValue || alpha.Evaluation.Compare(best.Evaluation) < 0) {
					alpha = best;
				}
			} else {
				if (!beta.HasValue || beta.Evaluation.Compare(best.Evaluation) > 0) {
					beta = best;
				}
			}
			if (beta.HasValue && alpha.HasValue && beta.Evaluation.Compare(alpha.Evaluation) <= 0) {
				break;
			}
		}
		best.Evaluation.Push(localEvaluation);
		if (!hasKoAction && !bestIsConsecutivePass) {
			Set(finishedStep, currentBoard, best.Evaluation);
		}
		return best;
	}

protected:
	Step DepthLimit = std::numeric_limits<Step>::max();

	virtual void StepInit(const Step finishedStep, const Board board) {

	}

	virtual bool Get(const Step finishedStep, const Board board, E& evaluation) const {
		return false;
	}

	virtual void Set(const Step finishedStep, const Board board, const E& evaluation) {

	}

	pair<Action, E> Search(const Step finishedStep, const Board lastBoard, const Board currentBoard) {
		auto me = MyPlayer(finishedStep);
		auto opponent = TurnUtil::Opponent(me);
		auto isFirstStep = IsFirstStep(me, lastBoard, currentBoard);
		auto alpha = Limit();
		auto beta = Limit();
		auto max = true;
		auto depth = 0;
		auto bestAction = Action::Pass;

		Limit best;
		const auto allActions = AllActions(me, lastBoard, currentBoard, isFirstStep, &actionSequence);
		const auto nextFinishedStep = finishedStep + 1;
		for (const auto& action : allActions) {
			const auto nextConsecutivePass = (!isFirstStep && lastBoard == currentBoard) && action.first == Action::Pass;
			auto value = SearchMiniMax(false, depth + 1, me, opponent, nextFinishedStep, false, currentBoard, action.second, nextConsecutivePass, alpha, beta);
			auto comp = best.Evaluation.Compare(value.Evaluation);
			if (!best.HasValue || comp < 0) {
				best.Evaluation = value.Evaluation;
				best.HasValue = true;
				bestAction = action.first;// <--
			}
			if (!alpha.HasValue || alpha.Evaluation.Compare(best.Evaluation) < 0) {
				alpha = best;
			}
		}
		assert(best.HasValue);
		return std::make_pair(bestAction, best.Evaluation);
	}
public:
	AlphaBetaAgent(const ActionSequence& _actionSequence = DEFAULT_ACTION_SEQUENCE) : actionSequence(_actionSequence) {}

	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		StepInit(finishedStep, currentBoard);
		auto result = Search(finishedStep, lastBoard, currentBoard);
		return result.first;
	}
};

template<typename E>
class CachedAlphaBetaAgent : public AlphaBetaAgent<E> {
private:
	mutable StorageManager<E> caches;
protected:
	CachedAlphaBetaAgent(const ActionSequence& _actionSequence = DEFAULT_ACTION_SEQUENCE) : AlphaBetaAgent<E>(_actionSequence), caches("") {}

	virtual void StepInit(const Step finishedStep, const Board board) override {
		caches.ClearAll();
	}

	virtual bool Get(const Step finishedStep, const Board board, E& evaluation) const override {
		return caches.Get(finishedStep, board, evaluation);
	}

	virtual void Set(const Step finishedStep, const Board board, const E& evaluation) override {
		caches.Set(finishedStep, board, evaluation);
	}
};

class StoneCountAlphaBetaAgent : public CachedAlphaBetaAgent<EvaluationTrace<StoneCountAlphaBetaEvaluation>> {
private:
	array<Step, TOTAL_POSITIONS> depthLimits;//index by num stones
protected:
public:
	StoneCountAlphaBetaAgent(const Step _depthLimit) {
		depthLimits.fill(_depthLimit);
	}

	StoneCountAlphaBetaAgent(const array<Step, TOTAL_POSITIONS>& _depthLimits): depthLimits(_depthLimits) {}

	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		const auto numStone = Score::Stones(currentBoard);
		const auto stones = numStone.Black + numStone.White;
		DepthLimit = depthLimits[stones];
#ifdef INTERACT_MODE
		cout << "Alpha-beta search depth: " << static_cast<int>(DepthLimit) << endl;
#endif
		return CachedAlphaBetaAgent<EvaluationTrace<StoneCountAlphaBetaEvaluation>>::Act(finishedStep, lastBoard, currentBoard);
	}
};

class LookupStoneCountAlphaBetaAgent : public StoneCountAlphaBetaAgent {
public:
	LookupStoneCountAlphaBetaAgent(const Step _depthLimit) : StoneCountAlphaBetaAgent(_depthLimit) {}
	LookupStoneCountAlphaBetaAgent(const array<Step, TOTAL_POSITIONS>& _depthLimits) : StoneCountAlphaBetaAgent(_depthLimits) {}

	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		//TODO:
		return StoneCountAlphaBetaAgent::Act(finishedStep, lastBoard, currentBoard);
	}
};

class WinStepAlphaBetaAgent : public CachedAlphaBetaAgent<FullSearchEvaluation> {
private:
public:
	WinStepAlphaBetaAgent(const ActionSequence& _actionSequence = DEFAULT_ACTION_SEQUENCE) : CachedAlphaBetaAgent<FullSearchEvaluation>(_actionSequence) {
		assert(DepthLimit > MAX_STEP);
	}

	pair<Action, FullSearchEvaluation> AlphaBeta(const Step finishedStep, const Board lastBoard, const Board currentBoard) {
		return Search(finishedStep, lastBoard, currentBoard);
	}
};