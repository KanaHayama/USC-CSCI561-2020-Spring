#pragma once

#include "go_abstract.h"
#include "storage_manager.h"
#include "full_search_eval.h"

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

template <typename E>
class EvaluationTrace {
private:
	Step count = 0;
	array<E, TOTAL_POSITIONS> trace;
public:
	EvaluationTrace() {}

	EvaluationTrace(const E& eval) {
		trace[0] = eval;
		count = 1;
	}

	//EvaluationTrace(const bool gameFinished, const Step finishedStep, const Player player, const Board currentBoard) : EvaluationTrace(E(gameFinished, finishedStep, player, currentBoard)){}

	inline void Assign(const E& value) {
		trace[count] = value;
		count++;
	}

	inline E Dominance() const {
		assert(count >= 1);
		return trace[0];
	}

	bool Validate() const {
		return Dominance().Validate();
	}

	int Compare(const EvaluationTrace& other) const {//the better the larger
		short selfP = 0;
		short otherP = 0;
		while (selfP < count && otherP < other.count) {
			auto cmp = trace[selfP].Compare(other.trace[otherP]);
			if (cmp != 0) {
				return cmp;
			}
			selfP++;
			otherP++;
		}
		if (selfP >= count && otherP >= other.count) {
			return 0;
		} else if (selfP >= count) {
			return -1;
		} else {
			assert(otherP >= other.count);
			return 1;//more info is better
		}
	}

	bool operator == (const EvaluationTrace& other) const {
		return Compare(other) == 0;
	}
};

template<typename E>
class AlphaBetaAgent : public Agent {
protected:
	using Trace = EvaluationTrace<E>;
	class Limit {
	public:
		bool HasValue = false;
		Trace Evaluation;
		Limit() : Evaluation() {}
		Limit(const Trace& _evaluationTrace) : HasValue(true), Evaluation(_evaluationTrace) {}
	};
private:
	const ActionSequence& actionSequence;

	inline static bool GameFinished(const Step finishedStep, const bool isFirstStep, const bool consecutivePass) {
		return finishedStep == MAX_STEP || (!isFirstStep && consecutivePass);
	}
	Limit SearchMax(const Player me, const Player opponent, const int depth, const Step finishedStep, const bool isFirstStep, const Board lastBoard, const Board currentBoard, const bool consecutivePass, Limit alpha, Limit beta) {
		assert(!alpha.HasValue || !beta.HasValue || alpha.Evaluation.Compare(beta.Evaluation) == -1);
		Trace getTrace;
		if (Get(finishedStep, currentBoard, getTrace)) {
			return Limit(getTrace);
		}
		if (GameFinished(finishedStep, isFirstStep, consecutivePass)) {
			return Limit(Trace(E(true, finishedStep, me, currentBoard)));
		}
		auto localEvaluation = E(false, finishedStep, me, currentBoard);
		if (depth >= DepthLimit && lastBoard != currentBoard) {
			return Limit(Trace(localEvaluation));
		}
		Limit best;
		const auto allActions = AllActions(me, lastBoard, currentBoard, isFirstStep, &actionSequence);
		const auto nextFinishedStep = finishedStep + 1;
		for (const auto& action : allActions) {
			const auto nextConsecutivePass = (!isFirstStep && lastBoard == currentBoard) && action.first == Action::Pass;
			const auto value = SearchMin(me, opponent, depth + 1, nextFinishedStep, false, currentBoard, action.second, nextConsecutivePass, alpha, beta);
			if (!best.HasValue || best.Evaluation.Compare(value.Evaluation) < 0) {
				best.Evaluation = value.Evaluation;
				best.HasValue = true;
			}
			if (!alpha.HasValue || alpha.Evaluation.Compare(best.Evaluation) < 0) {
				alpha = best;
			}
			if (beta.HasValue && alpha.HasValue && beta.Evaluation.Compare(alpha.Evaluation) <= 0) {
				break;
			}
		}
		best.Evaluation.Assign(localEvaluation);
		Set(finishedStep, currentBoard, best.Evaluation);
		return best;
	}

	Limit SearchMin(const Player me, const Player opponent, const int depth, const Step finishedStep, const bool isFirstStep, const Board lastBoard, const Board currentBoard, const bool consecutivePass, Limit alpha, Limit beta) {
		assert(!alpha.HasValue || !beta.HasValue || alpha.Evaluation.Compare(beta.Evaluation) == -1);
		Trace getTrace;
		if (Get(finishedStep, currentBoard, getTrace)) {
			return Limit(getTrace);
		}
		if (GameFinished(finishedStep, isFirstStep, consecutivePass)) {
			return Limit(Trace(E(true, finishedStep, me, currentBoard)));
		}
		auto localEvaluation = E(false, finishedStep, me, currentBoard);
		if (depth >= DepthLimit && lastBoard != currentBoard) {
			return Limit(Trace(localEvaluation));
		}
		Limit best;
		const auto allActions = AllActions(opponent, lastBoard, currentBoard, isFirstStep, &actionSequence);
		const auto nextFinishedStep = finishedStep + 1;
		for (const auto& action : allActions) {
			const auto nextConsecutivePass = (!isFirstStep && lastBoard == currentBoard) && action.first == Action::Pass;
			const auto value = SearchMax(me, opponent, depth + 1, nextFinishedStep, false, currentBoard, action.second, nextConsecutivePass, alpha, beta);
			if (!best.HasValue || best.Evaluation.Compare(value.Evaluation) > 0) {
				best.Evaluation = value.Evaluation;
				best.HasValue = true;
			}
			if (!beta.HasValue || beta.Evaluation.Compare(best.Evaluation) > 0) {
				beta = best;
			}
			if (beta.HasValue && alpha.HasValue && beta.Evaluation.Compare(alpha.Evaluation) <= 0) {
				break;
			}
		}
		best.Evaluation.Assign(localEvaluation);
		Set(finishedStep, currentBoard, best.Evaluation);
		return best;
	}

protected:
	Step DepthLimit = std::numeric_limits<Step>::max();

	virtual void StepInit(const Step finishedStep, const Board board) {

	}

	virtual bool Get(const Step finishedStep, const Board board, Trace& evaluationTrace) const {
		return false;
	}

	virtual void Set(const Step finishedStep, const Board board, const Trace& evaluationTrace) {

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
			const auto value = SearchMin(me, opponent, depth + 1, nextFinishedStep, false, currentBoard, action.second, nextConsecutivePass, alpha, beta);
			auto comp = best.Evaluation.Compare(value.Evaluation);
			if (!best.HasValue || comp < 0) {
				best.Evaluation = value.Evaluation;
				best.HasValue = true;
				bestAction = action.first;// <--
			}
			if (!alpha.HasValue || alpha.Evaluation.Compare(best.Evaluation) < 0) {
				alpha = best;
			}
			if (beta.HasValue && alpha.HasValue && beta.Evaluation.Compare(alpha.Evaluation) <= 0) {
				break;
			}
		}
		assert(best.HasValue);
		return std::make_pair(bestAction, best.Evaluation.Dominance());
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
	using Trace = EvaluationTrace<E>;
	mutable StorageManager<Trace> caches;
protected:
	CachedAlphaBetaAgent(const ActionSequence& _actionSequence = DEFAULT_ACTION_SEQUENCE) : AlphaBetaAgent<E>(_actionSequence), caches("") {}

	virtual void StepInit(const Step finishedStep, const Board board) override {
		caches.ClearAll();
	}

	virtual bool Get(const Step finishedStep, const Board board, Trace& evaluationTrace) const override {
		if (caches.Get(finishedStep, board, evaluationTrace)) {
			return true;
		} else {
			return false;
		}
	}

	virtual void Set(const Step finishedStep, const Board board, const Trace& evaluationTrace) override {
		caches.Set(finishedStep, board, evaluationTrace);
	}
};

class StoneCountAlphaBetaEvaluation {
private:
	FullSearchEvaluation Final = FullSearchEvaluation();
	signed char PartialScoreAdvantage = 0;
	signed char PartialScore = 0;
	signed char LibertyAdvantage = 0;
	signed char Liberty = 0;
public:
	StoneCountAlphaBetaEvaluation() {}

	StoneCountAlphaBetaEvaluation(const bool gameFinished, const Step finishedStep, const Player player, const Board currentBoard) {
		const auto& score = Score::PartialScore(currentBoard);
		if (gameFinished) {
			const auto final = FinalScore(score);
			auto winner = final.Black > final.White ? Player::Black : Player::White;
			if (player == winner) {
				Final.SelfWinAfterStep = finishedStep;
			} else {
				Final.OpponentWinAfterStep = finishedStep;
			}
		}
		auto liberty = LibertyUtil::Liberty(currentBoard);
		switch (player) {
		case Player::Black:
			PartialScoreAdvantage = score.Black - score.White;
			PartialScore = score.Black;
			LibertyAdvantage = liberty.Black - liberty.White;
			Liberty = liberty.Black;
			break;
		case Player::White:
			PartialScoreAdvantage = score.White - score.Black;
			PartialScore = score.White;
			LibertyAdvantage = liberty.White - liberty.Black;
			Liberty = liberty.White;
			break;
		default:
			break;
		}
	}

	bool Validate() const {
		return true;
	}

	int Compare(const StoneCountAlphaBetaEvaluation& other) const {//the better the larger
		// cmp final
		bool thisFinalInitialized = Final.Initialized();
		bool otherFinalInitialized = other.Final.Initialized();
		if (thisFinalInitialized && otherFinalInitialized) {
			auto cmp = Final.Compare(other.Final);
			if (cmp != 0) {
				return cmp;
			}
		} else if (!thisFinalInitialized && otherFinalInitialized) {
			if (other.Final.Win()) {
				return -1;
			} else {
				return 1;//do not know result (self) is better than lose
			}
		} else if (thisFinalInitialized && !otherFinalInitialized) {
			if (Final.Win()) {
				return 1;
			} else {
				return -1;//lose (self) worse than do not know result
			}
		}
		//cmp local
		if (PartialScoreAdvantage < other.PartialScoreAdvantage) {
			return -1;
		} else if (PartialScoreAdvantage > other.PartialScoreAdvantage) {
			return 1;
		}

		if (LibertyAdvantage < other.LibertyAdvantage) {
			return -1;
		} else if (LibertyAdvantage > other.LibertyAdvantage) {
			return 1;
		}

		if (PartialScore < other.PartialScore) {
			return -1;
		} else if (PartialScore > other.PartialScore) {
			return 1;
		}

		if (Liberty < other.Liberty) {
			return -1;
		} else if (Liberty > other.Liberty) {
			return 1;
		}

		return 0;
	}

	bool operator == (const StoneCountAlphaBetaEvaluation& other) const {
		return Compare(other) == 0;
	}
};

class StoneCountAlphaBetaAgent : public CachedAlphaBetaAgent<StoneCountAlphaBetaEvaluation> {
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
		return CachedAlphaBetaAgent<StoneCountAlphaBetaEvaluation>::Act(finishedStep, lastBoard, currentBoard);
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