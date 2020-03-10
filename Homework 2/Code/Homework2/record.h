#pragma once

#include "go_abstract.h"

template <typename E>
class Record {
public:
	EncodedAction BestAction = ENCODED_ACTION_PASS;
	E Eval;
	Record() : Eval() {}
	Record(const EncodedAction _action, const E& _e) : BestAction(_action), Eval(_e) {}

	bool operator == (const Record& other) const {
		return BestAction == other.BestAction && Eval == other.Eval;
	}
};

template <typename E>
std::ostream& operator<<(std::ostream& os, const Record<E>& record) {
	os << "{act: (" << ActionMapping::EncodedToPlain(record.BestAction) << "), eval: " << record << "}";
	return os;
}

