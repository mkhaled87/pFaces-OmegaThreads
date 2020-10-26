/*
 * TicToc.hh
 *
 *  created on: 02.10.2015
 *      author: rungger
 */

#ifndef TICTOC_HH_
#define TICTOC_HH_

#include <iostream>
#include <chrono>

/* class: TicToc 
 * helper class to measure elapsed time based on std::chrono library */
class TicToc {
  private:
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point stop;
  public:
    TicToc(){};
    ~TicToc(){};

    /* function: tic 
     * set start time 
     */
    inline void tic(void) {
      start=std::chrono::high_resolution_clock::now();
    }
    /* function: toc 
     * set stop time and print out elapsed time since last call of tic()
     */
    inline void toc(void) {
      stop=std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> dt;
      dt=std::chrono::duration_cast<std::chrono::duration<double > >(stop-start);
      std::cout << "Elapsed time is " << dt.count() << " seconds." << std::endl;
    }
};

#endif /* TICTOC_HH_ */
