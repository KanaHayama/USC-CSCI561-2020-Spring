#pragma once

#include "go.h"
#include <iomanip>

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
	static void Step(const ::Step finishedStep) {
		cout << "Step: " << finishedStep + 1 << " =========================================" << endl;
	}

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
		cout << "Hex: " << std::hex << board << std::dec << endl;
	}

	static void LegalMoves(const std::vector<std::pair<Action, State>>& actions) {
		cout << "Moves: ";
		for (auto& action : actions) {
			auto plain = PlainAction(action.first);
			cout << plain.ToString() << " ";
		}
		cout << endl;
	}

	static void Status(const Board currentBoard) {
		Visualization::B(currentBoard);
	}

	static void Status(const Board lastBoard, const Board currentBoard) {
		cout << "Last:" << endl;
		Visualization::B(lastBoard);
		cout << "Current:" << endl;
		Visualization::B(currentBoard);
	}

	static void StatusFull(const Board lastBoard, const Board currentBoard, const Board after) {
		cout << "Last:" << endl;
		Visualization::B(lastBoard);
		cout << "Current:" << endl;
		Visualization::B(currentBoard);
		cout << "After:" << endl;
		Visualization::B(after);
	}

	static void Player(const ::Player player) {
		cout << "Player: " << (player == ::Player::Black ? "X" : "O") << endl;
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

	static void FinalScore(const FinalScore& finalScore) {
		cout << "Score (KOMI):" << endl;
		cout << "\t" << "X: " << finalScore.Black << "\t" << (finalScore.Black > finalScore.White ? "<-" : "") << endl;
		cout << "\t" << "O: " << finalScore.White << "\t" << (finalScore.White > finalScore.Black ? "<-" : "") << endl;
	}

	static void FinalScore(const Board board) {
		auto winner = Score::Winner(board);
		FinalScore(winner.second);
	}

	static void Action(const Action action) {
		cout << "Move: " << PlainAction(action).ToString() << endl;
	}

	static void Time(const std::chrono::milliseconds step, const std::chrono::milliseconds total) {
		cout << "Time: " << step.count() << "/" << total.count() << " milliseconds" << endl;
	}

	static void Liberty(const Board board) {
		auto liberty = LibertyUtil::Liberty(board);
		if (liberty.Black == 0 && liberty.White == 0) {
			return;
		}
		cout << "Total Liberty:" << endl;
		cout << "\t" << "X: " << static_cast<int>(liberty.Black) << "\t" << (liberty.Black > liberty.White ? "<-" : "") << endl;
		cout << "\t" << "O: " << static_cast<int>(liberty.White) << "\t" << (liberty.White > liberty.Black ? "<-" : "") << endl;
	}

	static void BestActions(const vector<::Action>& actions) {
		if (actions.size() > 0) {
			cout << "Known best actions (" << actions.size() << "): ";
			for (auto action : actions) {
				cout << PlainAction(action).ToString() << " ";
			}
			cout << endl;
		} else {
			cout << "Best action not found" << endl;
		}
	}
};