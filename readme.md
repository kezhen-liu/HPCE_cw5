# HPCE 2016 CW5

This documents describes the work partitions, Testing and verification methodologies  and detailed measured to optimize/speed up those puzzles provided.

## 0. Work partitions and plan etc.
--------------
As I'm the singleton in this year's HPCE class, all the work including analyzing, coding, testing and verification are done by myself. However due to time constrains, I only mannage to finish three out of four puzzles' optimization, which are, random_walk, Ising_spin and Julia. 

## 1. Random_walk
----------
### Computational expensive part analysis
In random Walk puzzle, the part where we calculate number of counts on each nodes is the most computational intensive part
### GPU or CPU?
Here I decided to go for parallel-for on CPU. The reasons are
- Its algorithm involves a lot of memory reads and write on non-primitive structures, i.e. nodes, costing a lot of time to copy memory between cpu and gpu
- There are memory racing on nodes.count
- There are some dependency on random number chain, more hassle if we unroll it in cpu and copy over to gpu

### Solve the problems
Now we chose to use parrallel-for implementation. To solve the above problems, I did the following
- Unroll nodes structure. We now have a independent vector of atomic unsigned to hold the counts on each node
- To parrallel over numSamples, we unrolled the random number generator (chain) by a sequential for before parrallel for

### Testing and modifying
- Considering using grain-size parrallel rather than auto-partitioned for the parrallel loop over numSamples. However later I found that it is really difficult to find a optimal grain size. The reason is, the work load in each iteration is NOT fixed but depending on Input.scale. As a result I switched back to auto partitioner
- Apart from the parralel for over numSamples, there is actually another nested for loop inside. I tested using a parrallel-over-both and found that it actually took longer to execute. As a result I removed the inner parrallel
- The for loop at the very beginning used to unroll random number generator CANNOT be parralleled, or breaking its sequence/order
- I added another parrallel for at where histogram is constructed. Since the instruction executed in each iteration is relatively small, I found a grain size of 4096 is one of the optimal solutions


## 2. Ising spin model
### Computational expensive part analysis
There are actually three nested for loops in the code, where they loop over repeats, maxTime and the entire Ising Model space respectively. 

### Can those loops be paralleled?
Starting from the inner most loop, which is the loop over Ising model, though each Ising site depends on its four surronding neighbors, and to match with reference solution restricts the order where each site can be iterated, we can still re-order the loop, by mapping the entire iteration space to a different coordinates, (from x, y to a skewed j k coordinates), without breaking the execution sequence specified by the reference function.
The loop over maxTime cannot be parralleld, as far as I can see. The reason is the input of the next step is always depending on the results of previous step (Sounds quite farmiliar with one of our previous coursework which uses opencl? However we don't use opencl here, the reason will be mentioned in the following sections), making it impossible to parrallel
I just realized that the out-most loop can be parralleled at the last minute, by unrolling rng() like the ransom walk puzzle we mentioned above, but don't have time to implement it.

### Why not GPU?
Memory racing. As I said there are dependencies on surronding Ising sites.

### Testing and mods
- grain size on Ising space parrallel? No, through testing there is not much speed up by introducing grain size, probably because after mapping to a new coordinates, the number of elements on each row varies.
- parrallel on mean and standard deviations? Yes, though very small speed up through testing. Here I estimated there are around 30 intructions per interation when calculating mean and stddev, thus chosed a grain size of 512, which turned out to be quite good.


## 3. Julia
I'm less confident on Julia to be honest but would like to try for a opencl implementation

### Computational expensive part
Clearly it is the julia iteration part. No memory racing, uniform memory reads and only need one result copy from GPU to CPU, making it very suitable for a GPU implementaion.

### The overhead cost
We all know the price to pay for a GPU speedup is the time spent on initial set up. In order to reduce this amount of time, I moved most of its GPU setup part of code to JuliaProvider constructors rather than residing in execute function. Some of the varables/objects kernel program needs are passed by JuliaProvider class' members I added. I'm not that farmiliar with c++ and this parts took me alot of time.

### Float number issues...
I didn't make much progress on this to be honest, although through reading opencl's reference, I found using hypot(z_x, z_y) function seems to have a better result comparing with using direct formulars.
- Issued: Fri 11th Nov
- Due: Fri 25th Nov, 22:00

Previous readme.md
----------------------------
Specification
-------------

You have been given the included code with the
goal of making things faster. For our purposes,
faster means the wall-clock execution time of
`puzzler::Puzzle::Execute`, across a broad spectrum
of scale factors. The target platform is an
AWS GPU (g2.2xlarge) instance, and the target AMI
will be the public HPCE-2015-v2 AMI. The AMI has
OpenCL GPU and software providers installed, alongside
TBB. You can determine the location of headers and
libraries by starting up the AMI.

Note that there are AWS credits available for you for
free from the Amazon Educate programme - you shouldn't
be paying anything to use them. You may want to look
back over the [AWS notes from CW2](https://github.com/HPCE/hpce-2016-cw2/blob/master/aws.md).

Pair-work
---------

This coursework is intended to be performed in pairs - based on
past student discussions these pairs are organised by the
students themselves. Usually everyone has already paired themselves
up, but if anyone is having difficulty, then let me know and
I may be able to help. Singletons are sometimes required (e.g.
an odd sized class), in which case I make some allowances for
the reduced amount of time available during marking. Trios are
also possible, but things go in the other direction (and trios
are generally much less efficient).

To indicate who you are working with, each member of the
pair should give write access to their hpce-2016-cw5-login
repository to the other person. You should have admin
control over your own account, so in the github page for
the repo got to Settings->Collaborators and Teams, or
go to:

    https://github.com/HPCE/hpce-2016-cw5-[LOGIN]/settings/collaboration
    
You can then add you partner as a collaborator.

During development try to keep the two accounts in sync if possible.
For the submission I will take the last commit in either repository
that is earlier than 22:00.

Meta-specification
------------------

You've now got some experience in different methods
for acceleration, and a decent working knowledge
about how to transform code in reasonable reliable
ways. This coursework represents a fairly common
situation - you haven't got much time, either to analyse
the problem or to do low-level optimisation, and the problem
is actually a large number of sub-problems. So the goal
here is to identify and capture as much of the low-hanging
performance fruit as possible while not breaking anything.

The code-base I've given you is somewhat baroque,
and despite having some rather iffy OOP practises,
actually has things quite reasonably
isolated. You will probably encounter the problem
that sometimes the reference solution starts to take
a very long time at large scales, but the persistence
framework gives you a way of dealing with that.

Beyond that, there isn't a lot more guidance, either
in terms of what you should focus on, or how
_exactly_ it will be measured. Part of the assesment
is in seeing whether you can work out what can be
accelerated, and where you should spend your time.

The allocation of marks I'm using is:

- Compilation/Execution: 10%

  - How much work do I have to do to get it to compile and run.

- Performance: 60%

  - You are competing with each other here, so there is an element of
    judgement in terms of how much you think others are doing or are
    capable of.

- Correctness: 30%

  - As far as I'm aware the ReferenceExecute is always correct, though slow.

Deliverable format
------------------

- Your repository should contain a readme.txt, readme.pdf, or readme.md covering:

    - What is the approach used to improve performance, in terms of algorithms, patterns, and optimisations.

    - A description of any testing methodology or verification.

    - A summary of how work was partitioned within the pair, including planning, analysis, design, and testing, as well as coding.

- Anything in the `include` directory is not owned by you, and subject to change

  - Any changes will happen in an additive way (none are expected for this CW)

  - Bug-fixes to `include` stuff are still welcome.
  
- You own the files in the `provider` directory

  - You'll be replacing the implementation of `XXXXProvider::Execute` in `provider/xxxx.hpp`
    with something (hopefully) faster.
  
  - A good starting point is to replace the implementation of `XXXXProvider::Execute` with a copy
    of the body of `XXXXPuzzle::ReferenceExecute`, and check that it still does the same thing.
    
  - The reason for the indirection is to force people to have an unmodified reference version
    available at all times, as it tends to encourage testing.

- The public entry point to your code is via `puzzler::PuzzleRegistrar::UserRegisterPuzzles`,
    which must be compiled into the static library `lib/libpuzzler.a`.

    - Clients will not directly include your code, they will only `#include "puzzler/puzzles.h`,
      then access puzzles via the registrar. They will get access to the registrar implementation
      by linking against `lib/libpuzzler.a`.

    - **Note**: If you do something complicated in your building of libpuzzler, it should still be
      possible to build it by going into `lib` and calling `make all`.

    - The current working directory during execution will be the root of the repository. So
      it will be executed as if typing `bin/execute_puzzle`, and an opencl kernel could be
      loaded using the relative path `provider/something.kernel`.

- The programs in `src` have no special meaning or status, they are just example programs

The reason for all this strange indirection is that I want to give
maximum freedom for you to do strange things within your implementation
(example definitions of "strange" include CMake) while still having a clean
abstraction layer between your code and the client code.

Intermediate Testing
--------------------

I'll be occasionally pulling and running tests on all the repositories, and
pushing the results back. These tests do _not_ check for correctness, they only check
that the implementations build and run correctly (and are also for my own interest
in seeing how performance evolves over time) I will push the results into
the `dt10_runs` directory.

If you are interested in seeing comparitive performance results, you can opt in
by commiting a file called `dt10_runs/count_me_in`. This will result in graphs with
lines for your implementation versus others who also opted in, but you will only be able
to identify your line on the graph graph.

I will pull from the "master" branch, as this reflects better working practise - if
there is a testing branch, then that is where the unstable code should
be. The master branch should ideally always be compilable and correct, and
branches only merged into master once they are stable.

Finally, to re-iterate: the tests I am doing do _no_ testing at all for correctness, they
don't even look at the output of the tests.
