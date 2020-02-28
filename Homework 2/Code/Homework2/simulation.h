#pragma once

#include <cassert>
#include <algorithm>
#include <array>
#include <tuple>
#include <queue>
#include <random>

typedef unsigned int UINT32;

typedef unsigned long long UINT64;

const int SIZE = 5;

const float KOMI = float(SIZE)/2;

enum class Action : UINT64 {
	Pass = 0,
	P00 = 1 << 0, P01 = 1 << 1, P02 = 1 << 2, P03 = 1 << 3, P04 = 1 << 4,
	P10 = 1 << 5, P11 = 1 << 6, P12 = 1 << 7, P13 = 1 << 8, P14 = 1 << 9,
	P20 = 1 << 10, P21 = 1 << 11, P22 = 1 << 12, P23 = 1 << 13, P24 = 1 << 14,
	P30 = 1 << 15, P31 = 1 << 16, P32 = 1 << 17, P33 = 1 << 18, P34 = 1 << 19,
	P40 = 1 << 20, P41 = 1 << 21, P42 = 1 << 22, P43 = 1 << 23, P44 = 1 << 24,
};

typedef Action Position;

class OrthogonalNeighbours {
public:
	const Position Up;
	const Position Right;
	const Position Down;
	const Position Left;
	OrthogonalNeighbours(const Position _up, const Position _right, const Position _down, const Position _left) : Up(_up), Right(_right), Down(_down), Left(_left) {}
};

class DiagonalNeighbours {
public:
	const Position UpRight;
	const Position DownRight;
	const Position DownLeft;
	const Position UpLeft;
	DiagonalNeighbours(const Position _upRight, const Position _downRight, const Position _downLeft, const Position _upLeft) : UpRight(_upRight), DownRight(_downRight), DownLeft(_downLeft), UpLeft(_upLeft) {}
};

class Neighbours {
public:
	const OrthogonalNeighbours Orthogonal;
	const DiagonalNeighbours Diagonal;
	Neighbours(const OrthogonalNeighbours _orthogonal, const DiagonalNeighbours _diagonal): Orthogonal(_orthogonal), Diagonal(_diagonal) {}
};

enum class Player : unsigned char {
	Black = 0,
	White = 1,
};

enum class PositionState : unsigned char {
	Black = 0,
	White = 1,
	Empty = 2,
};

const static Player FIRST_PLAYER = Player::Black;

typedef unsigned char Step;

const static Step MAX_STEP = SIZE * SIZE - 1;

typedef UINT64 State;

const static State INITIAL_STATE = 0;

const static UINT64 EMPTY_SHIFT = 55;
const static UINT64 STEP_SHIFT = 50;
const static UINT64 OCCUPY_SHIFT = 25;

const static State PLAYER_FIELD_MASK = (1ULL << OCCUPY_SHIFT) - 1;
const static State OCCUPY_PLAYER_FIELD_MASK = (1ULL << STEP_SHIFT) - 1;
const static State OCCUPY_FIELD_MASK = OCCUPY_PLAYER_FIELD_MASK ^ PLAYER_FIELD_MASK;
const static State STEP_OCCUPY_PLAYER_FIELD_MASK = (1ULL << EMPTY_SHIFT) - 1;
const static State STEP_FIELD_MASK = STEP_OCCUPY_PLAYER_FIELD_MASK ^ OCCUPY_PLAYER_FIELD_MASK;

typedef UINT64 Board;

class Field {
public:
	inline static State StepField(const State state) {
		return state & STEP_FIELD_MASK;
	}

	inline static State OccupyField(const State state) {
		return state & OCCUPY_FIELD_MASK;
	}

	inline static Board BoardField(const State state) {
		return state & OCCUPY_PLAYER_FIELD_MASK;
	}

	inline static State PlayerField(const State state) {
		return state & PLAYER_FIELD_MASK;
	}
};

class StepUtil {
public:

	inline static State ConvertStepToStepField(const Step step) {
		return static_cast<State>(step) << STEP_SHIFT;
	}

	inline static Step GetStep(const State state) {
		return static_cast<Step>(Field::StepField(state) >> STEP_SHIFT);
	}

	inline static State SetStep(const State state, const Step step) {
		return ConvertStepToStepField(step) | Field::BoardField(state);
	}

	inline static State IncStep(const State state) {
		return state + (1ULL << STEP_SHIFT);
	}

	inline static bool IsValidStep(const State state) {
		return state >= (static_cast<State>(MAX_STEP + 1) << STEP_SHIFT);
	}
};

class BoardUtil {
private:
	inline static State OccupyMask(const Position position) {
		return static_cast<State>(position) << OCCUPY_SHIFT;
	}
public:
	inline static bool Occupied(const State state, const Position position) {
		assert(position != Position::Pass);
		return (state & OccupyMask(position)) != 0;
	}

	inline static bool Empty(const State state, const Position position) {
		assert(position != Position::Pass);
		return (state & OccupyMask(position)) == 0;
	}

	inline static Player GetRawPlayer(const State state, const Position position) {
		assert(position != Position::Pass);
		return static_cast<Player>((state & static_cast<State>(position)) != 0);
	}

	inline static PositionState GetPositionState(const State state, const Position position) {
		assert(position != Position::Pass);
		return Empty(state, position) ? PositionState::Empty : static_cast<PositionState>(GetRawPlayer(state, position));
	}
};

class ActionUtil {
public:
	inline static Action LastAction(const State currentStateBeforeCapture, const State lastStateAfterCapture) {
		return static_cast<Action>(Field::OccupyField(currentStateBeforeCapture ^ lastStateAfterCapture) >> OCCUPY_SHIFT);
	}

	inline static State ActWithoutCaptureWithoutIncStep(const State stateAfterCapture, const Player player, const Action action) {
		assert(action == Action::Pass || BoardUtil::Empty(stateAfterCapture, static_cast<Position>(action)));
		return stateAfterCapture | (static_cast<State>(action) << OCCUPY_SHIFT) | (static_cast<State>(player) * static_cast<State>(action));
	}
};

class TurnUtil {
public:
	inline static Player WhoNext(const Step finishedStep) {
		return static_cast<Player>(finishedStep & 0b1);//input 0, return Player::Black = 0
	}

	inline static Player Opponent(const Player player) {
		return static_cast<Player>(!static_cast<bool>(player));
	}

	inline static Player NextPlayer(const State currentState) {
		return WhoNext(StepUtil::GetStep(currentState));
	}
};

class PositionUtil {
public:
	static OrthogonalNeighbours GetOrthogonalNeighbours(const Position position) {
		assert(position != Position::Pass);
		auto up = static_cast<UINT64>(position) <= static_cast<UINT64>(Position::P04) ? Position::Pass : static_cast<Position>(static_cast<UINT64>(position) >> SIZE);
		auto right = position == Position::P04 || position == Position::P14 || position == Position::P24 || position == Position::P34 || position == Position::P44 ? Position::Pass : static_cast<Position>(static_cast<UINT64>(position) << 1);
		auto down = static_cast<UINT64>(position) >= static_cast<UINT64>(Position::P40) ? Position::Pass : static_cast<Position>(static_cast<UINT64>(position) << SIZE);
		auto left = position == Position::P00 || position == Position::P10 || position == Position::P20 || position == Position::P30 || position == Position::P40 ? Position::Pass : static_cast<Position>(static_cast<UINT64>(position) >> 1);
		return OrthogonalNeighbours(up, right, down, left);
	}

	static DiagonalNeighbours GetDiagonalNeighbours(const Position position) {
		assert(position != Position::Pass);
		auto upRight = position == Position::P04 ? Position::Pass : static_cast<Position>(static_cast<UINT64>(position) >> (SIZE - 1));
		auto downRight = position == Position::P44 ? Position::Pass : static_cast<Position>(static_cast<UINT64>(position) << (SIZE + 1));
		auto downLeft = position == Position::P40 ? Position::Pass : static_cast<Position>(static_cast<UINT64>(position) << (SIZE - 1));
		auto upLeft = position == Position::P00 ? Position::Pass : static_cast<Position>(static_cast<UINT64>(position) >> (SIZE + 1));
		return DiagonalNeighbours(upRight, downRight, downLeft, upLeft);
	}

	static Neighbours GetNeighbours(const Position position) {
		return Neighbours(GetOrthogonalNeighbours(position), GetDiagonalNeighbours(position));
	}
};

class Isomorphism {
private:
	inline static Board LowR90(const Board board) {
		return ((board & (1 << 20)) >> 20) | ((board & (1 << 15)) >> 14) | ((board & (1 << 10)) >> 8) | ((board & (1 << 5)) >> 2) | ((board & (1 << 0)) << 4)
			| ((board & (1 << 21)) >> 16) | ((board & (1 << 16)) >> 10) | ((board & (1 << 11)) >> 4) | ((board & (1 << 6)) << 2) | ((board & (1 << 1)) << 8)
			| ((board & (1 << 22)) >> 12) | ((board & (1 << 17)) >> 6) | ((board & (1 << 12)) << 0) | ((board & (1 << 7)) << 6) | ((board & (1 << 2)) << 12)
			| ((board & (1 << 23)) >> 8) | ((board & (1 << 18)) >> 2) | ((board & (1 << 13)) << 4) | ((board & (1 << 8)) << 10) | ((board & (1 << 3)) << 16)
			| ((board & (1 << 24)) >> 4) | ((board & (1 << 19)) << 2) | ((board & (1 << 14)) << 8) | ((board & (1 << 9)) << 14) | ((board & (1 << 4)) << 20);
	}

	inline static Board LowR180(const Board board) {
		return ((board & (1 << 24)) >> 24) | ((board & (1 << 23)) >> 22) | ((board & (1 << 22)) >> 20) | ((board & (1 << 21)) >> 18) | ((board & (1 << 20)) >> 16)
			| ((board & (1 << 19)) >> 14) | ((board & (1 << 18)) >> 12) | ((board & (1 << 17)) >> 10) | ((board & (1 << 16)) >> 8) | ((board & (1 << 15)) >> 6)
			| ((board & (1 << 14)) >> 4) | ((board & (1 << 13)) >> 2) | ((board & (1 << 12)) << 0) | ((board & (1 << 11)) << 2) | ((board & (1 << 10)) << 4)
			| ((board & (1 << 9)) << 6) | ((board & (1 << 8)) << 8) | ((board & (1 << 7)) << 10) | ((board & (1 << 6)) << 12) | ((board & (1 << 5)) << 14)
			| ((board & (1 << 4)) << 16) | ((board & (1 << 3)) << 18) | ((board & (1 << 2)) << 20) | ((board & (1 << 1)) << 22) | ((board & (1 << 0)) << 24);
	}

	inline static Board LowR270(const Board board) {
		return ((board & (1 << 4)) >> 4) | ((board & (1 << 9)) >> 8) | ((board & (1 << 14)) >> 12) | ((board & (1 << 19)) >> 16) | ((board & (1 << 24)) >> 20)
			| ((board & (1 << 3)) << 2) | ((board & (1 << 8)) >> 2) | ((board & (1 << 13)) >> 6) | ((board & (1 << 18)) >> 10) | ((board & (1 << 23)) >> 14)
			| ((board & (1 << 2)) << 8) | ((board & (1 << 7)) << 4) | ((board & (1 << 12)) << 0) | ((board & (1 << 17)) >> 4) | ((board & (1 << 22)) >> 8)
			| ((board & (1 << 1)) << 14) | ((board & (1 << 6)) << 10) | ((board & (1 << 11)) << 6) | ((board & (1 << 16)) << 2) | ((board & (1 << 21)) >> 2)
			| ((board & (1 << 0)) << 20) | ((board & (1 << 5)) << 16) | ((board & (1 << 10)) << 12) | ((board & (1 << 15)) << 8) | ((board & (1 << 20)) << 4);
	}

	inline static Board LowT(const Board board) {
		return ((board & (1 << 0)) << 0) | ((board & (1 << 5)) >> 4) | ((board & (1 << 10)) >> 8) | ((board & (1 << 15)) >> 12) | ((board & (1 << 20)) >> 16)
			| ((board & (1 << 1)) << 4) | ((board & (1 << 6)) << 0) | ((board & (1 << 11)) >> 4) | ((board & (1 << 16)) >> 8) | ((board & (1 << 21)) >> 12)
			| ((board & (1 << 2)) << 8) | ((board & (1 << 7)) << 4) | ((board & (1 << 12)) << 0) | ((board & (1 << 17)) >> 4) | ((board & (1 << 22)) >> 8)
			| ((board & (1 << 3)) << 12) | ((board & (1 << 8)) << 8) | ((board & (1 << 13)) << 4) | ((board & (1 << 18)) << 0) | ((board & (1 << 23)) >> 4)
			| ((board & (1 << 4)) << 16) | ((board & (1 << 9)) << 12) | ((board & (1 << 14)) << 8) | ((board & (1 << 19)) << 4) | ((board & (1 << 24)) << 0);
	}

	inline static Board LowTR90(const Board board) {
		return ((board & (1 << 4)) >> 4) | ((board & (1 << 3)) >> 2) | ((board & (1 << 2)) << 0) | ((board & (1 << 1)) << 2) | ((board & (1 << 0)) << 4)
			| ((board & (1 << 9)) >> 4) | ((board & (1 << 8)) >> 2) | ((board & (1 << 7)) << 0) | ((board & (1 << 6)) << 2) | ((board & (1 << 5)) << 4)
			| ((board & (1 << 14)) >> 4) | ((board & (1 << 13)) >> 2) | ((board & (1 << 12)) << 0) | ((board & (1 << 11)) << 2) | ((board & (1 << 10)) << 4)
			| ((board & (1 << 19)) >> 4) | ((board & (1 << 18)) >> 2) | ((board & (1 << 17)) << 0) | ((board & (1 << 16)) << 2) | ((board & (1 << 15)) << 4)
			| ((board & (1 << 24)) >> 4) | ((board & (1 << 23)) >> 2) | ((board & (1 << 22)) << 0) | ((board & (1 << 21)) << 2) | ((board & (1 << 20)) << 4);
	}

	inline static Board LowTR180(const Board board) {
		return ((board & (1 << 24)) >> 24) | ((board & (1 << 19)) >> 18) | ((board & (1 << 14)) >> 12) | ((board & (1 << 9)) >> 6) | ((board & (1 << 4)) << 0)
			| ((board & (1 << 23)) >> 18) | ((board & (1 << 18)) >> 12) | ((board & (1 << 13)) >> 6) | ((board & (1 << 8)) << 0) | ((board & (1 << 3)) << 6)
			| ((board & (1 << 22)) >> 12) | ((board & (1 << 17)) >> 6) | ((board & (1 << 12)) << 0) | ((board & (1 << 7)) << 6) | ((board & (1 << 2)) << 12)
			| ((board & (1 << 21)) >> 6) | ((board & (1 << 16)) << 0) | ((board & (1 << 11)) << 6) | ((board & (1 << 6)) << 12) | ((board & (1 << 1)) << 18)
			| ((board & (1 << 20)) << 0) | ((board & (1 << 15)) << 6) | ((board & (1 << 10)) << 12) | ((board & (1 << 5)) << 18) | ((board & (1 << 0)) << 24);
	}

	inline static Board LowTR270(const Board board) {
		return ((board & (1 << 20)) >> 20) | ((board & (1 << 21)) >> 20) | ((board & (1 << 22)) >> 20) | ((board & (1 << 23)) >> 20) | ((board & (1 << 24)) >> 20)
			| ((board & (1 << 15)) >> 10) | ((board & (1 << 16)) >> 10) | ((board & (1 << 17)) >> 10) | ((board & (1 << 18)) >> 10) | ((board & (1 << 19)) >> 10)
			| ((board & (1 << 10)) << 0) | ((board & (1 << 11)) << 0) | ((board & (1 << 12)) << 0) | ((board & (1 << 13)) << 0) | ((board & (1 << 14)) << 0)
			| ((board & (1 << 5)) << 10) | ((board & (1 << 6)) << 10) | ((board & (1 << 7)) << 10) | ((board & (1 << 8)) << 10) | ((board & (1 << 9)) << 10)
			| ((board & (1 << 0)) << 20) | ((board & (1 << 1)) << 20) | ((board & (1 << 2)) << 20) | ((board & (1 << 3)) << 20) | ((board & (1 << 4)) << 20);
	}
public:
	Board R0, R90, R180, R270, T, TR90, TR180, TR270;

	Isomorphism(const State state) {
		auto playerField = Field::PlayerField(state);
		auto boardField = Field::BoardField(state);
		auto loweredOccupyField = boardField >> OCCUPY_SHIFT;

		R0 = boardField;
		R90 = (LowR90(loweredOccupyField) << OCCUPY_SHIFT) | LowR90(playerField);
		R180 = (LowR180(loweredOccupyField) << OCCUPY_SHIFT) | LowR180(playerField);
		R270 = (LowR270(loweredOccupyField) << OCCUPY_SHIFT) | LowR270(playerField);
		T = (LowT(loweredOccupyField) << OCCUPY_SHIFT) | LowT(playerField);
		TR90 = (LowTR90(loweredOccupyField) << OCCUPY_SHIFT) | LowTR90(playerField);
		TR180 = (LowTR180(loweredOccupyField) << OCCUPY_SHIFT) | LowTR180(playerField);
		TR270 = (LowTR270(loweredOccupyField) << OCCUPY_SHIFT) | LowTR270(playerField);
	}

	inline Board IndexBoard() const {
		return std::min(std::min(std::min(R0, R90), std::min(R180, R270)), std::min(std::min(T, TR90), std::min(TR180, TR270)));
	}

	inline static Board IndexBoard(const State state) {
		return Isomorphism(state).IndexBoard();
	}

	inline static State IndexState(const State state) {
		return Field::StepField(state) | IndexBoard(state);
	}
};

class Capture {
private:
	inline static void Expand(const State state, const Player expandPlayer, const Position neighbour, State& checked, State& group, bool& liberty, std::queue<Position>& queue) {
		if (neighbour == Position::Pass) {
			return;
		}
		if ((checked & static_cast<State>(neighbour)) != 0) {
			return;
		}
		checked |= static_cast<State>(neighbour);
		if (BoardUtil::Empty(state, neighbour)) {
			liberty = true;
			return;
		}
		auto neighbourPlayer = BoardUtil::GetRawPlayer(state, neighbour);
		if (neighbourPlayer != expandPlayer) {
			return;
		}
		group |= static_cast<State>(neighbour);
		queue.emplace(neighbour);
	}

	inline static void CaptureNeighbour(State& state, const Player player, const Position neighbour) {
		if (neighbour == Position::Pass) {
			return;
		}
		if (BoardUtil::Empty(state, neighbour)) {
			return;
		}
		auto neighbourPlayer = BoardUtil::GetRawPlayer(state, neighbour);
		if (neighbourPlayer == player) {
			return;
		}
		auto temp = FindGroupAndLiberty(state, neighbour);
		if (!temp.second) {
			state ^= temp.first << OCCUPY_SHIFT;
		}
	}

	static std::pair<State, bool> FindGroupAndLiberty(const State state, const Position position) {
		assert(BoardUtil::GetPositionState(state, position) != PositionState::Empty);
		auto player = BoardUtil::GetRawPlayer(state, position);
		auto checked = static_cast<State>(position);
		auto group = static_cast<State>(position);
		auto liberty = false;
		auto queue = std::queue<Position>();
		queue.emplace(position);
		while (queue.size() > 0) {
			auto current = queue.front();
			queue.pop();
			auto neighbour = PositionUtil::GetOrthogonalNeighbours(current);
			Expand(state, player, neighbour.Up, checked, group, liberty, queue);
			Expand(state, player, neighbour.Right, checked, group, liberty, queue);
			Expand(state, player, neighbour.Down, checked, group, liberty, queue);
			Expand(state, player, neighbour.Left, checked, group, liberty, queue);
		}
		return std::make_pair(group, liberty);
	}
public:
	
	static bool TryApply(State& stateAfterAct, const Position position) {
		assert(BoardUtil::GetPositionState(stateAfterAct, position) != PositionState::Empty);
		auto self = BoardUtil::GetRawPlayer(stateAfterAct, position);
		auto opponent = TurnUtil::Opponent(self);
		auto neighbour = PositionUtil::GetOrthogonalNeighbours(position);
		//only neighbour positions can be captured
		CaptureNeighbour(stateAfterAct, self, neighbour.Up);
		CaptureNeighbour(stateAfterAct, self, neighbour.Right);
		CaptureNeighbour(stateAfterAct, self, neighbour.Down);
		CaptureNeighbour(stateAfterAct, self, neighbour.Left);
		auto temp = FindGroupAndLiberty(stateAfterAct, position);
		return temp.second;
	}
};

class Rule {
public:
	inline static bool ViolateNoConsecutivePassingRule(const bool isFirstStep, const Board lastBoard, const Board currentBoard, const Action action) {
		return !isFirstStep && action == Action::Pass && lastBoard == currentBoard;
	}

	inline static bool ViolateEmptyRule(const Board board, const Action action) {
		return action != Action::Pass && BoardUtil::Occupied(board, action);
	}

	inline static bool ViolateNoSuicideRule(const bool hasLiberty) {
		return !hasLiberty;
	}

	inline static bool ViolateKoRule(const bool isFirstStep, const Board lastBoard, const Board afterBoard) {
		return !isFirstStep && lastBoard == afterBoard;
	}
};

const static std::array<Action, 26> ACTION_SEQUENCE = {
	Action::P00, Action::P01, Action::P02, Action::P03, Action::P04,
	Action::P10, Action::P11, Action::P12, Action::P13, Action::P14,
	Action::P20, Action::P21, Action::P22, Action::P23, Action::P24,
	Action::P30, Action::P31, Action::P32, Action::P33, Action::P34,
	Action::P40, Action::P41, Action::P42, Action::P43, Action::P44,
	Action::Pass,
};

class LegalActionIterator {
private:
	Board lastBoard;
	Board currentBoard;

	bool isFirstStep;
	Player player;
	
	int nextActionIndex;
	bool hasAnyLegalAction = false;

	inline static bool TryAction(const Board lastBoard, const Board currentBoard, const Player player, const bool isFirstStep, const Action action, Board& resultBoard) {
		if (Rule::ViolateEmptyRule(currentBoard, action) || Rule::ViolateNoConsecutivePassingRule(isFirstStep, lastBoard, currentBoard, action)) {
			return false;
		}
		if (action == Action::Pass) {
			resultBoard = currentBoard;
		} else {
			auto afterBoard = ActionUtil::ActWithoutCaptureWithoutIncStep(currentBoard, player, action);
			auto hasLiberty = Capture::TryApply(afterBoard, static_cast<Position>(action));
			if (Rule::ViolateNoSuicideRule(hasLiberty) || Rule::ViolateKoRule(isFirstStep, lastBoard, afterBoard)) {
				return false;
			}
			resultBoard = afterBoard;
		}
		return true;
	}
public:

	LegalActionIterator() : LegalActionIterator(FIRST_PLAYER, 0, 0, true) {}

	LegalActionIterator(const Player _player, const Board _lastBoard, const Board _currentBoard, const bool _isFirstStep) : player(_player), lastBoard(_lastBoard), currentBoard(_currentBoard), isFirstStep(_isFirstStep) {
		nextActionIndex = 0;
	}

	Player GetPlayer() const {
		return player;
	}

	Board GetCurrentBoard() const {
		return currentBoard;
	}

	bool Next(Action& action, Board& afterBoard) {
		while (nextActionIndex < ACTION_SEQUENCE.size()) {
			action = ACTION_SEQUENCE[nextActionIndex];
			nextActionIndex++;
			auto available = TryAction(lastBoard, currentBoard, player, isFirstStep, action, afterBoard);
			if (available) {
				hasAnyLegalAction = true;
				return true;
			}
		}
		assert(hasAnyLegalAction);
		return false;
	}

	static std::vector<std::pair<Action, State>> ListAll(const Player player, const Board lastBoard, const Board currentBoard, const bool isFirstStep) {
		auto result = std::vector<std::pair<Action, State>>();
		auto iter = LegalActionIterator(player, lastBoard, currentBoard, isFirstStep);
		Action action;
		Board board;
		while (iter.Next(action, board)) {
			result.emplace_back(action, board);
		}
		return result;
	}
};

class PartialScore {
public:
	UINT32 Black = 0;
	UINT32 White = 0;
};

class FinalScore {
public:
	float Black;
	float White;
	FinalScore() : Black(0.0), White(0.0){}
	FinalScore(const PartialScore& _partial): Black(float(_partial.Black)), White(KOMI + _partial.White) {}
};

class Score {
private:
	inline static void Expand(const State state, const Position neighbour, State& checked, State& toFill, bool& nextToBlack, bool& nextToWhite, std::queue<Position>& queue) {
		if (neighbour == Position::Pass) {
			return;
		}
		if ((checked & static_cast<State>(neighbour)) != 0) {
			return;
		}
		checked |= static_cast<State>(neighbour);
		if (BoardUtil::Occupied(state, neighbour)) {
			auto player = BoardUtil::GetRawPlayer(state, neighbour);
			switch (player) {
			case Player::Black:
				nextToBlack = true;
				break;
			case Player::White:
				nextToWhite = true;
				break;
			}
			return;
		}
		toFill |= static_cast<State>(neighbour);
		queue.emplace(neighbour);
	}

	static void FillEmptyPosition(State& state, const Position position) {
		if (BoardUtil::Occupied(state, position)) {
			return;
		}
		auto nextToBlack = false;
		auto nextToWhite = false;
		auto checked = static_cast<State>(position);
		auto toFill = static_cast<State>(position);
		auto queue = std::queue<Position>();
		queue.emplace(position);
		while (queue.size() > 0) {
			auto current = queue.front();
			queue.pop();
			auto neighbour = PositionUtil::GetOrthogonalNeighbours(current);
			Expand(state, neighbour.Up, checked, toFill, nextToBlack, nextToWhite, queue);
			Expand(state, neighbour.Right, checked, toFill, nextToBlack, nextToWhite, queue);
			Expand(state, neighbour.Down, checked, toFill, nextToBlack, nextToWhite, queue);
			Expand(state, neighbour.Left, checked, toFill, nextToBlack, nextToWhite, queue);
		}
		assert(nextToBlack || nextToWhite);
		if (nextToBlack && nextToWhite) {
			return;
		}
		state |= toFill << OCCUPY_SHIFT;
		if (nextToBlack) {//fill 0
			state &= ~toFill;
		} else if (nextToWhite) {// fill 1
			state |= toFill;
		}
	}

	static State FillEmptyPositions(const State state) {
		auto result = state;
		FillEmptyPosition(result, Position::P00);
		FillEmptyPosition(result, Position::P01);
		FillEmptyPosition(result, Position::P02);
		FillEmptyPosition(result, Position::P03);
		FillEmptyPosition(result, Position::P04);
		FillEmptyPosition(result, Position::P10);
		FillEmptyPosition(result, Position::P11);
		FillEmptyPosition(result, Position::P12);
		FillEmptyPosition(result, Position::P13);
		FillEmptyPosition(result, Position::P14);
		FillEmptyPosition(result, Position::P20);
		FillEmptyPosition(result, Position::P21);
		FillEmptyPosition(result, Position::P22);
		FillEmptyPosition(result, Position::P23);
		FillEmptyPosition(result, Position::P24);
		FillEmptyPosition(result, Position::P30);
		FillEmptyPosition(result, Position::P31);
		FillEmptyPosition(result, Position::P32);
		FillEmptyPosition(result, Position::P33);
		FillEmptyPosition(result, Position::P34);
		FillEmptyPosition(result, Position::P40);
		FillEmptyPosition(result, Position::P41);
		FillEmptyPosition(result, Position::P42);
		FillEmptyPosition(result, Position::P43);
		FillEmptyPosition(result, Position::P44);
		return result;
	}

	inline static void CountPosition(const State filledState, const Position position, UINT32& blackPartialScore, UINT32& whitePartialScore) {
		if (BoardUtil::Empty(filledState, position)) {
			return;
		}
		auto player = BoardUtil::GetRawPlayer(filledState, position);
		switch (player) {
		case Player::Black:
			blackPartialScore++;
			break;
		case Player::White:
			whitePartialScore++;
			break;
		}
	}

	inline static PartialScore CountPartialScores(const State filledState) {
		auto result = PartialScore();
		CountPosition(filledState, Position::P00, result.Black, result.White);
		CountPosition(filledState, Position::P01, result.Black, result.White);
		CountPosition(filledState, Position::P02, result.Black, result.White);
		CountPosition(filledState, Position::P03, result.Black, result.White);
		CountPosition(filledState, Position::P04, result.Black, result.White);
		CountPosition(filledState, Position::P10, result.Black, result.White);
		CountPosition(filledState, Position::P11, result.Black, result.White);
		CountPosition(filledState, Position::P12, result.Black, result.White);
		CountPosition(filledState, Position::P13, result.Black, result.White);
		CountPosition(filledState, Position::P14, result.Black, result.White);
		CountPosition(filledState, Position::P20, result.Black, result.White);
		CountPosition(filledState, Position::P21, result.Black, result.White);
		CountPosition(filledState, Position::P22, result.Black, result.White);
		CountPosition(filledState, Position::P23, result.Black, result.White);
		CountPosition(filledState, Position::P24, result.Black, result.White);
		CountPosition(filledState, Position::P30, result.Black, result.White);
		CountPosition(filledState, Position::P31, result.Black, result.White);
		CountPosition(filledState, Position::P32, result.Black, result.White);
		CountPosition(filledState, Position::P33, result.Black, result.White);
		CountPosition(filledState, Position::P34, result.Black, result.White);
		CountPosition(filledState, Position::P40, result.Black, result.White);
		CountPosition(filledState, Position::P41, result.Black, result.White);
		CountPosition(filledState, Position::P42, result.Black, result.White);
		CountPosition(filledState, Position::P43, result.Black, result.White);
		CountPosition(filledState, Position::P44, result.Black, result.White);
		return result;
	}
public:

	static std::pair<Player, FinalScore> Winner(const State finalState) {
		auto filled = FillEmptyPositions(finalState);
		auto partial = CountPartialScores(filled);
		auto fin = FinalScore(partial);
		assert(fin.Black != fin.White);
		auto winner = fin.White > fin.Black ? Player::White : Player::Black;
		return std::make_pair(winner, fin);
	}
};

class Agent {
protected:
	inline static bool IsFirstStep(const Player player, const Board lastBoard, const Board currentBoard) {
		return player == FIRST_PLAYER && lastBoard == currentBoard && currentBoard == 0;
	}

	inline static Player MyPlayer(const Step finishedStep) {
		return TurnUtil::WhoNext(finishedStep);
	}

public :
	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) = 0;//Why step is not provided?
};

class RandomAgent : public Agent {
public:
	virtual Action Act(const Step finishedStep, const Board lastBoard, const Board currentBoard) override {
		auto player = TurnUtil::WhoNext(finishedStep);
		auto isFirstStep = IsFirstStep(player, lastBoard, currentBoard);
		auto allActions = LegalActionIterator::ListAll(player, lastBoard, currentBoard, isFirstStep);
		std::random_device dev;
		auto rng = std::mt19937(dev());
		auto dist = std::uniform_int_distribution<std::mt19937::result_type>(0, allActions.size() - 1);
		auto selected = allActions.at(dist(rng));
		return selected.first;
	}
};

class PlainAction {
public:
	bool Pass;
	unsigned char I;
	unsigned char J;

	explicit PlainAction() : Pass(true), I(0), J(0) {}
	PlainAction(const int i, const int j) : I(i), J(j), Pass(false) {}
	PlainAction(const Action _action) {
		if (_action == Action::Pass) {
			Pass = true;
			I = 0;
			J = 0;
		} else {
			Pass = false;
			switch (_action) {
			case Action::P00:
				I = 0;
				J = 0;
				break;
			case Action::P01:
				I = 0;
				J = 1;
				break;
			case Action::P02:
				I = 0;
				J = 2;
				break;
			case Action::P03:
				I = 0;
				J = 3;
				break;
			case Action::P04:
				I = 0;
				J = 4;
				break;
			case Action::P10:
				I = 1;
				J = 0;
				break;
			case Action::P11:
				I = 1;
				J = 1;
				break;
			case Action::P12:
				I = 1;
				J = 2;
				break;
			case Action::P13:
				I = 1;
				J = 3;
				break;
			case Action::P14:
				I = 1;
				J = 4;
				break;
			case Action::P20:
				I = 2;
				J = 0;
				break;
			case Action::P21:
				I = 2;
				J = 1;
				break;
			case Action::P22:
				I = 2;
				J = 2;
				break;
			case Action::P23:
				I = 2;
				J = 3;
				break;
			case Action::P24:
				I = 2;
				J = 4;
				break;
			case Action::P30:
				I = 3;
				J = 0;
				break;
			case Action::P31:
				I = 3;
				J = 1;
				break;
			case Action::P32:
				I = 3;
				J = 2;
				break;
			case Action::P33:
				I = 3;
				J = 3;
				break;
			case Action::P34:
				I = 3;
				J = 4;
				break;
			case Action::P40:
				I = 4;
				J = 0;
				break;
			case Action::P41:
				I = 4;
				J = 1;
				break;
			case Action::P42:
				I = 4;
				J = 2;
				break;
			case Action::P43:
				I = 4;
				J = 3;
				break;
			case Action::P44:
				I = 4;
				J = 4;
				break;
			}
		}
	}

	Action Convert() const {
		if (Pass) {
			return Action::Pass;
		}
		switch (I) {
		case 0:
			switch (J) {
			case 0:
				return Action::P00;
			case 1:
				return Action::P01;
			case 2:
				return Action::P02;
			case 3:
				return Action::P03;
			case 4:
				return Action::P04;
			}
		case 1:
			switch (J) {
			case 0:
				return Action::P10;
			case 1:
				return Action::P11;
			case 2:
				return Action::P12;
			case 3:
				return Action::P13;
			case 4:
				return Action::P14;
			}
		case 2:
			switch (J) {
			case 0:
				return Action::P20;
			case 1:
				return Action::P21;
			case 2:
				return Action::P22;
			case 3:
				return Action::P23;
			case 4:
				return Action::P24;
			}
		case 3:
			switch (J) {
			case 0:
				return Action::P30;
			case 1:
				return Action::P31;
			case 2:
				return Action::P32;
			case 3:
				return Action::P33;
			case 4:
				return Action::P34;
			}
		case 4:
			switch (J) {
			case 0:
				return Action::P40;
			case 1:
				return Action::P41;
			case 2:
				return Action::P42;
			case 3:
				return Action::P43;
			case 4:
				return Action::P44;
			}
		}
	}
};

class PlainState {
private:
	inline State LoweredOccupyField() const {
		return (static_cast<State>(Occupy[0][0])* static_cast<State>(Position::P00)) | (static_cast<State>(Occupy[0][1])* static_cast<State>(Position::P01)) | (static_cast<State>(Occupy[0][2])* static_cast<State>(Position::P02)) | (static_cast<State>(Occupy[0][3])* static_cast<State>(Position::P03)) | (static_cast<State>(Occupy[0][4])* static_cast<State>(Position::P04))
			| (static_cast<State>(Occupy[1][0])* static_cast<State>(Position::P10)) | (static_cast<State>(Occupy[1][1])* static_cast<State>(Position::P11)) | (static_cast<State>(Occupy[1][2])* static_cast<State>(Position::P12)) | (static_cast<State>(Occupy[1][3])* static_cast<State>(Position::P13)) | (static_cast<State>(Occupy[1][4])* static_cast<State>(Position::P14))
			| (static_cast<State>(Occupy[2][0])* static_cast<State>(Position::P20)) | (static_cast<State>(Occupy[2][1])* static_cast<State>(Position::P21)) | (static_cast<State>(Occupy[2][2])* static_cast<State>(Position::P22)) | (static_cast<State>(Occupy[2][3])* static_cast<State>(Position::P23)) | (static_cast<State>(Occupy[2][4])* static_cast<State>(Position::P24))
			| (static_cast<State>(Occupy[3][0])* static_cast<State>(Position::P30)) | (static_cast<State>(Occupy[3][1])* static_cast<State>(Position::P31)) | (static_cast<State>(Occupy[3][2])* static_cast<State>(Position::P32)) | (static_cast<State>(Occupy[3][3])* static_cast<State>(Position::P33)) | (static_cast<State>(Occupy[3][4])* static_cast<State>(Position::P34))
			| (static_cast<State>(Occupy[4][0])* static_cast<State>(Position::P40)) | (static_cast<State>(Occupy[4][1])* static_cast<State>(Position::P41)) | (static_cast<State>(Occupy[4][2])* static_cast<State>(Position::P42)) | (static_cast<State>(Occupy[4][3])* static_cast<State>(Position::P43)) | (static_cast<State>(Occupy[4][4])* static_cast<State>(Position::P44))
			;
	}

	inline State PlayerField() const {
		return (static_cast<State>(Player[0][0])* static_cast<State>(Position::P00)) | (static_cast<State>(Player[0][1])* static_cast<State>(Position::P01)) | (static_cast<State>(Player[0][2])* static_cast<State>(Position::P02)) | (static_cast<State>(Player[0][3])* static_cast<State>(Position::P03)) | (static_cast<State>(Player[0][4])* static_cast<State>(Position::P04))
			| (static_cast<State>(Player[1][0])* static_cast<State>(Position::P10)) | (static_cast<State>(Player[1][1])* static_cast<State>(Position::P11)) | (static_cast<State>(Player[1][2])* static_cast<State>(Position::P12)) | (static_cast<State>(Player[1][3])* static_cast<State>(Position::P13)) | (static_cast<State>(Player[1][4])* static_cast<State>(Position::P14))
			| (static_cast<State>(Player[2][0])* static_cast<State>(Position::P20)) | (static_cast<State>(Player[2][1])* static_cast<State>(Position::P21)) | (static_cast<State>(Player[2][2])* static_cast<State>(Position::P22)) | (static_cast<State>(Player[2][3])* static_cast<State>(Position::P23)) | (static_cast<State>(Player[2][4])* static_cast<State>(Position::P24))
			| (static_cast<State>(Player[3][0])* static_cast<State>(Position::P30)) | (static_cast<State>(Player[3][1])* static_cast<State>(Position::P31)) | (static_cast<State>(Player[3][2])* static_cast<State>(Position::P32)) | (static_cast<State>(Player[3][3])* static_cast<State>(Position::P33)) | (static_cast<State>(Player[3][4])* static_cast<State>(Position::P34))
			| (static_cast<State>(Player[4][0])* static_cast<State>(Position::P40)) | (static_cast<State>(Player[4][1])* static_cast<State>(Position::P41)) | (static_cast<State>(Player[4][2])* static_cast<State>(Position::P42)) | (static_cast<State>(Player[4][3])* static_cast<State>(Position::P43)) | (static_cast<State>(Player[4][4])* static_cast<State>(Position::P44))
			;
	}

public:
	Step Step;
	bool Occupy[SIZE][SIZE];
	Player Player[SIZE][SIZE];

	PlainState(const State state) {
		Step = StepUtil::GetStep(state);
		Occupy[0][0] = BoardUtil::Occupied(state, Position::P00);
		Occupy[0][1] = BoardUtil::Occupied(state, Position::P01);
		Occupy[0][2] = BoardUtil::Occupied(state, Position::P02);
		Occupy[0][3] = BoardUtil::Occupied(state, Position::P03);
		Occupy[0][4] = BoardUtil::Occupied(state, Position::P04);
		Occupy[1][0] = BoardUtil::Occupied(state, Position::P10);
		Occupy[1][1] = BoardUtil::Occupied(state, Position::P11);
		Occupy[1][2] = BoardUtil::Occupied(state, Position::P12);
		Occupy[1][3] = BoardUtil::Occupied(state, Position::P13);
		Occupy[1][4] = BoardUtil::Occupied(state, Position::P14);
		Occupy[2][0] = BoardUtil::Occupied(state, Position::P20);
		Occupy[2][1] = BoardUtil::Occupied(state, Position::P21);
		Occupy[2][2] = BoardUtil::Occupied(state, Position::P22);
		Occupy[2][3] = BoardUtil::Occupied(state, Position::P23);
		Occupy[2][4] = BoardUtil::Occupied(state, Position::P24);
		Occupy[3][0] = BoardUtil::Occupied(state, Position::P30);
		Occupy[3][1] = BoardUtil::Occupied(state, Position::P31);
		Occupy[3][2] = BoardUtil::Occupied(state, Position::P32);
		Occupy[3][3] = BoardUtil::Occupied(state, Position::P33);
		Occupy[3][4] = BoardUtil::Occupied(state, Position::P34);
		Occupy[4][0] = BoardUtil::Occupied(state, Position::P40);
		Occupy[4][1] = BoardUtil::Occupied(state, Position::P41);
		Occupy[4][2] = BoardUtil::Occupied(state, Position::P42);
		Occupy[4][3] = BoardUtil::Occupied(state, Position::P43);
		Occupy[4][4] = BoardUtil::Occupied(state, Position::P44);
		Player[0][0] = BoardUtil::GetRawPlayer(state, Position::P00);
		Player[0][1] = BoardUtil::GetRawPlayer(state, Position::P01);
		Player[0][2] = BoardUtil::GetRawPlayer(state, Position::P02);
		Player[0][3] = BoardUtil::GetRawPlayer(state, Position::P03);
		Player[0][4] = BoardUtil::GetRawPlayer(state, Position::P04);
		Player[1][0] = BoardUtil::GetRawPlayer(state, Position::P10);
		Player[1][1] = BoardUtil::GetRawPlayer(state, Position::P11);
		Player[1][2] = BoardUtil::GetRawPlayer(state, Position::P12);
		Player[1][3] = BoardUtil::GetRawPlayer(state, Position::P13);
		Player[1][4] = BoardUtil::GetRawPlayer(state, Position::P14);
		Player[2][0] = BoardUtil::GetRawPlayer(state, Position::P20);
		Player[2][1] = BoardUtil::GetRawPlayer(state, Position::P21);
		Player[2][2] = BoardUtil::GetRawPlayer(state, Position::P22);
		Player[2][3] = BoardUtil::GetRawPlayer(state, Position::P23);
		Player[2][4] = BoardUtil::GetRawPlayer(state, Position::P24);
		Player[3][0] = BoardUtil::GetRawPlayer(state, Position::P30);
		Player[3][1] = BoardUtil::GetRawPlayer(state, Position::P31);
		Player[3][2] = BoardUtil::GetRawPlayer(state, Position::P32);
		Player[3][3] = BoardUtil::GetRawPlayer(state, Position::P33);
		Player[3][4] = BoardUtil::GetRawPlayer(state, Position::P34);
		Player[4][0] = BoardUtil::GetRawPlayer(state, Position::P40);
		Player[4][1] = BoardUtil::GetRawPlayer(state, Position::P41);
		Player[4][2] = BoardUtil::GetRawPlayer(state, Position::P42);
		Player[4][3] = BoardUtil::GetRawPlayer(state, Position::P43);
		Player[4][4] = BoardUtil::GetRawPlayer(state, Position::P44);
	}

	inline State Convert() const {
		return StepUtil::ConvertStepToStepField(Step) | (LoweredOccupyField() << OCCUPY_SHIFT) | PlayerField();
	}
};
