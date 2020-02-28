#pragma once

#include "simulation.h"

inline int run(int argc, char* argv[]) {
	RecordManager record;
	Thread threads(record);
	auto flagRe = std::regex("([suq])(\\d+)([tf])");
	auto threadRe = std::regex("t(\\d+)");
	auto clearRe = std::regex("c(\\d+)");
	
	while (true) {
		cout << "Input: ";
		string line;
		std::getline(std::cin, line);
		auto m = std::smatch();
		if (line.compare("") == 0) {

		} else if (line.compare("c") == 0) {
			system("CLS");
		} else if (line.compare("e") == 0) {
			threads.Resize(1);
		} else if (line.compare("p") == 0) {
			threads.Resize(0);
		} else if (line.compare("q") == 0) {
			threads.Resize(0);
			break;
		} else if (line.compare("r") == 0) {
			record.Report();
		} else if (line.compare("s") == 0) {
			record.Serialize();
		} else if (std::regex_search(line, m, flagRe)) {
			auto index = std::stoi(m.str(2)) - 1;
			auto sw = m.str(3).compare("t") == 0;
			if (index < 0 || index >= MAX_STEP) {
				SearchPrint::Illegal();
			}
			if (m.str(1).compare("s") == 0) {
				record.SkipSerialize[index] = sw;
			} else if (m.str(1).compare("u") == 0) {
				record.SkipUpdate[index] = sw;
			} else if (m.str(1).compare("q") == 0) {
				record.SkipQuery[index] = sw;
			}
		} else if (std::regex_search(line, m, threadRe)) {
			if (!threads.Resize(std::stoi(m.str(1)))) {
				SearchPrint::Illegal();
			}
		} else if (std::regex_search(line, m, clearRe)) {
			if (!record.Clear(std::stoi(m.str(1)))) {
				SearchPrint::Illegal();
			}
		} else {
			SearchPrint::Help();
		}
	}
	return 0;
}