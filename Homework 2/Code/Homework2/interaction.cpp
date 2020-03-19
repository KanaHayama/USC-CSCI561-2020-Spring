
#include "game_host.h"
#include "agent.h"
#include "best.h"

using std::cin;
#pragma region Play Game
class InteractionPrint {
public:
	static void Help() {
		cout << "Commands:" << endl;
		cout << "\t" << "c: clear" << endl;
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
		Visualization::Step(finishedStep);
		Visualization::Status(lastBoard, currentBoard);
		Visualization::Liberty(currentBoard);
		Visualization::FinalScore(currentBoard);
		auto player = TurnUtil::WhoNext(finishedStep);
		Visualization::Player(player);
		auto allActions = LegalActionIterator::ListAll(player, lastBoard, currentBoard, finishedStep == 0, &DEFAULT_ACTION_SEQUENCE);
		if (allActions.empty()) {
			Visualization::NoLegalMove();
			return Action::Pass;
		}
		Visualization::LegalMoves(allActions);

		auto re = std::regex("([01234])\\W([01234])");
		while (true) {
			cout << "Input: ";
			string line;
			std::getline(std::cin, line);
			auto m = std::smatch();
			if (line.compare("") == 0) {

			} else if (line.compare("c") == 0) {
				system("CLS");
			} else if (line.compare("e") == 0) {
				exit(0);
			} else if (line.compare("b") == 0) {
				Visualization::Status(lastBoard, currentBoard);
			} else if (line.compare("m") == 0) {
				Visualization::LegalMoves(allActions);
			} else if (line.compare("l") == 0) {
				auto isomorphism = Isomorphism(currentBoard);
				Visualization::Isomorphism(isomorphism);
			} else if (line.compare("p") == 0) {
				if (Has(allActions, Action::Pass)) {
					return Action::Pass;
				}
				Visualization::IllegalInput();
			} else if (std::regex_search(line, m, re)) {
				auto i = std::stoi(m.str(1));
				auto j = std::stoi(m.str(2));
				auto action = PlainAction(i, j).Convert();
				if (Has(allActions, action)) {
					return action;
				}
				Visualization::IllegalInput();
			} else {
				InteractionPrint::Help();
			}
		}
	}
};

int SetRound() {
	system("CLS");
	cout << "Round: ";
	int round;
	cin >> round;
	return round;
}

std::shared_ptr<StoneCountAlphaBetaAgent> SetStoneCountAlphaBetaAgent() {
	while (true) {
		system("CLS");
		cout << "Set Search Depth: ";
		int depth;
		cin >> depth;
		return std::make_shared<LookupStoneCountAlphaBetaAgent>(depth);
	}
}

std::shared_ptr<Agent> SelectAgent(const Player player) {
	while (true) {
		system("CLS");
		cout << "Select Player " << (player == Player::Black ? "X" : "O") << " Agent: " << endl;
		cout << "\t" << "1: Human" << endl;
		cout << "\t" << "2: Random" << endl;
		cout << "\t" << "3: Greedy" << endl;
		cout << "\t" << "4: Aggressive" << endl;
		cout << "\t" << "5: Alpha-Beta (Count Stone)" << endl;
		cout << "\t" << "6: Comprehensive (My Agent)" << endl;
		char agent;
		cin >> agent;
		switch (agent) {
		case '1':
			return std::make_shared<HumanAgent>();
		case '2':
			return std::make_shared<RandomAgent>();
		case '3':
			return std::make_shared<GreedyAgent>();
		case '4':
			return std::make_shared<AggressiveAgent>();
		case '5':
			return SetStoneCountAlphaBetaAgent();
		//case '6':
			//return std::make_shared<MyAgent>();
		}
	}
}

void SetHost(std::unique_ptr<Host>& host) {
PRINT:
	while (true) {
		system("CLS");
		cout << "Print Step [y/n] ?: ";
		char v;
		cin >> v;
		switch (v) {
		case 'y':
			host -> SetPrintStep(true);
			goto BOARD;
		case 'n':
			host->SetPrintStep(false);
			goto BOARD;
		}
	}
BOARD:
	while (true) {
		system("CLS");
		cout << "Set Board [y/n] ?: ";
		char v;
		cin >> v;
		int finishedStep;
		string boardHex;
		Board board;
		string actionLiteral;
		Action action;
		auto re = std::regex("([01234])\\W([01234])");
		auto m = std::smatch();
		int i, j;
		bool success;
		switch (v) {
		case 'y':
			system("CLS");
			cout << "Finished Step: ";
			cin >> finishedStep;
			cout << "Board Hex: ";
			cin >> boardHex;
			board = static_cast<Board>(std::stoull(boardHex, nullptr, 16));
			cout << "Action [ p | i,j ]: ";
			do {
				std::getline(cin, actionLiteral);
			} while (actionLiteral.compare("") == 0);
			if (actionLiteral.compare("p") == 0) {
				action = Action::Pass;
			} else if (std::regex_search(actionLiteral, m, re)) {
				i = std::stoi(m.str(1));
				j = std::stoi(m.str(2));
				action = PlainAction(i, j).Convert();
			} else {
				Visualization::IllegalInput();
				exit(-1);
			}
			success = host->SetBoard(static_cast<Step>(finishedStep), board, action);
			if (!success) {
				Visualization::IllegalInput();
				exit(-1);
			}
			goto END;
		case 'n':
			host->ResetBoard();
			goto END;
		}
	}
END:
	;
}

void PlayGame() {
	std::unique_ptr<Host> host = nullptr;
	auto black = SelectAgent(Player::Black);
	auto white = SelectAgent(Player::White);
	auto blackWin = 0;
	auto whiteWin = 0;
	auto round = SetRound();
	host = std::make_unique<Host>(*black, *white);
	SetHost(host);
	for (auto i = 0; i < round; i++) {
		auto winStatus = host->RunToEnd();
		cout << "Round " << i + 1 << " Finished ========================" << endl;
		cout << "Final board:" << endl;
		Visualization::B(std::get<1>(winStatus));
		Visualization::FinalScore(std::get<2>(winStatus));
		auto winner = std::get<0>(winStatus);
		cout << "Winner: " << (winner == Player::Black ? "X" : "O") << endl;
		switch (winner) {
		case Player::Black:
			blackWin++;
			break;
		case Player::White:
			whiteWin++;
			break;
		}
		host->ResetBoard();
	}
	cout << "Statics =========================" << endl;
	cout << "\t" << "X win: " << blackWin << " " << (blackWin > whiteWin ? "<-" : "") << endl;
	cout << "\t" << "O win: " << whiteWin << " " << (whiteWin > blackWin ? "<-" : "") << endl;
}
#pragma endregion

void VisualizeRecord() {
	cout << "Input Board HEX: ";
	string hex;
	cin >> hex;
	auto board = static_cast<Board>(std::stoull(hex, nullptr, 16));
	cout << "Input: " << endl;
	Visualization::B(board);
	cout << "Standard: " << endl;
	Visualization::B(Isomorphism::Isomorphism(board).IndexBoard());
}
/*
class BestActionConverter {
private:
	using E = FullSearchEvaluation;
	static map<Board, E> Read(const string& filename) {
		map<Board, E> result;
		ifstream file(filename, std::ios::binary);
		assert(file.is_open());
		UINT64 size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size));
		for (auto i = 0ull; i < size; i++) {
			Board board;
			E record;
			file.read(reinterpret_cast<char*>(&board), sizeof(board));
			file.read(reinterpret_cast<char*>(&record), sizeof(record));
			result[board] = record;
		}
		file.close();
		return result;
	}

	static void Write(const string& filename, const map<Board, BestMove>& m) {
		ofstream file(filename, std::ios::binary);
		assert(file.is_open());
		UINT64 size = m.size();
		file.write(reinterpret_cast<const char*>(&size), sizeof(size));
		for (auto it = m.begin(); it != m.end(); it++) {
			file.write(reinterpret_cast<const char*>(&it->first), sizeof(it->first));
			file.write(reinterpret_cast<const char*>(&it->second), sizeof(it->second));
		}
		file.close();
	}

	static map<Board, BestMove> Calc(const Step finishedStep, const map<Board, E>& source, const map<Board, E>& lookup) {
		map<Board, BestMove> result;
		auto player = TurnUtil::WhoNext(finishedStep);
		for (auto it = source.begin(); it != source.end(); it++) {
			const Board& b = it->first;
			assert(Isomorphism(b).IndexBoard() == b);
			const E& e = it->second;
			auto actions = LegalActionIterator::ListAll(player, EMPTY_BOARD, b, b == EMPTY_BOARD, &DEFAULT_ACTION_SEQUENCE);
			E bestE;
			Action bestA;
			int bestCount = 0;
			for (const auto& a : actions) {
				const auto& action = a.first;
				const auto& next = a.second;
				const auto standard = Isomorphism(next).IndexBoard();
				auto find = lookup.find(standard);
				assert(find != lookup.end());
				const auto& origionalE = find->second;
				auto e = E(origionalE.OpponentWinAfterStep, origionalE.SelfWinAfterStep);
				auto cmp = bestE.Compare(e);
				if (cmp < 0) {
					bestE = e;
					bestA = action;
					bestCount = 1;
				}
				if (cmp == 0) {
					bestCount++;
				}
			}
			assert(bestCount > 0);
			if (bestCount > 1) {
				cout << bestCount << " best actions" << endl;
			}
			BestMove r;
			r.Win = bestE.Win();
			r.Action = ActionMapping::ActionToEncoded(bestA);
			result[b] = r;
		}
		return result;
	}
public:
	static void Convert(const string& prefix, const int begin, const int end) {
		auto next = ;

		for (auto step = begin; step < end; step++) {

		}
	}
	
};

void ConvertBestAction() {

}
*/
int main(int argc, char* argv[]) {
	cout << "Select function:" << endl;
	cout << "\t" << "1: Play Game" << endl;
	cout << "\t" << "2: Visualize Record" << endl;
	int i;
	cin >> i;
	system("CLS");
	switch (i) {
	case 1:
		PlayGame();
		break;
	case 2:
		VisualizeRecord();
		break;
	}
	return 0;
}