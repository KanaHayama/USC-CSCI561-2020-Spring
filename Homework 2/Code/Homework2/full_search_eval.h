#pragma once

#include "go_abstract.h"

class FullSearchEvaluation {
public:
	Step SelfWinAfterStep = INFINITY_STEP;
	Step OpponentWinAfterStep = INFINITY_STEP;

	FullSearchEvaluation() {}
	FullSearchEvaluation(const Step _selfStepToWin, const Step _opponentStepToWin) : SelfWinAfterStep(_selfStepToWin), OpponentWinAfterStep(_opponentStepToWin) {}
	FullSearchEvaluation(const bool gameFinished, const Step finishedStep, const Player player, const Board currentBoard) {
		assert(gameFinished);
		auto winStatus = Score::Winner(currentBoard);
		if (winStatus.first == player) {
			SelfWinAfterStep = finishedStep;
		} else {
			OpponentWinAfterStep = finishedStep;
		}
	}

	bool Validate() const {
		return SelfWinAfterStep <= MAX_STEP || OpponentWinAfterStep <= MAX_STEP;
	}

	int Compare(const FullSearchEvaluation& other) const {//the better the larger
		const auto initialized = Initialized();
		const auto otherInitialized = other.Initialized();
		if (initialized && !otherInitialized) {
			return 1;
		} else if (!initialized && otherInitialized) {
			return -1;
		} else if (initialized && otherInitialized) {
			const auto win = Win();
			const auto otherWin = other.Win();
			if (win && !otherWin) {
				return 1;
			} else if (!win && otherWin) {
				return -1;
			} if (win && otherWin) {
				if (SelfWinAfterStep < other.SelfWinAfterStep) {// win earlier
					return 1;
				} else if (SelfWinAfterStep > other.SelfWinAfterStep) {
					return -1;
				}
			} else {
				assert(!win && !otherWin);
				if (OpponentWinAfterStep > other.OpponentWinAfterStep) {// lose later
					return 1;
				} else if (OpponentWinAfterStep < other.OpponentWinAfterStep) {
					return -1;
				}
			}
		}
		return 0;
	}

	inline bool Initialized() const {
		return SelfWinAfterStep <= MAX_STEP || OpponentWinAfterStep <= MAX_STEP;
	}

	inline bool Win() const {
		return SelfWinAfterStep < OpponentWinAfterStep;
	}

	bool operator == (const FullSearchEvaluation& other) const {
		return Compare(other) == 0;
	}
};

std::ostream& operator<<(std::ostream& os, const FullSearchEvaluation& evaluation) {
	os << "{self: " << (evaluation.SelfWinAfterStep > MAX_STEP ? "N/A" : std::to_string(evaluation.SelfWinAfterStep)) << ", oppo: " << (evaluation.OpponentWinAfterStep > MAX_STEP ? "N/A" : std::to_string(evaluation.OpponentWinAfterStep)) << "}";
	return os;
}
