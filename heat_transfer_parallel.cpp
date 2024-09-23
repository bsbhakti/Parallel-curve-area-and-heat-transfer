#include "core/utils.h"
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <thread>


#define DEFAULT_GRID_SIZE "1000"
#define DEFAULT_NUMBER_OF_THREADS "1"
#define DEFAULT_CX "1"
#define DEFAULT_CY "1"
#define DEFAULT_TIME_STEPS "1000"
#define DEFAULT_MIDDLE_TEMP "600"
#define nullptr nullptr


CustomBarrier *barrier = nullptr;


class TemperatureArray {
private:
	uint size;
	uint step;
	double Cx;
	double Cy;
	double *CurrArray;
	double *PrevArray;
  void assign(double *A, uint x, uint y, double newvalue) {
    A[x*size+y] = newvalue;
  };
  double read(double *A, uint x, uint y) {
    return A[x*size+y];
  }; 
public:	
	TemperatureArray(uint input_size, double iCx, double iCy, double init_temp) { // create array of dimension sizexsize
		size = input_size;
 		Cx = iCx;
    		Cy = iCy;
		step = 0;
		CurrArray = (double *)malloc(size*size*sizeof(double));
		PrevArray = (double *)malloc(size*size*sizeof(double));
		for (uint i = 0; i < size; i++)
			for (uint j = 0; j < size; j++) {
				if ((i > size/3) && (i < 2*size/3) && (j > size/3) && (j < 2*size/3)) {
					assign(PrevArray, i, j, init_temp); assign(CurrArray, i, j, init_temp);
				}
				else {
					assign(PrevArray, i, j, 0); assign (CurrArray, i, j, 0);
				}	
			}
	};
 
	~TemperatureArray() {
		free (PrevArray);   free (CurrArray);
	};

	void IncrementStepCount() { step ++; };

	uint ReadStepCount() { return(step); };

	void ComputeNewTemp(uint x, uint y) {
    double prev1 = read(PrevArray,x,y);
    double prev2 = read(PrevArray,x-1,y);
    double prev3 = read(PrevArray,x,y-1);
    double prev4 = read(PrevArray,x,y+1);
    double prev5 = read(PrevArray,x+1,y);
		if ((x > 0) && (x < size-1) && (y > 0) && (y < size-1))
			assign(CurrArray, x, y , prev1	+ Cx * (prev2 + prev5 - 2*prev1) + Cy * (prev3 + prev4 - 2*prev1));
	};

	void SwapArrays() {
		double *temp = PrevArray;
		PrevArray = CurrArray;
		CurrArray = temp;
	};	
 
	double temp(uint x, uint y) {
		return read(CurrArray, x, y);
	};
};

struct thread_args {
    uint tid;
    uint size;
    uint start;
    uint end;
    double time_taken;
    TemperatureArray* T;
    uint steps;
};

inline void heat_transfer_calculation(thread_args *all_arguments) {
  // std::cout<<"Inside heat_transfer"<<std::endl;
  uint tid = all_arguments->tid;
  uint size = all_arguments->size;
  uint start = all_arguments->start;
  uint end = all_arguments->end;
  TemperatureArray* T = all_arguments->T;
  uint steps = all_arguments->steps;
  timer local;
  local.start();
  uint stepcount;
  for (stepcount = 1; stepcount <= steps; stepcount ++) {
	  for (uint x = start; x <= end; x++) {
		  for (uint y = 0; y < size; y++) {
			  T->ComputeNewTemp(x, y);
		  }
	  }
    //barrier wait
    barrier->wait();

    if (tid == 0) {
        // thread 0 should swap arrays. This is the only thread in the serial version
            T->SwapArrays();
            T->IncrementStepCount();
            barrier->wait();
        }
    else {  
        // other threads should wait until swap is complete
            barrier->wait();
        }
    }  // end of current step
    all_arguments->time_taken = local.stop(); //loop ends
}

void heat_transfer_calculation_serial(uint size, uint number_of_threads, TemperatureArray* T, uint steps) {
  timer serial_timer;
  double time_taken = 0.0;
  std::vector<uint> startx(number_of_threads);
  std::vector<uint> endx(number_of_threads);
  std::vector<std::thread> all_threads(number_of_threads);
//   std::thread *threads = new std::thread [number_of_threads]; 
  thread_args *all_arguments = new thread_args [number_of_threads]; 



  // The following code is used to determine start and end of each thread's share of the grid
  // Also used to determine which points to print out at the end of this function
  uint min_columns_for_each_thread = size /   number_of_threads;
  uint excess_columns = size % number_of_threads;
  uint curr_column = 0;

  for (uint i = 0; i < number_of_threads; i++) {
    startx[i] = curr_column;
    if (excess_columns > 0) {
      endx[i] = curr_column + min_columns_for_each_thread;
      excess_columns--;
      } 
    else {
           endx[i] = curr_column + min_columns_for_each_thread - 1;
      }
    curr_column = endx[i]+1;
  } 
  // end of code to determine start and end of each thread's share of the grid


  serial_timer.start();
  //*------------------------------------------------------------------------
  for(int i=0; i < number_of_threads; i++){
      all_arguments[i].start = startx[i];
      all_arguments[i].end = endx[i];
      all_arguments[i].size = size;
      all_arguments[i].steps = steps;
      all_arguments[i].T  = T;
      all_arguments[i].tid = i;
      all_arguments[i].time_taken = 0.0;
      std::thread new_thread(heat_transfer_calculation,&(all_arguments[i]));
      all_threads.push_back(std::move(new_thread));
  }
  // std::cout<<"Total number of threads "<<all_threads.size()<<std::endl;

  for (auto& thread : all_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    // std::cout<<"Done Joining threads"<<std::endl;
  // Print these statistics foall_argumentsr each thread 
    std::cout << "thread_id, start_column, end_column, time_taken\n";
  for(int i=0;i<number_of_threads;i++){
    std::cout << i<<", "<<all_arguments[i].start<<", "<<all_arguments[i].end<<", "<<all_arguments[i].time_taken<<"\n";
  }
 
  
  uint step = size/6;
  uint position = 0;
  for (uint x = 0; x < 6; x++) {
      std::cout << "Temp[" << position << "," << position << "]=" << T->temp(position,position) << std::endl;
      position += step;
  } 
  
  // Print temparature at select boundary points;
  for (uint i = 0; i < number_of_threads; i++) {
      std::cout << "Temp[" << endx[i] << "," << endx[i] << "]=" << T->temp(endx[i],endx[i]) << std::endl;
  }

  //*------------------------------------------------------------------------
  time_taken = serial_timer.stop();

  std::cout << "Time taken (in seconds) : " << std::setprecision(TIME_PRECISION)
            << time_taken << "\n";
  delete [] all_arguments;
}

int main(int argc, char *argv[]) {
  // Initialize command line arguments
  cxxopts::Options options("Heat_transfer_calculation",
                           "Model heat transfer in a grid using serial and parallel execution");
  options.add_options(
      "custom",
      {
          {"nThreads", "Number of threads",
           cxxopts::value<uint>()->default_value(DEFAULT_NUMBER_OF_THREADS)},
          {"gSize", "Grid Size",         
           cxxopts::value<uint>()->default_value(DEFAULT_GRID_SIZE)},
          {"mTemp", "Temperature in middle of array",         
           cxxopts::value<double>()->default_value(DEFAULT_MIDDLE_TEMP)},
	        {"iCX", "Coefficient of horizontal heat transfer",
           cxxopts::value<double>()->default_value(DEFAULT_CX)},
          {"iCY", "Coefficient of vertical heat transfer",
           cxxopts::value<double>()->default_value(DEFAULT_CY)},
          {"tSteps", "Time Steps",
           cxxopts::value<uint>()->default_value(DEFAULT_TIME_STEPS)}
      });
  auto cl_options = options.parse(argc, argv);
  uint n_threads = cl_options["nThreads"].as<uint>();
//   if (n_threads != 1) {
// 	std::cout << "Serial version. Number of threads should be equal to 1. Terminating..." << std::endl;
// 	return 1;
//   }
  uint grid_size = cl_options["gSize"].as<uint>();
  double init_temp = cl_options["mTemp"].as<double>();
  double Cx = cl_options["iCX"].as<double>();
  double Cy = cl_options["iCY"].as<double>();
  uint steps = cl_options["tSteps"].as<uint>();
  std::cout << "Grid Size : " << grid_size << "x" << grid_size << std::endl;
  std::cout << "Number of threads : " << n_threads << std::endl;
  std::cout << "Cx : " << Cx << std::endl << "Cy : " << Cy << std::endl;
  std::cout << "Temperature in the middle of grid : " << init_temp << std::endl;
  std::cout << "Time Steps : " << steps << std::endl;
  
  std::cout << "Initializing Temperature Array..." << std::endl;
  TemperatureArray *T = new TemperatureArray(grid_size, Cx, Cy, init_temp);
  if (!T) {
      std::cout << "Cannot Initialize Temperature Array...Terminating" << std::endl;
      return 2;
  }
  barrier = new CustomBarrier((int)n_threads);
  heat_transfer_calculation_serial (grid_size, n_threads, T, steps);

  delete T;
  delete barrier;

  return 0;
}
