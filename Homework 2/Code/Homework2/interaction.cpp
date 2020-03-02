
#include "game_host.h"
#include "visualization.h"

using std::cin;

class InteractionPrint {
public:
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
		Visualization::Status(finishedStep, lastBoard, currentBoard);
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

			} else if (line.compare("e") == 0) {
				exit(0);
			} else if (line.compare("b") == 0) {
				Visualization::Status(finishedStep, lastBoard, currentBoard);
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

void PlayGame() {
	auto random = RandomAgent();
	auto human = HumanAgent();

	std::unique_ptr<Host> host = nullptr;
	while (host == nullptr) {
		cout << "Profiles:" << endl;
		cout << "\t" << "X: X vs Rand" << endl;
		cout << "\t" << "O: O vs Rand" << endl;
		cout << "You play: ";
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

	auto winStatus = host->RunToEnd();
	cout << "Game Finished ========================" << endl;
	cout << "Final board:" << endl;
	Visualization::B(std::get<1>(winStatus));
	cout << "Score: X=" << std::get<2>(winStatus).Black << " O=" << std::get<2>(winStatus).White << endl;
	cout << "Winner: " << (std::get<0>(winStatus) == Player::Black ? "X" : "O") << endl;
}

void VisualizeRecord() {
	cout << "Input Board HEX: ";
	string hex;
	cin >> hex;
	auto board = static_cast<Board>(std::stoull(hex, nullptr, 16));
	Visualization::B(board);
}

int main(int argc, char* argv[]) {
	cout << "Select function:" << endl;
	cout << "\t" << "1. Play Game" << endl;
	cout << "\t" << "2. Visualize Record" << endl;
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