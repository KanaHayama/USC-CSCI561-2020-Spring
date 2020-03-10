#pragma once

#include "go_abstract.h"

class FullSearchEvaluation {
public:
	Step SelfWinAfterStep = INFINITY_STEP;
	Step OpponentWinAfterStep = INFINITY_STEP;

	FullSearchEvaluation() {}
	FullSearchEvaluation(const Step _selfStepToWin, const Step _opponentStepToWin) : SelfWinAfterStep(_selfStepToWin), OpponentWinAfterStep(_opponentStepToWin) {}

	bool Validate() const {
		return SelfWinAfterStep <= MAX_STEP || OpponentWinAfterStep <= MAX_STEP;
	}

	bool operator == (const FullSearchEvaluation& other) const {
		return SelfWinAfterStep == other.SelfWinAfterStep && OpponentWinAfterStep == other.OpponentWinAfterStep;
	}
};

std::ostream& operator<<(std::ostream& os, const FullSearchEvaluation& evaluation) {
	os << "{self: " << (evaluation.SelfWinAfterStep > MAX_STEP ? "N/A" : std::to_string(evaluation.SelfWinAfterStep)) << ", oppo: " << (evaluation.OpponentWinAfterStep > MAX_STEP ? "N/A" : std::to_string(evaluation.OpponentWinAfterStep)) << "}";
	return os;
}
