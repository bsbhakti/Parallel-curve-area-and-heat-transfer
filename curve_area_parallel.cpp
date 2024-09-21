#include "core/utils.h"
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <thread>


#define sqr(x) ((x) * (x))
#define DEFAULT_NUMBER_OF_POINTS "1000000000"
#define DEFAULT_A "2"
#define DEFAULT_B "1"
#define DEFAULT_RANDOM_SEED "1"
#define DEFAULT_THREADS "1"
#define nullptr nullptr

struct thread_args{
    unsigned long n_points;
    uint random_seed;
    float a;
    float b;
    double time_taken;
    double local_curve_points;
} ;


std::atomic<unsigned long> curve_points;
uint c_const = (uint)RAND_MAX + (uint)1;


inline double get_random_coordinate(uint *random_seed) {
  return ((double)rand_r(random_seed)) / c_const;  // thread-safe random number generator
}

void * get_points_in_curve(void *arguments) {
    timer serial_timer;
    serial_timer.start();
    thread_args *args = (thread_args *) arguments;
    unsigned long n = args->n_points;
    unsigned long curve_count = 0;
    uint random_seed = args->random_seed;
    float a = args->a;
    float b = args->b;
    
  
   double x_coord, y_coord;
   for (unsigned long i = 0; i < n; i++) {
    //    std::cout << "I am here inside making point " << i << std::endl;
       x_coord = ((2.0 * get_random_coordinate(&random_seed)) - 1.0);
       y_coord = ((2.0 * get_random_coordinate(&random_seed)) - 1.0);
       if ((a*sqr(x_coord) + b*sqr(sqr(y_coord))) <= 1.0)
       curve_count++;
  }
    curve_points +=  curve_count;
    args->local_curve_points = curve_count;
    args->time_taken = serial_timer.stop();
    return nullptr;
}

void curve_area_calculation_serial(unsigned long n, float a, float b, uint r_seed, uint n_threads) {
  timer serial_timer;
  
  double time_taken = 0;
  uint random_seed = r_seed;
  uint each_thread_points = n/n_threads;
  uint remainder = n % n_threads;
  curve_points = 0;
  std::vector<std::thread> all_threads(n_threads);
  thread_args *all_arguments = new thread_args [n_threads]; 

//   std::cout <<"Each thread will make "<< each_thread_points <<" points"<<std::endl;
  serial_timer.start();

  for(int i=0; i<n_threads; i++){
    all_arguments[i].a = a;
    all_arguments[i].b = b;
    all_arguments[i].n_points = each_thread_points;
    if(i == 0){
      all_arguments[i].n_points += remainder;
    }
    all_arguments[i].random_seed = r_seed + i;
    all_arguments[i].time_taken = 0;
    all_arguments[i].local_curve_points = 0;
    // std::cout<<"Making thread "<<i<<std::endl;
    std::thread new_thread(get_points_in_curve,(void*)&all_arguments[i]);
    all_threads.push_back(std::move(new_thread));
  }

     for (auto& thread : all_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

//   unsigned long curve_points = get_points_in_curve(n, r_seed, a, b);
  
    double area_value = 4.0 * (double)curve_points / (double)n;
    time_taken = serial_timer.stop();

  //*------------------------------------------------------------------------
  
  std::cout << "thread_id, points_generated, curve_points, time_taken\n";
  for (int i = 0; i < n_threads; i++){
    std::cout<< i+1 <<", " << all_arguments[i].n_points << ", " <<std::fixed << std::setprecision(0) << all_arguments[i].local_curve_points
    << ", " << std::setprecision(TIME_PRECISION) << all_arguments[i].time_taken << "\n";
  }
  // std::cout << "1, " << n << ", "
  //             << curve_points << ", " << std::setprecision(TIME_PRECISION)
  //             << time_taken << "\n";

  std::cout << "Total points generated : " << n << "\n";
  std::cout << "Total points in curve : " << curve_points << "\n";
  std::cout << "Area : " << std::setprecision(VAL_PRECISION) << area_value
            << "\n";
  std::cout << "Time taken (in seconds) : " << std::setprecision(TIME_PRECISION)
            << time_taken << "\n";
}

int main(int argc, char *argv[]) {
  // Initialize command line arguments
  cxxopts::Options options("Curve_area_calculation",
                           "Calculate area inside curve a x^2 + b y ^4 = 1 using serial and parallel execution");
  options.add_options(
      "custom",
      {
          {"nPoints", "Number of points",         
           cxxopts::value<unsigned long>()->default_value(DEFAULT_NUMBER_OF_POINTS)},
	        {"coeffA", "Coefficient a",
	         cxxopts::value<float>()->default_value(DEFAULT_A)},
          {"coeffB", "Coefficient b",
           cxxopts::value<float>()->default_value(DEFAULT_B)},
          {"rSeed", "Random Seed",
           cxxopts::value<uint>()->default_value(DEFAULT_RANDOM_SEED)},
            {"nThreads", "Number of Threads",
           cxxopts::value<uint>()->default_value(DEFAULT_THREADS)}
      });
  auto cl_options = options.parse(argc, argv);
  unsigned long n_points = cl_options["nPoints"].as<unsigned long>();
  float a = cl_options["coeffA"].as<float>();
  float b = cl_options["coeffB"].as<float>();
  uint r_seed = cl_options["rSeed"].as<uint>();
  uint nThreads = cl_options["nThreads"].as<uint>();

  std::cout << "Number of points : " << n_points << "\n";;
  std::cout << "Number of threads : " << nThreads << "\n";;
  std::cout << "A : " << a << "\n" << "B : " << b << "\n";
  std::cout << "Random Seed : " << r_seed << "\n";

  curve_area_calculation_serial(n_points, a, b, r_seed,nThreads );
  return 0;
}
