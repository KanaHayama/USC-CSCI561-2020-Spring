
#include "cache_storage.h"
#include "full_search.h"

const int MAX_NUM_THREAD = 112;

class Thread {
private:
	const Step& startMiniMaxFinishedStep;
	const Step& startCutOffFinishedStep;

	StorageManager<WinEval>& Store;
	array<std::unique_ptr<thread>, MAX_NUM_THREAD> Threads{ nullptr };
	int ThreadNum = 0;

	void Search(int id) {
		cout << "Thread " << id + 1 << " started" << endl;
		srand(id);
		thread_local ActionSequence sequence = DEFAULT_ACTION_SEQUENCE;
		std::random_shuffle(sequence.begin(), sequence.end());
		FullSearcher searcher(Store, sequence, startMiniMaxFinishedStep, startCutOffFinishedStep, Tokens.at(id));
		searcher.Start();
		cout << "Thread " << id + 1 << " exit" << endl;
	}
public:
	Thread(StorageManager<WinEval>& _store, const Step& _startMiniMaxFinishedStep, const Step& _startCutOffFinishedStep) : Store(_store), startMiniMaxFinishedStep(_startMiniMaxFinishedStep), startCutOffFinishedStep(_startCutOffFinishedStep){}

	array<bool, MAX_NUM_THREAD> Tokens{ false };

	bool Resize(const unsigned char num) {
		if (num > MAX_NUM_THREAD) {
			return false;
		}
		if (num < ThreadNum) {
			for (auto i = num; i < ThreadNum; i++) {
				Tokens[i] = true;
			}
			for (auto i = num; i < ThreadNum; i++) {
				Threads[i]->join();
				Tokens[i] = false;
				Threads[i] = nullptr;
			}
		} else if (num > ThreadNum) {
			for (auto i = ThreadNum; i < num; i++) {
				Threads[i] = std::make_unique<thread>(&Thread::Search, this, i);
			}
		}
		ThreadNum = num;
		return true;
	}

	int GetSize() const {
		return ThreadNum;
	}
};

class SearchPrint {
public:
	static void Help() {
		cout << "Commands:" << endl;
		cout << "\t" << "e: execute" << endl;
		cout << "\t" << "p: pause" << endl;
		cout << "\t" << "q: quit" << endl;
		cout << "\t" << "c: clear screen" << endl;
		cout << "\t" << "h: clear hit rate" << endl;
		cout << "\t" << "r: print report" << endl;
		cout << "\t" << "s: serialize" << endl;
		cout << "\t" << "d: deserialize" << endl;
		cout << "\t" << "t[0-24]: thread num" << endl;
		cout << "\t" << "b[0-24][cme]: switch backend [cache|memory|external]" << endl;
		cout << "\t" << "c[0-24]: clear storage" << endl;
		cout << "\t" << "s[0-24][tf]: set serialize flag" << endl;
		cout << "\t" << "o[0-24]: cut-off start depth" << endl;
		cout << "\t" << "m[0-24]: minimax start depth" << endl;
	}

	static void Illegal() {
		cout << "* ILLEGAL INPUT *" << endl;
	}
};

Step startMiniMaxFinishedStep = MAX_STEP;
Step startCutOffFinishedStep = 2;

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "no file prefix" << endl;
		return -1;
	}
	StorageManager<WinEval> record(argv[1]);
	auto threads = std::make_shared<Thread>(record, startMiniMaxFinishedStep, startCutOffFinishedStep);
	auto serializeRe = std::regex("s(\\d+)([tf])");
	auto threadRe = std::regex("t(\\d+)");
	auto clearRe = std::regex("c(\\d+)");
	auto backendRe = std::regex("b(\\d+)([cm])");
	auto minimaxRe = std::regex("m(\\d+)");
	auto cutoffRe = std::regex("o(\\d+)");
	while (true) {
		cout << "Input: ";
		string line;
		std::getline(std::cin, line);
		bool paused = threads->GetSize() == 0;
		auto m = std::smatch();
		if (line.compare("") == 0) {

		} else if (line.compare("c") == 0) {
			system("CLS");
		} else if (line.compare("h") == 0) {
			record.ClearAllHitRate();
		} else if (line.compare("e") == 0) {
			threads->Resize(1);
		} else if (line.compare("p") == 0) {
			threads->Resize(0);
		} else if (line.compare("q") == 0) {
			threads->Resize(0);
			break;
		} else if (line.compare("r") == 0 || line.compare(" ") == 0) {
			cout << "Current Cut-off start step: " << int(startCutOffFinishedStep) << endl;
			cout << "Current Minimax start step: " << int(startMiniMaxFinishedStep) << endl;
			record.Report();
		} else if (line.compare("s") == 0) {
			if (!paused) {
				SearchPrint::Illegal();
			} else {
				record.Serialize();
			}
		} else if (line.compare("d") == 0) {
			record.Deserialize();
		} else if (std::regex_search(line, m, serializeRe)) {
			auto step = std::stoi(m.str(1));
			auto sw = m.str(2).compare("t") == 0;
			if (step < 0 || step > MAX_STEP) {
				SearchPrint::Illegal();
			} else {
				record.EnableSerialize(step, sw);
			}
		} else if (std::regex_search(line, m, threadRe)) {
			if (!threads->Resize(std::stoi(m.str(1)))) {
				SearchPrint::Illegal();
			}
		} else if (std::regex_search(line, m, clearRe)) {
			auto step = std::stoi(m.str(1));
			if (!paused || step < 0 || step > MAX_STEP) {
				SearchPrint::Illegal();
			} else {
				cout << "Clear step: " << int(step) << endl;
				record.Clear(step);
			}
		} else if (std::regex_search(line, m, minimaxRe)) {
			auto newStartStep = std::stoi(m.str(1));
			if (newStartStep < 0 || newStartStep > MAX_STEP) {
				SearchPrint::Illegal();
			} else {
				startMiniMaxFinishedStep = newStartStep;
				cout << "Change minimax search start step to " << int(startMiniMaxFinishedStep) << endl;
			}
		} else if (std::regex_search(line, m, cutoffRe)) {
			auto newStartStep = std::stoi(m.str(1));
			if (newStartStep < 0 || newStartStep > MAX_STEP) {
				SearchPrint::Illegal();
			} else {
				startCutOffFinishedStep = newStartStep;
				cout << "Change cut-off start step to " << int(startCutOffFinishedStep) << endl;
			}
		} else if (std::regex_search(line, m, backendRe)) {
			if (!paused) {
				SearchPrint::Illegal();
			} else {
				auto step = std::stoi(m.str(1));
				if (m.str(2).compare("m") == 0) {
					record.SwitchBackend(step, std::make_shared<MemoryRecordStorage<WinEval>>());
				} else if (m.str(2).compare("c") == 0) {
					cout << "cache capacity: ";
					long capacity = 0;
					cin >> capacity;
					record.SwitchBackend(step, std::make_shared<CacheRecordStorage<WinEval>>(capacity, thread::hardware_concurrency() * 2));
				} else {
					SearchPrint::Illegal();
				}
			}
		} else {
			SearchPrint::Help();
		}
	}
	return 0;
}