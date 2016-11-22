#ifndef user_julia_hpp
#define user_julia_hpp

#include "puzzler/puzzles/julia.hpp"

#define __CL_ENABLE_EXCEPTIONS 	//For openCl wrappers
#include "CL/cl.hpp"

#include <fstream>		//To read in kernel files
#include <streambuf>

class JuliaProvider
  : public puzzler::JuliaPuzzle
{
public:
	JuliaProvider()
	{
		// Platform
		std::vector<cl::Platform> platforms;

		cl::Platform::get(&platforms);
		if(platforms.size()==0)
			throw std::runtime_error("No OpenCL platforms found.");

		
		std::cerr<<"Found "<<platforms.size()<<" platforms\n";
		for(unsigned i=0;i<platforms.size();i++){
			std::string vendor=platforms[i].getInfo<CL_PLATFORM_VENDOR>();
			std::cerr<<"  Platform "<<i<<" : "<<vendor<<"\n";
		}
		
		
		int selectedPlatform=0;
		if(getenv("HPCE_SELECT_PLATFORM")){
			selectedPlatform=atoi(getenv("HPCE_SELECT_PLATFORM"));
		}
		std::cerr<<"Choosing platform "<<selectedPlatform<<"\n";
		cl::Platform platform=platforms.at(selectedPlatform); 
		
		// Device
		std::vector<cl::Device> devices;
		platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);  
		if(devices.size()==0){
			throw std::runtime_error("No opencl devices found.\n");
		}
		
		std::cerr<<"Found "<<devices.size()<<" devices\n";
		for(unsigned i=0;i<devices.size();i++){
			std::string name=devices[i].getInfo<CL_DEVICE_NAME>();
			std::cerr<<"  Device "<<i<<" : "<<name<<"\n";
		}
		
		int selectedDevice=0;
		if(getenv("HPCE_SELECT_DEVICE")){
			selectedDevice=atoi(getenv("HPCE_SELECT_DEVICE"));
		}
		std::cerr<<"Choosing device "<<selectedDevice<<"\n";
		cl::Device device=devices.at(selectedDevice);
		
		// Context
		cl::Context context(devices);
		
		// Collect sources into program
		std::string kernelSource=LoadSource("kernel_julia.cl");

		cl::Program::Sources sources;   // A vector of (data,length) pairs
		sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1)); // push on our single string

		cl::Program program(context, sources);
		try{
			program.build(devices);
		}catch(...){
			for(unsigned i=0;i<devices.size();i++){
				std::cerr<<"Log for device "<<devices[i].getInfo<CL_DEVICE_NAME>()<<":\n\n";
				std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[i])<<"\n\n";
			}
			throw;
		}
	}

	virtual void Execute(
		       puzzler::ILog *log,
		       const puzzler::JuliaInput *input,
		       puzzler::JuliaOutput *output
		       ) const override {
	
		std::vector<unsigned> dest(input->width*input->height);
	  
		log->LogInfo("Starting");
		/************************	Julia Implementation Starts Here		*******************************/
		/*juliaFrameRender_Reference(
		  input->width,     //! Number of pixels across
		  input->height,    //! Number of rows of pixels
		  input->c,        //! Constant to use in z=z^2+c calculation
		  input->maxIter,   //! When to give up on a pixel
		  &dest[0]     //! Array of width*height pixels, with pixel (x,y) at pDest[width*y+x]
		);*/
		
		float dx=3.0f/input->width, dy=3.0f/input->height;

        for(unsigned y=0; y<input->height; y++){
            for(unsigned x=0; x<input->width; x++){
                // Map pixel to z_0
                complex_t z(-1.5f+x*dx, -1.5f+y*dy);

                //Perform a julia iteration starting at the point z_0, for offset c.
                //   z_{i+1} = z_{i}^2 + c
                // The point escapes for the first i where |z_{i}| > 2.

                unsigned iter=0;
                while(iter<input->maxIter){
                    if(abs(z) > 2){
                        break;
                    }
                    // Anybody want to refine/tighten this?
                    z = z*z + input->c;
                    ++iter;
                }
                dest[y*input->width+x] = iter;
            }
        }
		
		/************************	Julia Implementation Ends Here		**********************************/
		log->LogInfo("Mapping");
	  
		log->Log(Log_Debug, [&](std::ostream &dst){
			dst<<"\n";
			for(unsigned y=0;y<input->height;y++){
				for(unsigned x=0;x<input->width;x++){
					unsigned got=dest[y*input->width+x];
					dst<<(got%9);
				}
				dst<<"\n";
			}
		});
		log->LogVerbose("  c = %f,%f,  arg=%f\n", input->c.real(), input->c.imag(), std::arg(input->c));
	  
		output->pixels.resize(dest.size());
		for(unsigned i=0; i<dest.size(); i++){
			output->pixels[i] = (dest[i]==input->maxIter) ? 0 : (1+(dest[i]%256));
		}
	  
		log->LogInfo("Finished");
	}

private:
	std::string LoadSource(const char *fileName)
	{
		// TODO : Don't forget to change your_login here
		std::string baseDir="provider";
		if(getenv("HPCE_CL_SRC_DIR")){
			baseDir=getenv("HPCE_CL_SRC_DIR");
		}

		std::string fullName=baseDir+"/"+fileName;

		// Open a read-only binary stream over the file
		std::ifstream src(fullName, std::ios::in | std::ios::binary);
		if(!src.is_open())
			throw std::runtime_error("LoadSource : Couldn't load cl file from '"+fullName+"'.");

		// Read all characters of the file into a string
		return std::string(
			(std::istreambuf_iterator<char>(src)), // Node the extra brackets.
			std::istreambuf_iterator<char>()
		);
	}
};

#endif
