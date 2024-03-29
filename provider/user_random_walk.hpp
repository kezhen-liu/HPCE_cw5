#ifndef user_random_walk_hpp
#define user_random_walk_hpp

#include "puzzler/puzzles/random_walk.hpp"
#include "tbb/parallel_for.h"

class RandomWalkProvider
  : public puzzler::RandomWalkPuzzle
{
public:
  RandomWalkProvider()
  {}

  virtual void Execute(
		       puzzler::ILog *log,
		       const puzzler::RandomWalkInput *input,
		       puzzler::RandomWalkOutput *output
		       ) const override {
    
				   //memory intensive, going to use tbb
	// Take a copy, as we'll need to modify the "count" flags
      std::vector<puzzler::dd_node_t> nodes(input->nodes);
      /*
      log->Log(Log_Debug, [&](std::ostream &dst){
        dst<<"  Scale = "<<nodes.size()<<"\n";
        for(unsigned i=0;i<nodes.size();i++){
          dst<<"  "<<i<<" -> [";
          for(unsigned j=0;j<nodes[i].edges.size();j++){
            if(j!=0)
              dst<<",";
            dst<<nodes[i].edges[j];
          }
          dst<<"]\n";
        }
      });

      log->LogVerbose("Starting random walks");
		*/
	  /************************************** Random Walk Implementation Starts	*************************/
	  std::vector<tbb::atomic<uint32_t> > nodeCount(nodes.size(), 0);
	  
      // This gives the same sequence on all platforms
      std::mt19937 rng(input->seed);
	  unsigned seed[input->numSamples], start[input->numSamples];
	  unsigned length=input->lengthWalks;           // All paths the same length
	  
      for(unsigned i=0; i<input->numSamples; i++){
        seed[i]=rng();
        start[i]=rng() % nodes.size();    // Choose a random node
	  }
	  tbb::parallel_for(0u,(unsigned)input->numSamples,[&](unsigned i){
        //random_walk(nodes, seed[i], start[i], length);
		/*	Unroll and rewrite random_walk() */
		uint32_t rng=seed[i];
        unsigned current=start[i];
        for(unsigned k=0; k<length; k++){
          nodeCount[current]++;

          unsigned edgeIndex = rng % nodes[current].edges.size();
          rng=rng*1664525+1013904223;	//step rng
        
          current=nodes[current].edges[edgeIndex];
        }
	  });

	  /************************************** Random Walk Implementation Ends	*************************/
      //log->LogVerbose("Done random walks, converting histogram");

      // Map the counts from the nodes back into an array
      output->histogram.resize(nodes.size());
      //for(unsigned i=0; i<nodes.size(); i++){
	  //tbb::parallel_for(0u,(unsigned)nodes.size(),[&](unsigned i){
	  tbb::parallel_for(tbb::blocked_range<unsigned>(0u,(unsigned)nodes.size(),4096), [&](const tbb::blocked_range<unsigned> &chunk){
		for(unsigned i=chunk.begin(); i!=chunk.end(); i++){
        output->histogram[i]=std::make_pair(uint32_t(nodeCount[i]),uint32_t(i));
        //nodes[i].count=0;
		}
      },tbb::simple_partitioner());
	  
      // Order them by how often they were visited
      std::sort(output->histogram.rbegin(), output->histogram.rend());
      
	/*
      // Debug only. No cost in normal execution
      log->Log(Log_Debug, [&](std::ostream &dst){
        for(unsigned i=0; i<output->histogram.size(); i++){
          dst<<"  "<<i<<" : "<<output->histogram[i].first<<", "<<output->histogram[i].second<<"\n";
        }
      });

      log->LogVerbose("Finished"); */
  }

};

#endif
