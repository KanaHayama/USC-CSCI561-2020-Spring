#pragma once

#include "go_abstract.h"
#include "storage_manager.h"

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
	const ActionSequence& actionSequence;

	class Record {
	public:
		bool HasValue = false;
		T Evaluation;
		Action Action = Action::Pass;
		Record(): Evaluation() {}
		Record(const ::Action _action, const T& _evaluation) : HasValue(true), Action(_action), Evaluation(_evaluation) {}
		Record(const T& _evaluation) : HasValue(true), Evaluation(_evaluation) {}
	};

	inline static bool GameFinished(const Step finishedStep, const bool isFirstStep, const bool consecutivePass) {
		return finishedStep == MAX_STEP || (!isFirstStep && consecutivePass);
	}

	Record SearchMax(const Player me, const Player opponent, const int depth, const Step finishedStep, const bool isFirstStep, const Board lastBoard, const Board currentBoard, const bool consecutivePass, Record alpha, Record beta) {
		assert(!alpha.HasValue || !beta.HasValue || alpha.Evaluation.Compare(beta.Evaluation) == -1);
		Action getAction;
		T getEvaluation;
		if (Get(finishedStep, currentBoard, getEvaluation, getAction)) {
			return Record(getAction, getEvaluation);
		}
		if (GameFinished(finishedStep, isFirstStep, consecutivePass)) {
			return Record(T(true, me, currentBoard));
		}
		if (depth >= DepthLimit && lastBoard != currentBoard) {
			return Record(T(false, me, currentBoard));
		}
		Record best;
		const auto allActions = AllActions(me, lastBoard, currentBoard, isFirstStep, &actionSequence);
		const auto nextFinishedStep = finishedStep + 1;
		for (const auto& action : allActions) {
			const auto nextConsecutivePass = (!isFirstStep && lastBoard == currentBoard) && action.first == Action::Pass;
			const auto value = SearchMin(me, opponent, depth + 1, nextFinishedStep, false, currentBoard, action.second, nextConsecutivePass, alpha, beta);
			if (!best.HasValue || best.Evaluation.Compare(value.Evaluation) < 0) {
				best.Evaluation = value.Evaluation;
				best.HasValue = true;
				best.Action = action.first;
			}
			if (!alpha.HasValue || alpha.Evaluation.Compare(best.Evaluation) < 0) {
				alpha = best;
			}
			if (beta.HasValue && alpha.HasValue && beta.Evaluation.Compare(alpha.Evaluation) <= 0) {
				break;
			}
		}
		
		Set(finishedStep, currentBoard, best.Evaluation, best.Action);
		return best;
	}

	Record SearchMin(const Player me, const Player opponent, const int depth, const Step finishedStep, const bool isFirstStep, const Board lastBoard, const Board currentBoard, const bool consecutivePass, Record alpha, Record beta) {
		assert(!alpha.HasValue || !beta.HasValue || alpha.Evaluation.Compare(beta.Evaluation) == -1);
		Action getAction;
		T getEvaluation;
		if (Get(finishedStep, currentBoard, getEvaluation, getAction)) {
			return Record(getAction, getEvaluation);
		}
		if (GameFinished(finishedStep, isFirstStep, consecutivePass)) {
			return Record(T(true, me, currentBoard));
		} 
		if (depth >= DepthLimit && lastBoard != currentBoard) {
			return Record(T(false, me, currentBoard));
		}
		Record best;
		const auto allActions = AllActions(opponent, lastBoard, currentBoard, isFirstStep, &actionSequence);
		const auto nextFinishedStep = finishedStep + 1;
		for (const auto& action : allActions) {
			const auto nextConsecutivePass = (!isFirstStep && lastBoard == currentBoard) && action.first == Action::Pass;
			const auto value = SearchMax(me, opponent, depth + 1, nextFinishedStep, false, currentBoard, action.second, nextConsecutivePass, alpha, beta);
			if (!best.HasValue || best.Evaluation.Compare(value.Evaluation) > 0) {
				best.Evaluation = value.Evaluation;
				best.HasValue = true;
				best.Action = action.first;
			}
			if (!beta.HasValue || beta.Evaluation.Compare(best.Evaluation) > 0) {
				beta = best;
			}
			if (beta.HasValue && alpha.HasValue && beta.Evaluation.Compare(alpha.Evaluation) <= 0) {
				break;
			}
		}
		Set(finishedStep, currentBoard, best.Evaluation, best.Action);
		return best;
	}
protected:
	Step DepthLimit = std::numeric_limits<Step>::max();

	virtual void StepInit(const Step finishedStep, const Board board) {

	}

	virtual bool Get(const Step finishedStep, const Board board, T& evaluation, Action& action) const {
		return false;
	}

	virtual void Set(const Step finishedStep, const Board board, const T& evaluation, const Action action) {

	}
public:
	AlphaBetaAgent(const ActionSequence& _actionSequence = DEFAULT_ACTION_SEQUENCE) : actionSequence(_actionSequence) {}

	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		auto player = MyPlayer(finishedStep);
		auto opponent = TurnUtil::Opponent(player);
		auto isFirstStep = IsFirstStep(player, lastBoard, currentBoard);
		StepInit(finishedStep, currentBoard);
		auto record = SearchMax(player, opponent, 0, finishedStep, isFirstStep, lastBoard, currentBoard, false, Record(), Record());
		return record.Action;
	}
};

class StoneCountAlphaBetaEvaluation {
private:
	signed char PartialScoreAdvantage = 0;
	signed char PartialScore = 0;
	signed char LibertyAdvantage = 0;
	signed char Liberty = 0;
public:
	StoneCountAlphaBetaEvaluation() {}

	StoneCountAlphaBetaEvaluation(const bool gameFinished, const Player player, const Board currentBoard) {
		const auto& score = Score::PartialScore(currentBoard);
		if (gameFinished) {
			const auto final = FinalScore(score);
			switch (player) {
			case Player::Black:
				PartialScoreAdvantage = final.Black > final.White ? std::numeric_limits<decltype(PartialScoreAdvantage)>::max() : std::numeric_limits<decltype(PartialScoreAdvantage)>::min();
				break;
			case Player::White:
				PartialScoreAdvantage = final.White > final.Black ? std::numeric_limits<decltype(PartialScoreAdvantage)>::max() : std::numeric_limits<decltype(PartialScoreAdvantage)>::min();
				break;
			default:
				break;
			}
		} else {
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
	}

	int Compare(const StoneCountAlphaBetaEvaluation& other) const {
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

class StoneCountAlphaBetaAgent : public AlphaBetaAgent<StoneCountAlphaBetaEvaluation> {
private:
	typedef StoneCountAlphaBetaEvaluation T;
	mutable StorageManager<T> caches;
	array<Step, TOTAL_POSITIONS> depthLimits;//index by num stones
protected:

	virtual void StepInit(const Step finishedStep, const Board board) override {
		caches.ClearAll();
	}

	virtual bool Get(const Step finishedStep, const Board board, T& evaluation, Action& action) const override {
		::Record<T> find;
		
		if (caches.Get(finishedStep, board, find)) {
			action = ActionMapping::EncodedToAction(find.BestAction);
			evaluation = find.Eval;
			return true;
		} else {
			return false;
		}
	}

	virtual void Set(const Step finishedStep, const Board board, const T& evaluation, const Action action) override {
		caches.Set(finishedStep, board, ::Record<T>(ActionMapping::ActionToEncoded(action), evaluation));
	}
public:
	StoneCountAlphaBetaAgent(const Step _depthLimit) : caches("") {
		depthLimits.fill(_depthLimit);
	}

	StoneCountAlphaBetaAgent(const array<Step, TOTAL_POSITIONS>& _depthLimits): caches(""), depthLimits(_depthLimits) {}

	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		const auto numStone = Score::Stones(currentBoard);
		const auto stones = numStone.Black + numStone.White;
		DepthLimit = depthLimits[stones];
#ifdef INTERACT_MODE
		cout << "Alpha-beta search depth: " << static_cast<int>(DepthLimit) << endl;
#endif
		return AlphaBetaAgent<StoneCountAlphaBetaEvaluation>::Act(finishedStep, lastBoard, currentBoard);
	}
};