#pragma once


#include "simulation.h"

inline int run(int argc, char* argv[]) {
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
	InteractionPrint::B(std::get<1>(winStatus));
	cout << "Score: X=" << std::get<2>(winStatus).Black << " O=" << std::get<2>(winStatus).White << endl;
	cout << "Winner: " << (std::get<0>(winStatus) == Player::Black ? "X" : "O") << endl;
	return 0;
}