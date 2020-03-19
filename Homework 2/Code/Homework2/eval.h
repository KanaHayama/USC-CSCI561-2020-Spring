#pragma once

#include "go_abstract.h"

class FullSearchEvaluation {
public:
	Step SelfWinAfterStep = INFINITY_STEP;
	Step OpponentWinAfterStep = INFINITY_STEP;

	FullSearchEvaluation() {}
	FullSearchEvaluation(const Step _selfStepToWin, const Step _opponentStepToWin) : SelfWinAfterStep(_selfStepToWin), OpponentWinAfterStep(_opponentStepToWin) {}
	FullSearchEvaluation(const bool gameFinished, const Step finishedStep, const Player player, const Board currentBoard) {
		if (!gameFinished) {
			return;
		}
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

	inline void Push(const FullSearchEvaluation& value) {

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

class StoneCountAlphaBetaEvaluation {
private:
	Player player = Player::Black;
	Board board = EMPTY_BOARD;
	bool territoryAvailable = false;
	bool libertyAvailable = false;

	FullSearchEvaluation Final = FullSearchEvaluation();

	signed char PartialScoreAdvantage = 0;
	signed char PartialScore = 0;

	signed char TerritoryAdvantage = 0;
	signed char Territory = 0;
	
	signed char LibertyAdvantage = 0;
	signed char Liberty = 0;
	
	inline void PrepareTerritory() {
		if (territoryAvailable) {
			return;
		}
		auto filled = Score::FillEmptyPositions(board);
		auto territory = Score::PartialScore(filled);
		switch (player) {
		case Player::Black:
			TerritoryAdvantage = territory.Black - territory.White;
			Territory = territory.Black;
			break;
		case Player::White:
			TerritoryAdvantage = territory.White - territory.Black;
			Territory = territory.White;
			break;
		default:
			break;
		}
		territoryAvailable = true;
	}
	
	inline void PrepareLiberty() {
		if (libertyAvailable) {
			return;
		}
		auto liberty = LibertyUtil::Liberty(board);
		switch (player) {
		case Player::Black:
			LibertyAdvantage = liberty.Black - liberty.White;
			Liberty = liberty.Black;
			break;
		case Player::White:
			LibertyAdvantage = liberty.White - liberty.Black;
			Liberty = liberty.White;
			break;
		default:
			break;
		}
		libertyAvailable = true;
	}
public:
	StoneCountAlphaBetaEvaluation() {}

	StoneCountAlphaBetaEvaluation(const bool gameFinished, const Step finishedStep, const Player _player, const Board _currentBoard) : player(_player), board(_currentBoard) {
		const auto& score = Score::PartialScore(_currentBoard);
		if (gameFinished) {
			const auto final = FinalScore(score);
			auto winner = final.Black > final.White ? Player::Black : Player::White;
			if (player == winner) {
				Final.SelfWinAfterStep = finishedStep;
			} else {
				Final.OpponentWinAfterStep = finishedStep;
			}
		}
		switch (player) {
		case Player::Black:
			PartialScoreAdvantage = score.Black - score.White;
			PartialScore = score.Black;
			break;
		case Player::White:
			PartialScoreAdvantage = score.White - score.Black;
			PartialScore = score.White;
			break;
		default:
			break;
		}
	}

	bool Validate() const {
		return true;
	}

	inline void Push(const StoneCountAlphaBetaEvaluation& value) {

	}

	int Compare(StoneCountAlphaBetaEvaluation& other) {//the better the larger
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
		//cmp local advantage
		if (PartialScoreAdvantage < other.PartialScoreAdvantage) {
			return -1;
		} else if (PartialScoreAdvantage > other.PartialScoreAdvantage) {
			return 1;
		}

		PrepareTerritory();
		other.PrepareTerritory();
		if (TerritoryAdvantage < other.TerritoryAdvantage) {
			return -1;
		} else if (TerritoryAdvantage > other.TerritoryAdvantage) {
			return 1;
		}
		/*
		PrepareLiberty();
		other.PrepareLiberty();
		if (LibertyAdvantage < other.LibertyAdvantage) {
			return -1;
		} else if (LibertyAdvantage > other.LibertyAdvantage) {
			return 1;
		}
		*/
		//cmp local abs
		if (PartialScore < other.PartialScore) {
			return -1;
		} else if (PartialScore > other.PartialScore) {
			return 1;
		}

		if (Territory > other.Territory) {
			return -1;
		} else if (Territory < other.Territory) {
			return 1;
		}
		/*
		if (Liberty < other.Liberty) {
			return -1;
		} else if (Liberty > other.Liberty) {
			return 1;
		}
		*/
		return 0;
	}

	bool operator == (StoneCountAlphaBetaEvaluation& other) {
		return Compare(other) == 0;
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

	EvaluationTrace(const bool gameFinished, const Step finishedStep, const Player _player, const Board _currentBoard) : EvaluationTrace(E(gameFinished, finishedStep, _player, _currentBoard)) {}

	//EvaluationTrace(const bool gameFinished, const Step finishedStep, const Player player, const Board currentBoard) : EvaluationTrace(E(gameFinished, finishedStep, player, currentBoard)){}

	inline void Push(const EvaluationTrace<E>& value) {
		trace[count] = value.Dominance();
		count++;
	}

	inline E Dominance() const {
		assert(count >= 1);
		return trace[0];
	}

	bool Validate() const {
		return Dominance().Validate();
	}

	int Compare(EvaluationTrace& other) {//the better the larger
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

	bool operator == (EvaluationTrace& other) {
		return Compare(other) == 0;
	}
};