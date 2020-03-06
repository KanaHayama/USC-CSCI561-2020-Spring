
#define COLLECT_STORAGE_HIT_RATE

#include "external_storage.h"
#include "cache_storage.h"
#include "full_search.h"

const int MAX_NUM_THREAD = 112;

class Thread {
private:
	RecordManager& Store;
	array<std::unique_ptr<thread>, MAX_NUM_THREAD> Threads{ nullptr };
	int ThreadNum = 0;

	void Search(int id) {
		cout << "Thread " << id + 1 << " started" << endl;
		srand(id);
		thread_local ActionSequence sequence = DEFAULT_ACTION_SEQUENCE;
		std::random_shuffle(sequence.begin(), sequence.end());
		FullSearcher searcher(Store, sequence, Tokens.at(id));
		searcher.Start();
		cout << "Thread " << id + 1 << " exit" << endl;
	}
public:
	Thread(RecordManager& _store) : Store(_store) {}

	array<volatile bool, MAX_NUM_THREAD> Tokens{ false };

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
		cout << "\t" << "r: print report" << endl;
		cout << "\t" << "s: serialize" << endl;
		cout << "\t" << "d: deserialize" << endl;
		cout << "\t" << "t[1-24]: thread num" << endl;
		cout << "\t" << "b[1-24][cme]: switch backend [cache|memory|external]" << endl;
		cout << "\t" << "c[1-24]: clear storage" << endl;
		cout << "\t" << "s[1-24][tf]: set serialize flag" << endl;
	}

	static void Illegal() {
		cout << "* ILLEGAL INPUT *" << endl;
	}
};

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "no file prefix" << endl;
		return -1;
	}
	RecordManager record(argv[1]);
	Thread threads(record);
	auto serializeRe = std::regex("s(\\d+)([tf])");
	auto threadRe = std::regex("t(\\d+)");
	auto clearRe = std::regex("c(\\d+)");
	auto backendRe = std::regex("b(\\d+)([cme])");

	while (true) {
		cout << "Input: ";
		string line;
		std::getline(std::cin, line);
		bool paused = threads.GetSize() == 0;
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
			if (!threads.Resize(std::stoi(m.str(1)))) {
				SearchPrint::Illegal();
			}
		} else if (std::regex_search(line, m, clearRe)) {
			auto step = std::stoi(m.str(1));
			if (!paused || step < 0 || step > MAX_STEP) {
				SearchPrint::Illegal();
			} else {
				record.Clear(step);
			}
		} else if (std::regex_search(line, m, backendRe)) {
			if (!paused) {
				SearchPrint::Illegal();
			} else {
				auto step = std::stoi(m.str(1));
				if (m.str(2).compare("m") == 0) {
					record.SwitchBackend(step, std::make_shared<MemoryRecordStorage>());
				} else if (m.str(2).compare("c") == 0) {
					auto capacity = 50000000;
					record.SwitchBackend(step, std::make_shared<CacheRecordStorage>(capacity, thread::hardware_concurrency() * 2));
				} else if (m.str(2).compare("e") == 0) {
					auto blockSize = std::min(128, 1 << (step - 3));//step 24 -> 4G * 2
					record.SwitchBackend(step, std::make_shared<ExternalRecordStorage>(blockSize, blockSize));
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