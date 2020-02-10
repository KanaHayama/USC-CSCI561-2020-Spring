/*
	Author: Zongjian Li
	USC Email: zongjian@usc.edu
	USC ID: 6503378943
*/
#pragma region include
//=============================================//
//                                             //
//                include                      //
//                                             //
//=============================================//
#include <cassert>
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <regex>
using std::regex;
#include <algorithm>
#include <unordered_map>
using std::unordered_map;
#include <unordered_set>
using std::unordered_set;
#include <queue>
using std::priority_queue;
#include <iostream>
#include <fstream>
using std::ifstream;
using std::ofstream;
#pragma endregion end include
//=============================================//
//                                             //
//                type def                     //
//                                             //
//=============================================//
typedef long Int32;
typedef long long Int64;
typedef Int32 CoordinateUnit;
typedef Int32 TimeUnit;
typedef Int64 CostUnit;
#pragma endregion end type def
#pragma region const
//=============================================//
//                                             //
//                  const                      //
//                                             //
//=============================================//
const string INPUT_FILENAME = "input.txt";
const string OUTPUT_FILENAME = "output.txt";
const string SPACE = " ";
const CostUnit INIT_COST = 0;
const CostUnit UNIT_COST = 1;
const CostUnit STRAIGHT_UNEVEN_UNIT_COST = 10;
const CostUnit DIAGONAL_UNEVEN_UNIT_COST = 14;
#pragma endregion end const
#pragma region type def
#pragma region util
//=============================================//
//                                             //
//                  const                      //
//                                             //
//=============================================//
#pragma endregion end util
#pragma region class
//=============================================//
//                                             //
//                  class                      //
//                                             //
//=============================================//
#pragma region miscellaneous
enum class Algorithm {
	BreadthFirst,
	UniformCost,
	AStar,
};

enum class Action {
	None,
	Joint,
	North,
	NorthEast,
	East,
	SouthEast,
	South,
	SouthWest,
	West,
	NorthWest,
};
#pragma endregion end miscellaneous
#pragma region world
class Point {
public:
	CoordinateUnit x;
	CoordinateUnit y;
	Point() = default;
	Point(const CoordinateUnit _x, const CoordinateUnit _y) : x(_x), y(_y) {}
	inline bool operator == (const Point& other) const noexcept {
		return x == other.x && y == other.y;
	}
	inline Point Diff(const Point& other) const noexcept {
		return Point(x - other.x, y - other.y);
	}
};

class State {
public:
	Point point;
	TimeUnit time;
	State() = default;
	State(const CoordinateUnit& _x, const CoordinateUnit& _y, const TimeUnit _time) : point(_x, _y), time(_time) {}
	State(const Point& _point, const TimeUnit _time) : point(_point), time(_time) {}
	inline bool operator == (const State& other) const noexcept {
		return point == other.point && time == other.time;
	};
	inline State Diff(const State& other) const noexcept {
		return State(point.Diff(other.point), time - other.time);
	}
};
inline State abs(const State& state) noexcept {
	return State(abs(state.point.x), abs(state.point.y), abs(state.time));
}

class GridSize {
public:
	CoordinateUnit width;
	CoordinateUnit height;
	inline bool Check(const Point& point) const {
		return 0 <= point.x && point.x < width && 0 <= point.y && point.y < height;
	}
};

class Channel {
public:
	Point point;
	TimeUnit time1;
	TimeUnit time2;
	Channel() = default;
	Channel(const TimeUnit _time1, const CoordinateUnit _x, const CoordinateUnit _y, const TimeUnit _time2) : point(_x, _y), time1(_time1), time2(_time2) {}

};

class GridWorld {
public:
	Algorithm algorithm;
	State begin;
	State end;
	GridSize gridSize;
	vector<Channel> channels;
};

static GridWorld world;

#pragma region hash
namespace std {
	template<> struct hash<Point> {
		inline std::size_t operator()(Point const& point) const noexcept {
			auto xHash = std::hash<CoordinateUnit>{}(point.x);
			auto yHash = std::hash<CoordinateUnit>{}(point.y);
			return xHash ^ (yHash << 1);
		}
	};

	template<> struct hash<State> {
		inline std::size_t operator()(State const& state) const noexcept {
			auto xHash = std::hash<CoordinateUnit>{}(state.point.x);
			auto yHash = std::hash<CoordinateUnit>{}(state.point.y);
			auto timeHash = std::hash<TimeUnit>{}(state.time);
			return timeHash ^ (xHash << 1) ^ (yHash >> 1);
		}
	};
}
#pragma endregion end hash
#pragma endregion end world
#pragma region search

class Graph {
private:
	unordered_map<TimeUnit, unordered_set<State>> gridNeighbours;
	unordered_map<State, unordered_set<State>> channelNeighbours;
public:
	Graph(const GridWorld& world) {
		gridNeighbours[world.end.time].insert(world.end);
		for (auto& channel : world.channels) {
			State endPoint1(channel.point, channel.time1);
			State endPoint2(channel.point, channel.time2);
			// add to grid
			gridNeighbours[endPoint1.time].insert(endPoint1);
			gridNeighbours[endPoint2.time].insert(endPoint2);
			// add to channel
			channelNeighbours[endPoint1].insert(endPoint2);
			channelNeighbours[endPoint2].insert(endPoint1);
		}
	}

	inline unordered_set<State> Neighbours(const State& state) const {
		unordered_set<State> result;
		auto f1 = gridNeighbours.find(state.time);
		if (f1 != gridNeighbours.end()) {
			for (auto& neighbour : f1->second) {
				result.emplace(neighbour);
			}
		}
		auto f2 = channelNeighbours.find(state);
		if (f2 != channelNeighbours.end()) {
			for (auto& neighbour : f2->second) {
				result.emplace(neighbour);
			}
		}
		return result;
	}
};

class Record {
public:
	State state;
	CostUnit cost;
	State last;
	Record() = default;
	Record(const State& _state, const  CostUnit _cost, const State& _last) : state(_state), cost(_cost), last(_last) {}
};

class Visit {
public:
	CostUnit cost;
	State last;
	Visit() = default;
	Visit(const CostUnit _cost, const State& _last) : cost(_cost), last(_last) {}
};

class Step {
public:
	State state;
	CostUnit cost;
	Step() = default;
	Step(const State& _state, const CostUnit _cost) : state(_state), cost(_cost) {}
	inline bool operator == (const Step& other) const noexcept {
		return state == other.state && cost == other.cost;
	}
};

class Path {
public:
	bool success;
	CostUnit cost;
	vector<Step> steps;
};

class Result : public Path {
public:
	//Path keys;
};

enum class Terminate {
	None, // best for A*
	BeforePush, // best for BFS
	AfterPop, // best for UCS
};

template <typename T> inline int sign(T val) {
	return (T(0) < val) - (val < T(0));
}

class Searcher {
private:
	const Terminate terminate;
	struct Comparator {
		const Searcher& searcher;
		Comparator(const Searcher& _searcher) : searcher(_searcher) {}
		bool operator () (const Record& record1, const Record& record2) const {
			auto h1 = searcher.Heuristic(record1);
			auto h2 = searcher.Heuristic(record2);
			auto result = h1 > h2;
			return result;
		}
	};
	inline CostUnit Cost(const State& state1, const State& state2) const {
		auto diff = abs(state1.Diff(state2));
		assert(diff.time == 0 || diff.point == Point());
		return diff.time != 0 ? JointCost(diff.time) : diagonalUnitCost * std::min(diff.point.x,diff.point.y) + straightUnitCost * abs(diff.point.x - diff.point.y);
	}
	Path KeyPointSearch() {
		auto result = Path();
		if (world.begin == world.end) {
			result.success = true;
			result.steps.emplace_back(world.begin, INIT_COST);
			return result;
		}
		Graph graph(world);
		auto best = unordered_map<State, Visit>();
		State marker(-1, -1, -1); // recover end marker
		auto queue = priority_queue<Record, vector<Record>, Comparator>(Comparator(*this));
		// search
		queue.emplace(std::move(Record(world.begin, INIT_COST, marker)));
		while (!queue.empty()) {
			auto current = queue.top();
			queue.pop();
			auto visitRecord = best.find(current.state);
			if (visitRecord != best.end() && visitRecord->second.cost <= current.cost) {
				continue;
			}
			best.emplace(current.state, std::move(Visit(current.cost, current.last)));
			if (terminate == Terminate::AfterPop && current.state == world.end) {
				goto finish;
			}
			for (auto& neighbour : graph.Neighbours(current.state)) {
				if (!world.gridSize.Check(neighbour.point)) {
					continue;
				}
				auto newCost = current.cost + Cost(current.state, neighbour);
				if (terminate == Terminate::BeforePush && neighbour == world.end) {
					best.emplace(neighbour, std::move(Visit(newCost, current.state)));
					goto finish;
				}
				queue.emplace(std::move(Record(neighbour, newCost, current.state)));
			}
		}
	finish:
		// recover
		auto currentState = world.end;
		auto find = best.find(currentState);
		if (find == best.end()) { // destnation not reached
			return result;
		}
		result.success = true;
		Visit& trace = find->second;
		result.cost = trace.cost;
		while (true) {
			result.steps.emplace_back(currentState, trace.cost);
			if (trace.last == marker) {
				break;
			}
			currentState = trace.last;
			trace = best[trace.last];
		}
		std::reverse(result.steps.begin(), result.steps.end());
		return result;
	}
	Result Translate(const Path& path) const {
		auto result = Result();
		result.success = path.success;
		result.cost = path.cost;
		//result.keys = path;
		if (path.success) {
			result.steps.emplace_back(path.steps.front());
			for (auto it = path.steps.begin() + 1; it != path.steps.end(); it++) {
				auto& earlier = *(it - 1);
				auto& later = *it;
				assert(!(earlier.state == later.state));
				auto diff = later.state.Diff(earlier.state);
				if (diff.point.x == 0 && diff.point.y == 0) {
					auto current = later;
					current.cost = JointCost(abs(diff.time));
					result.steps.emplace_back(current);
				} else {
					auto current = earlier;
					//diagonal
					auto diagonalSteps = std::min(abs(diff.point.x), abs(diff.point.y));
					Point diagonalDirection(sign(later.state.point.x - current.state.point.x), sign(later.state.point.y - current.state.point.y));
					current.cost = diagonalUnitCost;
					for (auto i = 0; i < diagonalSteps; i++) {
						current.state.point.x += diagonalDirection.x;
						current.state.point.y += diagonalDirection.y;
						assert(world.gridSize.Check(current.state.point));
						result.steps.emplace_back(current);
					}
					//straight
					auto straightSteps = abs(abs(diff.point.x) - abs(diff.point.y));
					Point straightDirection(sign(later.state.point.x - current.state.point.x), sign(later.state.point.y - current.state.point.y));
					current.cost = straightUnitCost;
					for (auto i = 0; i < straightSteps; i++) {
						current.state.point.x += straightDirection.x;
						current.state.point.y += straightDirection.y;
						assert(world.gridSize.Check(current.state.point));
						result.steps.emplace_back(current);
					}
				}
			}
		}
		return result;
	}

protected:
	const CostUnit straightUnitCost;
	const CostUnit diagonalUnitCost;
	virtual CostUnit JointCost(const TimeUnit timeDiff) const noexcept = 0;
	virtual CostUnit Heuristic(const Record& record) const {
		return record.cost;
	}
	Searcher(const Terminate _terminate, const CostUnit _straightUnitCost, const CostUnit _diagonalUnitCost) : terminate(_terminate), straightUnitCost(_straightUnitCost), diagonalUnitCost(_diagonalUnitCost) {}
	

public:
	Result Search() {
		auto path = KeyPointSearch();
		auto result = Translate(path);
		return result;
	}
};

class BreadthFirstSearcher : public Searcher {//This is NOT the regular BFS, it does not go step by step.
protected:
	CostUnit JointCost(const TimeUnit timeDiff) const noexcept override {
		return 1;
	}

public:
	BreadthFirstSearcher() : Searcher(Terminate::AfterPop, UNIT_COST, UNIT_COST) {} //Do NOT use BeforePush!
};

class UnevenSearcher : public Searcher {
protected:
	CostUnit JointCost(const TimeUnit timeDiff) const noexcept override {
		assert(timeDiff >= 0);
		return timeDiff;
	}
	
public:
	UnevenSearcher(const Terminate _terminate) : Searcher(_terminate, STRAIGHT_UNEVEN_UNIT_COST, DIAGONAL_UNEVEN_UNIT_COST) {}
};

class UniformCostSearcher : public UnevenSearcher {
public:
	UniformCostSearcher() : UnevenSearcher(Terminate::AfterPop) {}
};

class AStarSearcher : public UnevenSearcher {
private:
	inline CostUnit EstimateCost(const State& state1, const State& state2) const {
		auto diff = abs(state1.Diff(state2));
		return  JointCost(diff.time) + diagonalUnitCost * std::min(diff.point.x, diff.point.y) + straightUnitCost * abs(diff.point.x - diff.point.y);
	}
	
protected:
	CostUnit Heuristic(const Record& record) const override {
		return record.cost + EstimateCost(record.state, world.end);
	}

public:
	AStarSearcher() : UnevenSearcher(Terminate::None) {}
};
#pragma endregion end search
#pragma region io
class Reader {
private:
	static void ProcLine1(const string& line) {//algorithm
		regex re("BFS|UCS|A\\*");
		std::smatch m;
		assert(std::regex_search(line, m, re));
		string v = m[0];
		if (v.compare("BFS") == 0) {
			world.algorithm = Algorithm::BreadthFirst;
		} else if (v.compare("UCS") == 0) {
			world.algorithm = Algorithm::UniformCost;
		} else if (v.compare("A*") == 0) {
			world.algorithm = Algorithm::AStar;
		} else {
			assert(false);
		}
	}

	static void ProcLine2(ifstream& f) {
		f >> world.gridSize.width >> world.gridSize.height;
	}

	static void ProcLine3(ifstream& f) {
		f >> world.begin.time >> world.begin.point.x >> world.begin.point.y;
	}

	static void ProcLine4(ifstream& f) {
		f >> world.end.time >> world.end.point.x >> world.end.point.y;
	}

	static Int32 ProcLine5(ifstream& f) {
		Int32 result;
		f >> result;
		return result;
	}

	static void ProcLineChannel(ifstream& f) {
		TimeUnit time1;
		CoordinateUnit x;
		CoordinateUnit y;
		TimeUnit time2;
		f >> time1 >> x >> y >> time2;
		world.channels.emplace_back(time1, x, y, time2);
	}

public:
	static void Read() {
		ifstream file(INPUT_FILENAME);
		assert(file.is_open());
		string line;
		assert(std::getline(file, line));
		ProcLine1(line);
		ProcLine2(file);
		ProcLine3(file);
		ProcLine4(file);
		auto channelNumber = ProcLine5(file);
		for (Int32 i = 0; i < channelNumber; i++) {
			ProcLineChannel(file);
		}
		file.close();
	}
};

class Writer {
public:
	static void Write(const Result& path) {
		ofstream file(OUTPUT_FILENAME);
		assert(file.is_open());
		if (!path.success) {
			file << "FAIL" << std::endl;
		} else {
			file << path.cost << std::endl;
			file << path.steps.size() << std::endl;
			for (auto& step : path.steps) {
				file << step.state.time << SPACE << step.state.point.x << SPACE << step.state.point.y << SPACE << step.cost << std::endl;
			}
		}
		file.close();
	}
};
#pragma endregion end io
#pragma endregion end class
#pragma region main
//=============================================//
//                                             //
//                   main                      //
//                                             //
//=============================================//

int main() {
	Reader::Read();
	Result result;
	switch (world.algorithm) {
	case Algorithm::BreadthFirst:
		result = BreadthFirstSearcher().Search();
		break;
	case Algorithm::UniformCost:
		result = UniformCostSearcher().Search();
		break;
	case Algorithm::AStar:
		result = AStarSearcher().Search();
		break;
	}
	Writer::Write(result);
	return 0;
}
#pragma endregion end main