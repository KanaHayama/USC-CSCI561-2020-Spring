#pragma once

#include "go_abstract.h"

class Visualization {
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
		auto c = '\0';
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
		for (auto i = 0; i < BOARD_SIZE; i++) {
			DrawHeader(i);
			for (auto j = 0; j < BOARD_SIZE; j++) {
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
				cout << int(plain.I) << "," << int(plain.J) << " ";
			}
		}
		cout << endl;
	}

	static void Status(const Step finishedStep, const Board lastBoard, const Board currentBoard) {
		system("CLS");
		cout << "Step: " << finishedStep + 1 << " =========================================" << endl;
		cout << "Last:" << endl;
		Visualization::B(lastBoard);
		cout << "Current:" << endl;
		Visualization::B(currentBoard);
	}

	static void Player(const Player player) {
		cout << "Player: " << (player == Player::Black ? "X" : "O") << endl;
	}

	static void IllegalInput() {
		cout << "* ILLEGAL INPUT *" << endl;
	}

	static void NoLegalMove() {
		cout << " * NO LEGAL MOVE *" << endl;
	}

	static void Isomorphism(const Isomorphism& isomorphism) {
		cout << "Original:" << endl;
		Visualization::B(isomorphism.Boards[0]);
		cout << "Rotate 90:" << endl;
		Visualization::B(isomorphism.Boards[1]);
		cout << "Rotate 180:" << endl;
		Visualization::B(isomorphism.Boards[2]);
		cout << "Rotate 270:" << endl;
		Visualization::B(isomorphism.Boards[3]);
		cout << "Transpose:" << endl;
		Visualization::B(isomorphism.Boards[4]);
		cout << "Transpose Rotate 90:" << endl;
		Visualization::B(isomorphism.Boards[5]);
		cout << "Transpose Rotate 180:" << endl;
		Visualization::B(isomorphism.Boards[6]);
		cout << "Transpose Rotate 270:" << endl;
		Visualization::B(isomorphism.Boards[7]);
	}
};