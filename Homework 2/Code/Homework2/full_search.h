#pragma once

#include "full_search_eval.h"
#include "storage_manager.h"
#include "agent.h"

template <typename E>
class Record {
private:

public:
	bool BestActionIsPass = true;
	E Eval;
	Record() : Eval() {}
	Record(const E& _e) : Eval(_e) {}

	bool operator == (const Record& other) const {
		return Eval == other.Eval;
	}
};

template <typename E>
std::ostream& operator<<(std::ostream& os, const Record<E>& record) {
	os << "{act: (" << ActionMapping::EncodedToPlain(record.BestAction) << "), eval: " << record << "}";
	return os;
}

class SearchState {
private:
	int nextActionIndex = 0;
	vector<std::pair<Action, Board>> actions;
	const ActionSequence* actionSequencePtr = nullptr;
	Step finishedStep = INITIAL_FINISHED_STEP;
	Board currentBoard = EMPTY_BOARD;
	Action opponentAction = Action::Pass;
	bool getThisStateByOpponentPass = false;

public:
	Record<FullSearchEvaluation> Rec;

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
	StorageManager<FullSearchEvaluation>& Store;
	const ActionSequence& actionSequence;
	const volatile bool& Token;
	const Step startMiniMaxFinishedStep;

	inline static void Update(Record<FullSearchEvaluation>& current, const Action action, const Record<FullSearchEvaluation>& after) {
		assert(static_cast<int>(after.Eval.OpponentWinAfterStep) <= static_cast<int>(std::numeric_limits<EncodedAction>::max()));
		assert(static_cast<int>(after.Eval.SelfWinAfterStep) <= static_cast<int>(std::numeric_limits<EncodedAction>::max()));
		FullSearchEvaluation temp(after.Eval.OpponentWinAfterStep, after.Eval.SelfWinAfterStep);
		if (current.Eval.Compare(temp) < 0) {//with opponent's best reaction, I can still have posibility to win
			current.BestActionIsPass = action == Action::Pass;
			current.Eval = temp;
		}
	}
public:
	FullSearcher(StorageManager<FullSearchEvaluation>& _store, const ActionSequence& _actionSequence, const Step _startMiniMaxFinishedStep, const volatile bool& _token) : Store(_store), actionSequence(_actionSequence), startMiniMaxFinishedStep(_startMiniMaxFinishedStep), Token(_token){
		if (_startMiniMaxFinishedStep >= MAX_STEP) {
			cout << "TRUE FULL SEARCH ENABLED" << endl;
		}
	}

	void Start() {
		vector<SearchState> stack;
		stack.reserve(MAX_STEP + 1);
		stack.emplace_back(Action::Pass, INITIAL_FINISHED_STEP, EMPTY_BOARD, EMPTY_BOARD, &actionSequence);
		while (!stack.empty() && !Token) {
			auto& const current = stack.back();
			const Step finishedStep = current.GetFinishedStep();
			const bool noninitialStep = finishedStep >= 1;
			SearchState* const ancestor = noninitialStep ? &stack.rbegin()[1] : nullptr;
			if (finishedStep == MAX_STEP || (noninitialStep && current.GetOpponentAction() == Action::Pass && ancestor->GetOpponentAction() == Action::Pass)) {//current == Black, min == White
				current.Rec.BestActionIsPass = false;
				auto winStatus = Score::Winner(current.GetCurrentBoard());
				if (winStatus.first == TurnUtil::WhoNext(finishedStep)) {
					current.Rec.Eval.SelfWinAfterStep = finishedStep;
				} else {
					current.Rec.Eval.OpponentWinAfterStep = finishedStep;
				}
			} else {
				if (finishedStep == startMiniMaxFinishedStep) {
					auto agent = WinStepAlphaBetaAgent(actionSequence);// clear storage every emulation
					auto result = agent.AlphaBeta(finishedStep, noninitialStep ? ancestor->GetCurrentBoard() : EMPTY_BOARD, current.GetCurrentBoard());
					current.Rec.BestActionIsPass = result.first == Action::Pass;
					current.Rec.Eval = result.second;
				} else {
					assert(finishedStep < startMiniMaxFinishedStep);
					SearchState after;
					if (current.Next(after)) {
						FullSearchEvaluation fetch;
						if ((current.GetThisStateByOpponentPassing() && after.GetOpponentAction() == Action::Pass) || !Store.Get(after.GetFinishedStep(), after.GetCurrentBoard(), fetch)) {//always check 2 passings before lookup => we can use records only if we do not want to or cannot finish game now by 2 passings
							stack.emplace_back(after);
						} else {//proceed
							assert(fetch.SelfWinAfterStep <= MAX_STEP || fetch.OpponentWinAfterStep <= MAX_STEP);
							Update(current.Rec, after.GetOpponentAction(), fetch);
						}
						continue;
					}
				}
			}
			//normal update ancestor
			if (noninitialStep) {
				Update(ancestor->Rec, current.GetOpponentAction(), current.Rec);
			}
			//store record
			assert(current.Rec.Eval.Initialized());
			if (!(current.GetThisStateByOpponentPassing() && current.Rec.BestActionIsPass)) {//do not store success by using 2 passings => we can use stored records iff we are not taking advantage of opponent's passing mistake (or our dead ends)
				Store.Set(finishedStep, current.GetCurrentBoard(), current.Rec.Eval);
			}
			stack.pop_back();
		}
	}

};