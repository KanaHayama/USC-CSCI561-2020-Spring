#pragma once

#include <iostream>
#include <regex>
#include <cstdlib>
#include "simulation.h"

using std::cout;
using std::endl;
using std::string;

class Host {
private:
	std::array<Board, MAX_STEP + 1> Boards;
	std::array<Action, MAX_STEP + 1> Actions;
	Step FinishedStep = 0;
	bool Finished = false;

	Agent& Black;
	Agent& White;

public:

	Host(Agent& _black, Agent& _white) : Black(_black), White(_white) {
		Boards[0] = 0;
		Actions[0] = Action::Pass;
	}

	std::tuple<bool, Player, Board> RunOneStep() {
		assert(FinishedStep < MAX_STEP);
		assert(!Finished);
		auto currentStep = FinishedStep + 1;
		auto player = TurnUtil::WhoNext(FinishedStep);
		auto currentBoard = Boards[FinishedStep];
		auto isFirstStep = FinishedStep == 0;
		auto lastBoard = isFirstStep ? 0 : Boards[FinishedStep - 1];
		auto lastAction = Actions[FinishedStep];
		Action currentAction;
		switch (player) {
		case Player::Black:
			currentAction = Black.Act(currentStep, lastBoard, currentBoard);
			break;
		case Player::White:
			currentAction = White.Act(currentStep, lastBoard, currentBoard);
			break;
		}
		Actions[currentStep] = currentAction;
		if (Rule::ViolateEmptyRule(currentBoard, currentAction) || Rule::ViolateNoConsecutivePassingRule(isFirstStep, lastBoard, currentBoard, currentAction)) {
			Finished = true;
			return std::make_tuple(true, TurnUtil::Opponent(player), 0);
		}
		auto afterBoard = currentBoard;
		if (currentAction != Action::Pass) {
			afterBoard = ActionUtil::ActWithoutCaptureWithoutIncStep(currentBoard, player, currentAction);
			auto hasLiberty = Capture::TryApply(afterBoard, static_cast<Position>(currentAction));
			if (Rule::ViolateNoSuicideRule(hasLiberty) || Rule::ViolateKoRule(isFirstStep, lastBoard, afterBoard)) {
				Finished = true;
				return std::make_tuple(true, TurnUtil::Opponent(player), 0);
			}
		}
		Boards[currentStep] = afterBoard;
		FinishedStep = currentStep;
		if (FinishedStep == MAX_STEP) {
			Finished = true;
			auto winner = Score::Winner(afterBoard);
			return std::make_tuple(true, winner.first, afterBoard);
		}
		return std::make_tuple(false, player, afterBoard);
	}

	Player RunToEnd() {
		while (true) {
			auto stepResult = RunOneStep();
			if (std::get<0>(stepResult)) {
				return std::get<1>(stepResult);
			}
		}
	}
};

class Print {
private:
	static void DrawHeader() {
		cout << "  | 0 | 1 | 2 | 3 | 4 |" << endl;
	}

	static void DrawHorizontalLine() {
		cout << string(23, '-') << endl;
	}

	static void DrawHeader(const int i) {
		cout << i << " |";
	}

	static void DrawCell(const PositionState pos) {
		char c;
		switch (pos) {
		case PositionState::Black:
			c = 'X';
			break;
		case PositionState::White:
			c = 'O';
			break;
		case PositionState::Empty:
			c = ' ';
			break;
		}
		cout << " " << c << " |";
	}

	static void DrawTailer() {
		cout << endl;
	}
public:
	static void B(const Board board) {
		auto plain = PlainState(static_cast<State>(board));
		DrawHorizontalLine();
		DrawHeader();
		DrawHorizontalLine();
		for (auto i = 0; i < SIZE; i++) {
			DrawHeader(i);
			for (auto j = 0; j < SIZE; j++) {
				auto occupy = plain.Occupy[i][j];
				auto rawPlayer = plain.Player[i][j];
				auto state = !occupy ? PositionState::Empty : static_cast<PositionState>(rawPlayer);
				DrawCell(state);
			}
			DrawTailer();
			DrawHorizontalLine();
		}
	}

	static void LegalMoves(const std::vector<std::pair<Action, State>>& actions) {
		cout << "Moves: ";
		for (auto& action : actions) {
			auto plain = PlainAction(action.first);
			if (plain.Pass) {
				cout << "PASS";
			} else {
				cout << plain.I << "," << plain.J;
			}
		}
		cout << endl;
	}

	static void Status(const Step finishedStep, const Board lastBoard, const Board currentBoard) {
		system("CLS");
		cout << "Step: " << finishedStep + 1 << " =========================================" << endl;
		cout << "Last:" << endl;
		Print::B(lastBoard);
		cout << "Current:" << endl;
		Print::B(currentBoard);
	}

	static void Player(const Player player) {
		cout << "Player: " << (player == Player::Black ? "X" : "O") << endl;
	}

	static void IllegalInput() {
		cout << "* ILLEGAL INPUT *" << endl;
	}

	static void Isomorphism(const Isomorphism& isomorphism) {
		cout << "Original:" << endl;
		Print::B(isomorphism.R0);
		cout << "Rotate 90:" << endl;
		Print::B(isomorphism.R90);
		cout << "Rotate 180:" << endl;
		Print::B(isomorphism.R180);
		cout << "Rotate 270:" << endl;
		Print::B(isomorphism.R270);
		cout << "Transpose:" << endl;
		Print::B(isomorphism.T);
		cout << "Transpose Rotate 90:" << endl;
		Print::B(isomorphism.TR90);
		cout << "Transpose Rotate 180:" << endl;
		Print::B(isomorphism.TR180);
		cout << "Transpose Rotate 270:" << endl;
		Print::B(isomorphism.TR270);
	}

	static void Help() {
		cout << "Commands:" << endl;
		cout << "\t" << "e: exit" << endl;
		cout << "\t" << "b: board" << endl;
		cout << "\t" << "m: moves" << endl;
		cout << "\t" << "l: isomorphism" << endl;
		cout << "\t" << "p: pass" << endl;
		cout << "\t" << "i,j: place" << endl;
	}
};

class HumanAgent : public Agent {
private:
	static bool Has(const std::vector<std::pair<Action, State>>& actions, const Action action) {
		for (auto a : actions) {
			if (a.first == action) {
				return true;
			}
		}
		return false;
	}

public:
	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		Print::Status(finishedStep, lastBoard, currentBoard);

		auto player = TurnUtil::WhoNext(finishedStep);
		Print::Player(player);

		auto allActions = LegalActionIterator::ListAll(player, lastBoard, currentBoard, finishedStep == 0);
		Print::LegalMoves(allActions);
		
		while (true) {
			cout << "Input: ";
			string line;
			std::getline(std::cin, line);
			auto re = std::regex("[01234]\\W[01234]");
			auto m = std::smatch();
			auto flag = false;
			if (line.compare("e")) {
				exit(0);
			} else if (line.compare("b")) {
				Print::Status(finishedStep, lastBoard, currentBoard);
			} else if (line.compare("m")) {
				Print::LegalMoves(allActions);
			} else if (line.compare("l")) {
				auto isomorphism = Isomorphism(currentBoard);
				Print::Isomorphism(isomorphism);
			} else if (line.compare("p")) {
				if (Has(allActions, Action::Pass)) {
					return Action::Pass;
				}
				Print::IllegalInput();
			} else if (std::regex_search(line, m, re)) {
				auto i = std::stoi(m.str(1));
				auto j = std::stoi(m.str(2));
				auto action = PlainAction(i, j).Convert();
				if (Has(allActions, action)) {
					return action;
				}
				Print::IllegalInput();
			} else {
				Print::Help();
			}
		}
	}
};


inline int run(int argc, char* argv[]) {
	auto random = RandomAgent();
	auto human = HumanAgent();
	
	std::unique_ptr<Host> host = nullptr;
	while (host == nullptr) {
		cout << "Profiles:" << endl;
		cout << "\t" << "X: X vs Rand" << endl;
		cout << "\t" << "O: O vs Rand" << endl;
		cout << "You play: " << endl;
		char who;
		std::cin >> who;
		switch (who) {
		case 'X':
			host = std::make_unique<Host>(human, random);
			break;
		case 'O':
			host = std::make_unique<Host>(random, human);
			break;
		}
	}

	auto winner = host->RunToEnd();
	cout << "Winner: " << (winner == Player::Black ? "X" : "O") << endl;
	return 0;
}