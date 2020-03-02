#pragma once

#include "go_abstract.h"

class Record {
public:
	EncodedAction BestAction = ENCODED_ACTION_PASS;
	Step SelfStepToWin = STEP_COMP_INITIAL;
	Step OpponentStepToWin = STEP_COMP_INITIAL;
	Record() {}
	Record(const EncodedAction _action, const Step _selfStepToWin, const Step _opponentStepToWin) : BestAction(_action), SelfStepToWin(_selfStepToWin), OpponentStepToWin(_opponentStepToWin) {}

	bool operator == (const Record& other) const {
		return BestAction == other.BestAction && SelfStepToWin == other.SelfStepToWin && OpponentStepToWin == other.OpponentStepToWin;
	}
};

std::ostream& operator<<(std::ostream& os, const Record& record) {
	os << "{act: (" << ActionMapping::EncodedToPlain(record.BestAction) << "), self: " << (record.SelfStepToWin > MAX_STEP ? "N/A" : std::to_string(record.SelfStepToWin)) << ", oppo: " << (record.OpponentStepToWin > MAX_STEP ? "N/A" : std::to_string(record.OpponentStepToWin)) << "}";
	return os;
}

