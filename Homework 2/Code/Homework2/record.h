#pragma once

#include "go_abstract.h"

class Record {
public:
	EncodedAction BestAction = ENCODED_ACTION_PASS;
	Step SelfWinAfterStep = INFINITY_STEP;
	Step OpponentWinAfterStep = INFINITY_STEP;
	Record() {}
	Record(const EncodedAction _action, const Step _selfStepToWin, const Step _opponentStepToWin) : BestAction(_action), SelfWinAfterStep(_selfStepToWin), OpponentWinAfterStep(_opponentStepToWin) {}

	bool operator == (const Record& other) const {
		return BestAction == other.BestAction && SelfWinAfterStep == other.SelfWinAfterStep && OpponentWinAfterStep == other.OpponentWinAfterStep;
	}
};

std::ostream& operator<<(std::ostream& os, const Record& record) {
	os << "{act: (" << ActionMapping::EncodedToPlain(record.BestAction) << "), self: " << (record.SelfWinAfterStep > MAX_STEP ? "N/A" : std::to_string(record.SelfWinAfterStep)) << ", oppo: " << (record.OpponentWinAfterStep > MAX_STEP ? "N/A" : std::to_string(record.OpponentWinAfterStep)) << "}";
	return os;
}

