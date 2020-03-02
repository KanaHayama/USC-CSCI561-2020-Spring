#pragma once

#include "storage_manager.h"

class SearchState {
private:
	int nextActionIndex = 0;
	vector<std::pair<Action, Board>> actions;
	const ActionSequence* actionSequencePtr = nullptr;
	Step finishedStep = INITIAL_FINISHED_STEP;
	Board currentBoard = EMPTY_BOARD;
	Action opponentAction = Action::Pass;
	bool getThisStateByOpponentPass;

public:
	Record Rec;

	SearchState() = default;
	SearchState(const Action _opponent, const Step _finishedStep, const Board _lastBoard, const Board _currentBoard, const ActionSequence* _actionSequencePtr) : getThisStateByOpponentPass(_opponent == Action::Pass), opponentAction(_opponent), finishedStep(_finishedStep), actions(LegalActionIterator::ListAll(TurnUtil::WhoNext(_finishedStep), _lastBoard, _currentBoard, _finishedStep == INITIAL_FINISHED_STEP, _actionSequencePtr)), actionSequencePtr(_actionSequencePtr), currentBoard(_currentBoard) {}

	inline Step GetFinishedStep() const {
		return finishedStep;
	}

	inline Board GetCurrentBoard() const {
		return currentBoard;
	}

	inline Action GetOpponentAction() const {
		return opponentAction;
	}

	inline bool GetThisStateByOpponentPassing() const {
		return getThisStateByOpponentPass;
	}

	bool Next(SearchState& next) {
		if (nextActionIndex >= actions.size()) {
			return false;
		}
		auto& a = actions[nextActionIndex];
		next = SearchState(a.first, finishedStep + 1, currentBoard, a.second, actionSequencePtr);
		nextActionIndex++;
		return true;
	}
};

class FullSearcher {
private:
	RecordManager& Store;
	const ActionSequence& actions;
	const volatile bool& Token;

	inline static void Update(Record& current, const Action action, const Record& after) {
		assert(static_cast<int>(after.OpponentStepToWin) + 1 <= static_cast<int>(std::numeric_limits<EncodedAction>::max()));
		assert(static_cast<int>(after.SelfStepToWin) + 1 <= static_cast<int>(std::numeric_limits<EncodedAction>::max()));
		const auto tempSelfStepToWin = after.OpponentStepToWin + 1;
		const auto tempOpponentStepToWin = after.SelfStepToWin + 1;
		const auto uninitializedFlag = current.SelfStepToWin > MAX_STEP || current.OpponentStepToWin > MAX_STEP;
		const auto winEarlierFlag = tempSelfStepToWin < current.SelfStepToWin && tempOpponentStepToWin > tempSelfStepToWin;
		const auto loseFlag = current.SelfStepToWin > current.OpponentStepToWin;
		const auto extendLoseFlag = current.OpponentStepToWin < tempOpponentStepToWin;
		if (uninitializedFlag || winEarlierFlag || (loseFlag && extendLoseFlag)) {//with opponent's best reaction, I can still have posibility to win
			current.BestAction = ActionMapping::ActionToEncoded(action);
			current.SelfStepToWin = tempSelfStepToWin;
			current.OpponentStepToWin = tempOpponentStepToWin;
		}
	}
public:
	FullSearcher(RecordManager& _store, const ActionSequence& _actions, const volatile bool& _token) : Store(_store), actions(_actions), Token(_token) {}

	void Start() {
		vector<SearchState> stack;
		stack.reserve(MAX_STEP);
		stack.emplace_back(Action::Pass, INITIAL_FINISHED_STEP, EMPTY_BOARD, EMPTY_BOARD, &actions);
		while (!stack.empty() && !Token) {
			auto& const current = stack.back();
			const Step finishedStep = current.GetFinishedStep();
			const bool noninitialStep = finishedStep >= 1;
			SearchState* const ancestor = noninitialStep ? &stack.rbegin()[1] : nullptr;
			if (finishedStep == MAX_STEP || (noninitialStep && current.GetOpponentAction() == Action::Pass && ancestor->GetOpponentAction() == Action::Pass)) {//current == Black, min == White
				auto winStatus = Score::Winner(current.GetCurrentBoard());
				if (winStatus.first == TurnUtil::WhoNext(finishedStep)) {
					current.Rec.SelfStepToWin = 0;
				} else {
					current.Rec.OpponentStepToWin = 0;
				}
			} else {
				SearchState after;
				if (current.Next(after)) {
					Record fetch;
					if ((current.GetThisStateByOpponentPassing() && after.GetOpponentAction() == Action::Pass) || !Store.Get(after.GetFinishedStep(), after.GetCurrentBoard(), fetch)) {//always check 2 passings before lookup => we can use records only if we do not want to or cannot finish game now by 2 passings
						stack.emplace_back(after);
					} else {//proceed
						Update(current.Rec, after.GetOpponentAction(), fetch);
					}
					continue;
				}
			}
			//normal update ancestor
			if (noninitialStep) {
				Update(ancestor->Rec, current.GetOpponentAction(), current.Rec);
			}
			//store record
			assert(current.Rec.SelfStepToWin <= MAX_STEP || current.Rec.OpponentStepToWin <= MAX_STEP);
			if (!(current.GetThisStateByOpponentPassing() && current.Rec.BestAction == ENCODED_ACTION_PASS)) {//do not store success by using 2 passings => we can use stored records iff we are not taking advantage of opponent's passing mistake (or our dead ends)
				Store.Set(current.GetFinishedStep(), current.GetCurrentBoard(), current.Rec);
			}
			stack.pop_back();
		}
	}

};