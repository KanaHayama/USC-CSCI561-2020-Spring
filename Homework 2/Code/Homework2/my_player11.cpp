/*
Name: Zongjian Li
USC ID: 6503378943
*/


#include "agent.h"
#include "visualization.h"

const static string INPUT_FILENAME = "input.txt";
const static string OUTPUT_FILENAME = "output.txt";
const static string STEP_SPECULATION_FILENAME = "step.txt";
const static std::chrono::seconds STEP_TIME_LIMIT = std::chrono::seconds(10);
const static std::chrono::seconds CAN_TRY_NEXT_STEP_TIME_LIMIT = STEP_TIME_LIMIT / 3;
const static std::chrono::milliseconds SAFE_WRITE_STEP_TIME_LIMIT = std::chrono::milliseconds(9750);

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
		file.write(reinterpret_cast<const char*>(&nextFinishedStep), sizeof(nextFinishedStep));
		file.close();
	}
};

bool TryAgent(const std::chrono::time_point<std::chrono::high_resolution_clock>& start, const Step finishedStep, const Input& input, std::shared_ptr<Agent>& agent) {
	auto write_safe = false;
	auto action = agent->Act(finishedStep, input.Last, input.Current);
	const auto stop = std::chrono::high_resolution_clock::now();
	auto step = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	if (step <= SAFE_WRITE_STEP_TIME_LIMIT) {
		Writter::Write(action);
		write_safe = true;
	}

	//Visualization
	if (write_safe) {
		auto afterBoard = input.Current;
		if (action != Action::Pass) {
			afterBoard = ActionUtil::ActWithoutCaptureWithoutIncStep(input.Current, input.Player, action);
			Capture::TryApply(afterBoard, static_cast<Position>(action));
		}
		Visualization::Action(action);
		Visualization::Liberty(afterBoard);
		Visualization::FinalScore(afterBoard);
		Visualization::Time(step, std::chrono::milliseconds::zero());
	} else {
		cout << "MOVE TIME EXCEED: result not valid" << endl;
	}

	return write_safe && step <= CAN_TRY_NEXT_STEP_TIME_LIMIT;
}

int main(int argc, char* argv[]) {
	const auto start = std::chrono::high_resolution_clock::now();
	const auto input = Reader::Read();
	const auto finishedStep = StepSpeculator::Speculate(input);
	StepSpeculator::WriteStep(finishedStep);
	Action action;
	std::shared_ptr<Agent> pAgent;
	Visualization::Step(finishedStep);
	Visualization::Player(input.Player);
	if (finishedStep <= 10) {//can not be lower!
		//TODO: Search Archived best moves
		if (false) {

		} else {
			auto depth = 4;//can always finish with depth 5, 6 exceed
			bool result;
			do {
				pAgent = std::make_shared<LookupStoneCountAlphaBetaAgent>(depth);//TODO: keep loaded evaluation in memory
				cout << "------ Try search depth: " << depth << " ------" << endl;
				result = TryAgent(start, finishedStep, input, pAgent);
				depth++;
			} while (result);
		}
	} else {
		pAgent = std::make_shared<WinStepAlphaBetaAgent>();
		TryAgent(start, finishedStep, input, pAgent);
	}
	return 0;
}