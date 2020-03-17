#pragma once

#include "agent_abstract.h"
#include "full_search_eval.h"

class MyAgent : public StoneCountAlphaBetaAgent {
private:
	mutable StorageManager<FullSearchEvaluation> truth;
public:
	MyAgent() : StoneCountAlphaBetaAgent({ 7, 7, 7, 7, 7, 7, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }), truth(""){}
	//on test platform, 2 games, seconds
	//full search from idx 7:
	//7:67
	//full search from idx 6:
	//6:534

	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		return StoneCountAlphaBetaAgent::Act(finishedStep, lastBoard, currentBoard);
	}
};