#ifndef user_ising_spin_hpp
#define user_ising_spin_hpp

#include "puzzler/puzzles/ising_spin.hpp"
#include "tbb/parallel_for.h"

class IsingSpinProvider
  : public puzzler::IsingSpinPuzzle
{
private:
  void mInit(
      const puzzler::IsingSpinInput *pInput,
      uint32_t &seed,
      int *out
    ) const {
      unsigned n=pInput->n;
      
      for(unsigned x=0; x<n; x++){
        for(unsigned y=0; y<n; y++){
          out[y*n+x] = (seed < 0x80001000ul) ? +1 : -1;
          seed = mLcg(seed);
        }
      }
    }
	
	void mDump(
      int logLevel,
      const puzzler::IsingSpinInput *pInput,
      int *in,
      puzzler::ILog *log
    ) const {
      if(logLevel > log->Level())
        return;
      
      unsigned n=pInput->n;
      
      log->Log(logLevel, [&](std::ostream &dst){
        dst<<"\n";
        for(unsigned y=0; y<n; y++){
          for(unsigned x=0; x<n; x++){
            dst<<(in[y*n+x]<0?"-":"+");
          } 
          dst<<"\n";
        }
      });
    }
	
	void mStep(
      const puzzler::IsingSpinInput *pInput,
      uint32_t &seed,
      const int *in,
      int *out
    ) const {
      unsigned n=pInput->n;
      
      //for(unsigned x=0; x<n; x++){
        //for(unsigned y=0; y<n; y++){
	  std::vector<uint32_t> seeds(n*n,0);
	  //seeds[0]=seed;
	  for(unsigned x=0; x<n; x++){
        for(unsigned y=0; y<n; y++){
			seeds[x*n+y]=seed;
			seed = mLcg(seed);
		}
	  }
	  /*
	  for(unsigned x=0; x<n; x++){
        for(unsigned y=0; y<n; y++){
			int W = x==0 ?    in[y*n+n-1]   : in[y*n+x-1];
          int E = x==n-1 ?  in[y*n+0]     : in[y*n+x+1];
          int N = y==0 ?    in[(n-1)*n+x] : in[(y-1)*n+x];
          int S = y==n-1 ?  in[0*n+x]     : in[(y+1)*n+x];
          int nhood=W+E+N+S;
          
          int C = in[y*n+x];
          
          unsigned index=(nhood+4)/2 + 5*(C+1)/2;
          float prob=pInput->probs[index];

          if( seeds[x*n+y] < prob){
            C *= -1; // Flip
          }
          
          out[y*n+x]=C;
		}
	  }*/
	  
	  tbb::parallel_for(0u, n, [&](unsigned k){	//upper half
		for(unsigned j=0; j<=k; j++){
		  unsigned x =j, y=k-j;
          int W = x==0 ?    in[y*n+n-1]   : in[y*n+x-1];
          int E = x==n-1 ?  in[y*n+0]     : in[y*n+x+1];
          int N = y==0 ?    in[(n-1)*n+x] : in[(y-1)*n+x];
          int S = y==n-1 ?  in[0*n+x]     : in[(y+1)*n+x];
          int nhood=W+E+N+S;
          
          int C = in[y*n+x];
          
          unsigned index=(nhood+4)/2 + 5*(C+1)/2;
          float prob=pInput->probs[index];

          if( seeds[x*n+y] < prob){
            C *= -1; // Flip
          }
          
          out[y*n+x]=C;
          
          //seed = mLcg(seed);
        }
      });
	  tbb::parallel_for(n, (unsigned)(2*n-1),[&](unsigned k){
		  for(unsigned j=0; j<=2*n-2-k; j++){
			unsigned x=-n+1+k+j, y=n-1-j;
            int W = x==0 ?    in[y*n+n-1]   : in[y*n+x-1];
            int E = x==n-1 ?  in[y*n+0]     : in[y*n+x+1];
            int N = y==0 ?    in[(n-1)*n+x] : in[(y-1)*n+x];
            int S = y==n-1 ?  in[0*n+x]     : in[(y+1)*n+x];
            int nhood=W+E+N+S;
          
            int C = in[y*n+x];
          
            unsigned index=(nhood+4)/2 + 5*(C+1)/2;
            float prob=pInput->probs[index];

            if( seeds[x*n+y] < prob){
              C *= -1; // Flip
            }
          
            out[y*n+x]=C;
          
            //seed = mLcg(seed);  
		  }
	  });
    }
	
	// This has some interesting and maybe useful properties...
    // The recurrence equation is:
    //   x_{i+1} = x_i * 1664525 + 1013904223 mod 2^32
    uint32_t mLcg(uint32_t x) const
    {
      return x*1664525+1013904223;
    };
	
	int mCount(
      const puzzler::IsingSpinInput *pInput,
      const int *in
    ) const {
      unsigned n=pInput->n;
      
      return std::accumulate(in, in+n*n, 0);
    }
	
public:
  IsingSpinProvider()
  {}

  virtual void Execute(
		       puzzler::ILog *log,
		       const puzzler::IsingSpinInput *input,
		       puzzler::IsingSpinOutput *output
		       ) const override {
    //return ReferenceExecute(log, input, output);
	//memory racing, thus no gpu resolution, complex 4-way dependencies, thus re coordinated
	  int n=input->n;
      
      std::vector<int> current(n*n), next(n*n);
      
      std::vector<double> sums(input->maxTime, 0.0);
      std::vector<double> sumSquares(input->maxTime, 0.0);
      
      //log->LogInfo("Starting steps.");
      
      std::mt19937 rng(input->seed); // Gives the same sequence on all platforms
      uint32_t seed;
      for(unsigned i=0; i<input->repeats; i++){
        seed=rng();
        
        //log->LogVerbose("  Repeat %u", i);
        
        mInit(input, seed, &current[0]);
        
        for(unsigned t=0; t<input->maxTime; t++){
          //log->LogDebug("    Step %u", t);
          
          // Dump the state of spins on high log levels
          //mDump(puzzler::Log_Debug, input, &current[0], log);
          
          mStep(input, seed, &current[0], &next[0]);
          std::swap(current, next);
          
          // Track the statistics
          double countPositive=mCount(input, &current[0]);
          sums[t] += countPositive;
          sumSquares[t] += countPositive*countPositive;
        }
        
        seed=mLcg(seed);
      }
      
      //log->LogInfo("Calculating final statistics");
      
      output->means.resize(input->maxTime);
      output->stddevs.resize(input->maxTime);
      //for(unsigned i=0; i<input->maxTime; i++){
	  tbb::parallel_for(0u, (unsigned)input->maxTime,[&](unsigned i){
        output->means[i] = sums[i] / input->maxTime;
        output->stddevs[i] = sqrt( sumSquares[i]/input->maxTime - output->means[i]*output->means[i] );
        //log->LogVerbose("  time %u : mean=%8.6f, stddev=%8.4f", i, output->means[i], output->stddevs[i]);
      });
      
      //log->LogInfo("Finished");
  }

};

#endif
