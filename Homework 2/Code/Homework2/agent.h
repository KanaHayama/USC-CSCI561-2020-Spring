#pragma once

#include "agent_abstract.h"
#include "full_search_eval.h"

class MyAgent : public StoneCountAlphaBetaAgent {
private:
	mutable StorageManager<FullSearchEvaluation> truth;
public:
	MyAgent() : StoneCountAlphaBetaAgent({ 5, 5, 5, 5, 5, 5, 5, 5, 6, 7, 8, 9, 10, 11, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }), truth(""){}

	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		/*
		assert(finishedStep != 0 || (lastBoard == EMPTY_BOARD && currentBoard == EMPTY_BOARD));
		if (lastBoard == EMPTY_BOARD && currentBoard == EMPTY_BOARD) {
			return Action::P22;
		}
		*/
		return StoneCountAlphaBetaAgent::Act(finishedStep, lastBoard, currentBoard);
	}
};