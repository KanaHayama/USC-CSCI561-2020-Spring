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

public:
	Record Rec;

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

class FullSearcher {
private:
	RecordManager& Store;
	const ActionSequence& actions;
	const volatile bool& Token;

	inline static void Update(Record& current, const Action action, const Record& after) {
		assert(static_cast<int>(after.OpponentStepToWin) + 1 <= static_cast<int>(std::numeric_limits<EncodedAction>::max()));
		assert(static_cast<int>(after.SelfStepToWin) + 1 <= static_cast<int>(std::numeric_limits<EncodedAction>::max()));
		auto tempSelfStepToWin = after.OpponentStepToWin + 1;
		auto tempOpponentStepToWin = after.SelfStepToWin + 1;
		auto uninitializedFlag = current.SelfStepToWin > MAX_STEP || current.OpponentStepToWin > MAX_STEP;
		auto winEarlierFlag = tempSelfStepToWin < current.SelfStepToWin && tempOpponentStepToWin > tempSelfStepToWin;
		auto loseFlag = current.SelfStepToWin > current.OpponentStepToWin;
		auto extendLoseFlag = current.OpponentStepToWin < tempOpponentStepToWin;
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
						Update(current.Rec, after.GetOpponentAction(), fetch);
					} else {//proceed
						stack.emplace_back(after);
					}
					continue;
				} else {
					if (noValidAction) {//dead end, opponent win
						current.Rec.OpponentStepToWin = 0;
					}
				}
			}
			//normal update ancestor
			if (ancestor != nullptr) {
				Update(ancestor->Rec, current.GetOpponentAction(), current.Rec);
			}
			//store record
			assert(current.Rec.SelfStepToWin <= MAX_STEP || current.Rec.OpponentStepToWin <= MAX_STEP);
			Store.Set(current.GetFinishedStep(), current.GetCurrentBoard(), current.Rec);
			stack.pop_back();
		}
	}

};