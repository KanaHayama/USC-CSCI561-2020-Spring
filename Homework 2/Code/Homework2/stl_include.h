//Name: Zongjian Li, USC ID: 6503378943
#pragma once

//enable counting encapsulated area
//#define FULL

#ifdef _MSC_VER
//gcc compiler <immintrin.h> support not enabled on the test platform
#include <immintrin.h>
#define __builtin_popcountll _mm_popcnt_u64
#endif

#ifdef _MSC_VER
#include <cassert>
#else
#define assert 
#endif

#include <algorithm>
#include <array>
#include <tuple>
#include <queue>
#include <random>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdlib>
#include <atomic>
#include <iostream>
#include <regex>
#include <chrono> 

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::array;
using std::map;
using std::string;
using std::ifstream;
using std::ofstream;
using std::mutex;
using std::recursive_mutex;
using std::thread;
using std::vector;
using std::regex;
using std::atomic;
using std::tuple;
using std::pair;
using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::time_point;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;

//#include <cassert>

const static string HELPER_FILE_EXTENSION = ".txt";