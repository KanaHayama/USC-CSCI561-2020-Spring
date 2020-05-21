# Strategy

Search all states and record the best action for step 0 to 14 (with 4GB helper files, 8GB is the file size limitation set by TAs, 0 to 15 requires more than 10GB).
Look up best actions if step is less or equal than 14, and search till the ending of the game if step is grater than 14 (can be done very fast).
Minimax search with full-history evaluation function is used when unexpected situation occurred.

# Search Optimizations

+ Isomorphism induction: reduce state space.
+ Bitwise operations: game board can be encoded compactly and compared efficiently.
+ Multi-threading: the speed up is nearly P. Intel TBB is utilized.
+ LRU cache: Although I do not have enough memory to store all states for steps near endings, I can use LRU cache to help reduce work load.
+ Pruning: search essential technique.
+ Time planning: record time spent in each step to know the maximum amount of time to calculate.
+ Iterative deepening search: give at least a valid move to avoid terminated by game host with out any output and lose.

# State Expand Order

Sort states and choose one to expand in real-time can reduce the space requirement, but may dramatically slow down speed in parallel.

# HW2 Competition

The competition is canceled and the message is announced on 27 Apr 2020, after I devoted more than 600 hundred dollars to rent HPC.
I am so sad with the decision, I spend a lot of time on this and I am sure that I would win to prove myself.
Also, I really need a valuable offer ...

# Search Results

Results are organized based on passed number of steps and stored in the `truth` directory.
Search not completed, but covered 4 out of 6 conditions after the first move.
White seems to have a winning strategy.
I believe the result I have is enough to win this competition.
Other step's result files not uploaded due to Github's file size limitation.
These raw win/lose result files should be converted into best action files before providing to the agent.
Multiple best actions are recorded in case of KO or consecutively PASS blocks the first best action.