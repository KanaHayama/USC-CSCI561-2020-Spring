#pragma once

#include <functional>

#include "go_abstract.h"
#include "agent_abstract.h"
#include "visualization.h"

class Host {
private:
	std::array<Board, MAX_STEP + 1> Boards;
	std::array<Action, MAX_STEP + 1> Actions;
	Step FinishedStep = 0;
	bool Finished = false;
	bool PrintStep = false;

	Agent& Black;
	Agent& White;

	std::tuple<bool, Player, Board, FinalScore> RunOneStep(std::function<Action(const Step, const Board, const Board)>& func) {
		assert(FinishedStep < MAX_STEP);
		assert(!Finished);
		auto currentStep = FinishedStep + 1;
		auto player = TurnUtil::WhoNext(FinishedStep);
		auto currentBoard = Boards[FinishedStep];
		auto isFirstStep = FinishedStep == 0;
		auto lastBoard = isFirstStep ? 0 : Boards[FinishedStep - 1];
		auto lastAction = Actions[FinishedStep];
		auto currentAction = Action::Pass;
		if (PrintStep) {
			Visualization::Status(FinishedStep, lastBoard, currentBoard);
			Visualization::Liberty(currentBoard);
			Visualization::FinalScore(currentBoard);
		}
		currentAction = func(FinishedStep, lastBoard, currentBoard);
		if (PrintStep) {
			Visualization::Action(currentAction);
		}
		Actions[currentStep] = currentAction;
		if (Rule::ViolateEmptyRule(currentBoard, currentAction)) {
			if (PrintStep) {
				cout << "Game End: Violate Empty Rule" << endl;
			}
			Finished = true;
			return std::make_tuple(true, TurnUtil::Opponent(player), 0, FinalScore());
		}
		auto afterBoard = currentBoard;
		if (currentAction != Action::Pass) {
			afterBoard = ActionUtil::ActWithoutCaptureWithoutIncStep(currentBoard, player, currentAction);
			auto hasLiberty = Capture::TryApply(afterBoard, static_cast<Position>(currentAction));
			auto violateNoSuicideRule = Rule::ViolateNoSuicideRule(hasLiberty);
			auto violateKoRule = Rule::ViolateKoRule(isFirstStep, lastBoard, afterBoard);
			if (violateNoSuicideRule || violateKoRule) {
				if (PrintStep) {
					if (violateNoSuicideRule) {
						cout << "Game End: Violate No Suicide Rule" << endl;
					}
					if (violateKoRule) {
						cout << "Game End: Violate KO Rule" << endl;
					}
				}
				Finished = true;
				return std::make_tuple(true, TurnUtil::Opponent(player), 0, FinalScore());
			}
		}
		Boards[currentStep] = afterBoard;
		auto winStatus = Score::Winner(afterBoard);
		if (currentStep == MAX_STEP || (FinishedStep >= 1 && currentAction == Action::Pass && lastAction == Action::Pass)) {
			Finished = true;
			return std::make_tuple(true, winStatus.first, afterBoard, winStatus.second);
		}
		FinishedStep = currentStep;
		return std::make_tuple(false, player, afterBoard, winStatus.second);
	}

public:

	Host(Agent& _black, Agent& _white) : Black(_black), White(_white) {
		Boards[0] = 0;
		Actions[0] = Action::Pass;
	}

	std::tuple<bool, Player, Board, FinalScore> RunOneStep() {
		auto player = TurnUtil::WhoNext(FinishedStep);
		std::function<Action(const Step, const Board, const Board)> lambda;
		switch (player) {
		case Player::Black:
			lambda = [&](const Step finishedStep, const Board lastBoard, const Board currentBoard) {
				return Black.Act(finishedStep, lastBoard, currentBoard);
			};
			break;
		case Player::White:
			lambda = [&](const Step finishedStep, const Board lastBoard, const Board currentBoard) {
				return White.Act(finishedStep, lastBoard, currentBoard);
			};
			break;
		}
		return RunOneStep(lambda);
	}

	std::tuple<Player, Board, FinalScore> RunToEnd() {
		while (true) {
			auto stepResult = RunOneStep();
			if (std::get<0>(stepResult)) {
				return std::make_tuple(std::get<1>(stepResult), std::get<2>(stepResult), std::get<3>(stepResult));
			}
		}
	}

	void SetPrintStep(const bool value) {
		PrintStep = value;
	}

	bool SetBoard(const Step finishedStep, const Board board, const Action nextAction) {
		Finished = false;
		FinishedStep = finishedStep;
		Boards[FinishedStep] = board;
		std::function<Action(const Step, const Board, const Board)> lambda = [nextAction](const Step finishedStep, const Board lastBoard, const Board currentBoard) {
			return nextAction;
		};
		auto result = RunOneStep(lambda);
		if (PrintStep) {
			if (std::get<0>(result)) {
				cout << "* GAME FINISHED AFTER SET BOARD *" << endl;
			} else {
				cout << "AFTER SET BOARD -> ";
				Visualization::Status(FinishedStep, board, Boards[FinishedStep]);
			}
		}
		return !std::get<0>(result);
	}

	void ResetBoard() {
		FinishedStep = 0;
		Finished = false;
	}
};