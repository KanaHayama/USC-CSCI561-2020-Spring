/*
Name: Zongjian Li
USC ID: 6503378943
*/

#include "agent.h"
#include "visualization.h"


/*===================================================================================================================//
//                                                                                                                   //
//                                                                                                                   //
//                                           !!!!!!  CHANGE THIS  !!!!!!                                             //
//                                                                                                                   //
//                                                                                                                   //
=====================================================================================================================*/

//#define GRADING
#define SUBMISSION

const static string INPUT_FILENAME = "input.txt";
const static string OUTPUT_FILENAME = "output.txt";
const static string STEP_SPECULATION_FILENAME = "step.txt";
const static string TIMER_FILENAME = "timer.txt";
const static string TERMINATION_TEST_FILENAME = "terminate.txt";
const static string GAME_COUNTER_FILENAME = "count.txt";

const static seconds GRADING_TOTAL_TIME_LIMIT = seconds(7200);
const static int GRADING_TOTAL_GAME = 200;

const static int MOVE_EACH_GAME = MAX_STEP / 2;
const static seconds SINGLE_MOVE_TIME_LIMIT = seconds(10);
const static seconds STATIC_AVERAGE_STEP_TIME_LIMIT = seconds(3);
const static seconds TOTAL_TIME_LIMIT_RESERVED_TIME = SINGLE_MOVE_TIME_LIMIT;
const static milliseconds SAFE_WRITE_STEP_TIME_LIMIT = duration_cast<milliseconds>(SINGLE_MOVE_TIME_LIMIT - milliseconds(150));
const static milliseconds MOVE_RESERVED_TIME = milliseconds(300);

const static array<Step, TOTAL_POSITIONS> SAFE_SEARCH_DEPTH = {/*0*/ 4, 4, 4, 5, 5,/*5*/ 5, 5, 5, 5, 5, /*10*/5, 5, 5, 6, 6, /*15*/6, 6, 6, 255, 255, /*20*/255, 255, 255, 255, 255 };
const static int FORCE_FULL_SEARCH_STEP = 14;

#ifdef GRADING
static seconds TotalTimeLimit = GRADING_TOTAL_TIME_LIMIT - TOTAL_TIME_LIMIT_RESERVED_TIME;
static int TotalGame = GRADING_TOTAL_GAME;
#else
static seconds TotalTimeLimit = seconds(5400) - TOTAL_TIME_LIMIT_RESERVED_TIME;
static int TotalGame = 150;
#endif

void CorrectGradingConfig() {
	cout << "WARNING: TOTALS MAY BE WRONG" << endl;
	TotalTimeLimit = GRADING_TOTAL_TIME_LIMIT - TOTAL_TIME_LIMIT_RESERVED_TIME;
	TotalGame = GRADING_TOTAL_GAME;
}

inline milliseconds TrueMoveTimeLimit(const int currentGame, const int finishedStep, const milliseconds accumulate) {
	auto remainGame = TotalGame - currentGame;
	auto gameRemainMove = (MAX_STEP - finishedStep) / 2 + 1;//estimate higher
	auto totalRemainMove = remainGame * MOVE_EACH_GAME + gameRemainMove;
	auto totalRemainTime = duration_cast<milliseconds>(TotalTimeLimit) - accumulate;
	auto newAverageMoveTimeLimit = totalRemainTime / totalRemainMove;
	return std::min(newAverageMoveTimeLimit, duration_cast<milliseconds>(SINGLE_MOVE_TIME_LIMIT));
}

inline milliseconds AdjustedMoveTimeLimit(const milliseconds moveTimeLimit, const Step finishedStep) {
	auto result = moveTimeLimit;
	auto player = TurnUtil::WhoNext(finishedStep);
	if (player == Player::Black) {
		result = result + milliseconds(300);
	} else {
		//result = result - milliseconds(500);
	}
	if (3 < finishedStep && finishedStep < 7) {
		result = result + milliseconds(2000);
	}
	return std::min(result, duration_cast<milliseconds>(SINGLE_MOVE_TIME_LIMIT));
}

inline static milliseconds TryNextDepthThreadhold(const milliseconds moveTimeLimit) {
	return moveTimeLimit / 2;
}

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

class Timer {
public:
	static pair<milliseconds, milliseconds> Read() {
		ifstream file(TIMER_FILENAME, std::ios::binary);
		if (!file.is_open()) {
			return std::make_pair(milliseconds::zero(), milliseconds::zero());
		}
		milliseconds lastStart;
		milliseconds lastEnd;
		file.read(reinterpret_cast<char*>(&lastStart), sizeof(lastStart));
		file.read(reinterpret_cast<char*>(&lastEnd), sizeof(lastEnd));
		file.close();
		return std::make_pair(lastStart, lastEnd);
	}

	static void Write(const milliseconds start, const milliseconds end) {
		ofstream file(TIMER_FILENAME, std::ios::binary);
		file.write(reinterpret_cast<const char*>(&start), sizeof(start));
		file.write(reinterpret_cast<const char*>(&end), sizeof(end));
		file.close();
		return;
	}
};

class TerminationTest {
public:
	static bool TestLast(const bool first) {
		if (first) {
			return false;
		}
		ifstream file(TERMINATION_TEST_FILENAME, std::ios::binary);
		if (!file.is_open()) {
			return false;
		}
		file.close();//file exist;
		return true;
	}

	static void MarkRunning() {
		ofstream file(TERMINATION_TEST_FILENAME, std::ios::binary);
		file.close();
	}

	static void MarkFinished() {
		std::remove(TERMINATION_TEST_FILENAME.c_str());
	}
};

class GameCounter {
public:
	static int GetLast() {
		ifstream file(GAME_COUNTER_FILENAME, std::ios::binary);
		if (!file.is_open()) {
			return 0;
		}
		int count;
		file.read(reinterpret_cast<char*>(&count), sizeof(count));
		file.close();
		if (count > TotalGame) {
			CorrectGradingConfig();
		}
		return count;
	}

	static void Update(const int currentGameCount) {
		ofstream file(GAME_COUNTER_FILENAME, std::ios::binary);
		file.write(reinterpret_cast<const char*>(&currentGameCount), sizeof(currentGameCount));
		file.close();
	}
};

bool TryAgent(const milliseconds lastAccumulate, const int gameCount, const time_point<high_resolution_clock>& start, const Step finishedStep, const Input& input, std::shared_ptr<Agent>& agent) {
	auto write_safe = false;
	auto action = agent->Act(finishedStep, input.Last, input.Current);
	const auto stop = high_resolution_clock::now();
	auto moveTime = duration_cast<milliseconds>(stop - start);
	auto accumulate = lastAccumulate + moveTime;
	if (moveTime <= SAFE_WRITE_STEP_TIME_LIMIT) {
		Writter::Write(action);
		Timer::Write(lastAccumulate, accumulate);
		TerminationTest::MarkFinished();
		write_safe = true;
	}

	//Visualization
#ifndef SUBMISSION
	if (write_safe) {
		auto afterBoard = input.Current;
		if (action != Action::Pass) {
			afterBoard = ActionUtil::ActWithoutCaptureWithoutIncStep(input.Current, input.Player, action);
			Capture::TryApply(afterBoard, static_cast<Position>(action));
		}
		Visualization::Status(afterBoard);
		
		Visualization::Action(action);
		Visualization::FinalScore(afterBoard);
		Visualization::Time(moveTime, accumulate);
	} else {
		cout << "MOVE TIME EXCEED: result not valid" << endl;
	}
#endif

	return write_safe && moveTime <= TryNextDepthThreadhold(AdjustedMoveTimeLimit(TrueMoveTimeLimit(gameCount, finishedStep, accumulate), finishedStep));
}

int main(int argc, char* argv[]) {
	const auto start = std::chrono::high_resolution_clock::now();
	const auto input = Reader::Read();
	const auto finishedStep = StepSpeculator::Speculate(input);
	StepSpeculator::WriteStep(finishedStep);
	const auto accumulate = Timer::Read();
	auto trueAccumulate = accumulate.second;
	const auto lastTerminated = TerminationTest::TestLast(accumulate.second == milliseconds::zero());
	TerminationTest::MarkRunning();
	if (lastTerminated) {
		trueAccumulate = accumulate.first + SINGLE_MOVE_TIME_LIMIT;
	}
	trueAccumulate += MOVE_RESERVED_TIME;
	auto gameCount = GameCounter::GetLast();
	if (finishedStep == 0 || finishedStep == 1) {
		gameCount++;
		GameCounter::Update(gameCount);
	}

	//visualization
#ifndef SUBMISSION
	cout << "Game " << gameCount << ", ";
	Visualization::Step(finishedStep);
	Visualization::Player(input.Player);
	auto player = TurnUtil::WhoNext(finishedStep);
	Visualization::Status(input.Last, input.Current);
	Visualization::LegalMoves(LegalActionIterator::ListAll(player, input.Last, input.Current, finishedStep == 0, &DEFAULT_ACTION_SEQUENCE));
	cout << "Move time limit: " << TrueMoveTimeLimit(gameCount, finishedStep, trueAccumulate).count() << " milliseconds" << endl;
#endif

	std::shared_ptr<Agent> pAgent;
	//safe guard
	bool doIter = true;
	auto safeDepth = SAFE_SEARCH_DEPTH[finishedStep];
	if (finishedStep + safeDepth < MAX_STEP) {
#ifndef SUBMISSION
		cout << "-------- safe guard --------" << endl;
#endif
		pAgent = std::make_shared<LookupStoneCountAlphaBetaAgent>(safeDepth);
		doIter = TryAgent(trueAccumulate, gameCount, start, finishedStep, input, pAgent);
	}
	//try
	if (doIter) {
		//TODO: search archived best moves
		if (false) {

		} else {
			auto depth = safeDepth + 1;
			bool estimate;
			do {
				estimate = finishedStep < FORCE_FULL_SEARCH_STEP &&(finishedStep + depth) < MAX_STEP;
				if (estimate) {
					pAgent = std::make_shared<LookupStoneCountAlphaBetaAgent>(depth);//TODO: keep loaded evaluation in memory
#ifndef SUBMISSION
					cout << "------ try search depth: " << depth << " ------" << endl;
#endif
				} else {
					pAgent = std::make_shared<LookupStoneCountAlphaBetaAgent>(MAX_STEP);//do not use win-step agent, I want it still estimate even if it must lose
#ifndef SUBMISSION
					cout << "------ full search ------" << endl;
#endif
				}
				doIter = TryAgent(trueAccumulate, gameCount, start, finishedStep, input, pAgent);
				depth++;
			} while (estimate && doIter);
		}
	}
	return 0;
}