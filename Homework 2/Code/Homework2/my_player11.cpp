/*
Name: Zongjian Li
USC ID: 6503378943
*/


#include "agent.h"
#include "visualization.h"

const static string INPUT_FILENAME = "input.txt";
const static string OUTPUT_FILENAME = "output.txt";
const static string STEP_SPECULATION_FILENAME = "step.txt";
const static string TIMER_FILENAME = "timer.txt";
const static string GAME_COUNT_FILENAME = "count.txt";

class Input {
public:
	const ::Player Player;
	const Board Last;
	const Board Current;
	Input(const ::Player _player, const Board _last, const Board _current) : Player(_player), Last(_last), Current(_current) {}
};

class Reader {
private:

	static Player ReadPlayer(ifstream& file) {
		int p;
		file >> p;
		switch (p) {
		case 1:
			return Player::Black;
		case 2:
			return Player::White;
		default:
			throw std::runtime_error("unknown player:" + std::to_string(p));
		}
	}

	static Board ReadBoard(ifstream& file) {
		PlainState plain(EMPTY_BOARD);
		for (auto i = 0; i < BOARD_SIZE; i++) {
			auto line = string("");
			while (line.compare("") == 0) {
				std::getline(file, line);
			}
			for (auto j = 0; j < BOARD_SIZE; j++) {
				auto v = line[j];
				switch (v) {
				case '0':
					break;
				case '1':
					plain.Occupy[i][j] = true;
					plain.Player[i][j] = Player::Black;
					break;
				case '2':
					plain.Occupy[i][j] = true;
					plain.Player[i][j] = Player::White;
					break;
				default:
					throw std::runtime_error("unknown stone: char(" + std::to_string(v) + ")");
				}
			}
		}
		return plain.Convert();
	}
public :
	static Input Read() {
		ifstream file(INPUT_FILENAME);
		auto player = ReadPlayer(file);
		auto last = ReadBoard(file);
		auto current = ReadBoard(file);
		file.close();
		return Input(player, last, current);
	}
};

class Writter {
public:
	static void Write(const PlainAction action) {
		ofstream file(OUTPUT_FILENAME);
		file << action.ToString();
		file.close();
	}
};

class StepSpeculator {
private:
	static pair<bool, Step> ReadFile() {
		ifstream file(STEP_SPECULATION_FILENAME, std::ios::binary);
		if (!file.is_open()) {
			return std::make_pair(false, Step());
		}
		Step step;
		file.read(reinterpret_cast<char*>(&step), sizeof(step));
		file.close();
		if (step >= MAX_STEP) {
			return std::make_pair(false, Step());
		}
		//cout << "read finished step: " << int(step) << endl;
		return std::make_pair(true, step);
	}
public:
	static Step Speculate(const Input& input) {
		auto f = ReadFile();
		//step 1
		if (input.Current == EMPTY_BOARD && input.Last == EMPTY_BOARD) {
			if (input.Player == Player::Black) {
				return 0;
			} else {
				return 1;// black pass
			}
		}
		auto stone = Score::Stones(input.Current);
		Step total = stone.Black + stone.White;
		//step2 
		if (input.Player == Player::White && stone.White == 0) {
			if (stone.Black == 1 || stone.Black == 0) {
				return 1;
			}
			
		}
		//have record
		if (f.first) {
			if (TurnUtil::WhoNext(f.second) == input.Player) {
				return f.second;
			}
			
		}

		// guess
		cout << "WARRING: SPECULATED STEP" << endl;
		if (TurnUtil::WhoNext(total) == input.Player) {
			return total;
		} else {
			return total + 1;
		}
		
	}

	static void WriteStep(const Step finishedStep) {
		auto nextFinishedStep = finishedStep + 2;
		if (nextFinishedStep >= MAX_STEP) {
			nextFinishedStep = INFINITY_STEP;
		}
		ofstream file(STEP_SPECULATION_FILENAME, std::ios::binary);
		//cout << "write next next step: " << nextFinishedStep << endl;
		file.write(reinterpret_cast<const char*>(&nextFinishedStep), sizeof(nextFinishedStep));
		file.close();
	}
};

class Timer {
public:
	static std::chrono::milliseconds Read() {
		ifstream file(TIMER_FILENAME, std::ios::binary);
		if (!file.is_open()) {
			return std::chrono::milliseconds::zero();
		}
		std::chrono::milliseconds result;
		file.read(reinterpret_cast<char*>(&result), sizeof(result));
		file.close();
		return result;
	}

	static void Write(const std::chrono::milliseconds time) {
		ofstream file(TIMER_FILENAME, std::ios::binary);
		file.write(reinterpret_cast<const char*>(&time), sizeof(time));
		file.close();
		return;
	}
};

int main(int argc, char* argv[]) {
	const auto start = std::chrono::high_resolution_clock::now();
	const auto input = Reader::Read();
	const auto finishedStep = StepSpeculator::Speculate(input);
	StepSpeculator::WriteStep(finishedStep);
	Action action;
	std::shared_ptr<Agent> pAgent;
	if (finishedStep <= 10) {//can not be lower!
		pAgent = std::make_shared<LookupStoneCountAlphaBetaAgent>(5);//6 exceed
	} else {
		pAgent = std::make_shared<WinStepAlphaBetaAgent>();
	}
	action = pAgent->Act(finishedStep, input.Last, input.Current);
	Writter::Write(action);
	const auto stop = std::chrono::high_resolution_clock::now();
	auto accumulate = Timer::Read();;
	auto step = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	auto total = accumulate + step;
	Timer::Write(total);

	//Visualization
	auto afterBoard = input.Current;
	if (action != Action::Pass) {
		afterBoard = ActionUtil::ActWithoutCaptureWithoutIncStep(input.Current, input.Player, action);
		Capture::TryApply(afterBoard, static_cast<Position>(action));
	}
	Visualization::Step(finishedStep);//Visualization::StatusFull(finishedStep, input.Last, input.Current, afterBoard);
	Visualization::Player(input.Player);
	Visualization::Action(action);
	Visualization::Liberty(afterBoard);
	Visualization::FinalScore(afterBoard);
	Visualization::Time(step, total);

	if (step > std::chrono::seconds(9)) {
		cout << "MOVE TIME EXCEED" << endl;
		ofstream file("TIME_EXCEED.TXT");
		file << step.count() << " seconds" << endl;
		file.close();
	}
	return 0;
}