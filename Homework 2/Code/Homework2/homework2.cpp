
#include "simulation.h"

#if defined SEARCH_MODE
	#include "search.h"
#elif defined INTERACT_MODE
	#include "interaction.h"
#else
	#include "agent.h"
#endif

int main(int argc, char* argv[]) {
	return run(argc, argv);
}