/*
 * CuddMintermIterator.hh
 *
 *  created on: 30.09.2015
 *      author: rungger
 */

#ifndef CUDDMINTERMITERATOR_HH_ 
#define CUDDMINTERMITERATOR_HH_ 

#include <iostream> 
#include <sstream> 
#include <vector> 
#include <cassert> 
#include <bitset> 
#include <stdexcept> 

#include "cuddObj.hh"
/* class: CuddMintermIterator
 *
 * an extension to the Cudd DdGen struct to iterate over all minterms of a bdd
 *
 * In the doc of Cudd_FirstCube/Cudd_Next a cube is described as an array of
 * literals, which are integers in {0, 1, 2}. 0 represents a complemented
 * literal, 1 represents an uncomplemented literal, and 2 stands for don't 
 * care. The enumeration produces a disjoint cover of the function associated
 * with the diagram. In this class the enumeration is extended to minterms. A 
 * minterm is a cube where each don't care 2 is replaced by either 0 or 1.
 *
 *
 */
class CuddMintermIterator {
  private:
    DdGen* gen;
    int* cube;
    CUDD_VALUE_TYPE value;
    bool done_;
    std::vector<size_t> ivars_;
    size_t nvars_;

    std::vector<size_t> idontcares;
    size_t ndontcares;
    size_t iterator;
    size_t nexpand;

    size_t progress;
    double nminterm;

    size_t counter_;
    
  public:
    /* constructor: CuddMintermIterator
     * iterate over minterms containing the variables defined by the indices
     * ivars
     *
     * the variables ivars must conatin all support variables of the bdd  */
    CuddMintermIterator(BDD bdd, std::vector<size_t> &ivars, size_t nvars) {
      ivars_=ivars;
      nvars_=nvars;
      done_=false;
      counter_=0;

      if(nvars_>sizeof(size_t)*8) {
          std::ostringstream os;
          os << "Error: CuddMintermIterator: number of variables we iterate over is limited to highest number in size_t.";
          throw std::invalid_argument(os.str().c_str());
      }

      /* check if bdd depens only on bdd variables with indices ivars */
      std::vector<unsigned int> sup = bdd.SupportIndices();
      for(size_t i=0; i<sup.size();i++) {
        int marker=0;
        for(size_t j=0; j<nvars; j++) {
          if (sup[i]==ivars[j])
            marker=1;
        }
        if(!marker) {
            std::ostringstream os;
            os << "Error: CuddMintermIterator: the bdd depends on variables with index outside ivars.";
            throw std::invalid_argument(os.str().c_str());
        }
      }
      /* initialize gen and grab first cube */
      DdManager *manager = bdd.manager();
      DdNode *node = bdd.getNode();
      gen = Cudd_FirstCube(manager, node , &cube, &value);
      if (Cudd_IsGenEmpty(gen)) {
        done_=true;
        return;
      }
      /* determine the number of minterms */
      nminterm=bdd.CountMinterm(nvars);
      progress=1;

      /* check cube for don't cares */
      idontcares.reserve(nvars);
      ndontcares=0;
      nexpand=1; 
      iterator=0;
      /* indices of the variables with don't cares */
      for(size_t i=0;i<nvars; i++) {
        if (cube[ivars[i]]==2) {
          idontcares[ndontcares]=ivars_[i];
          ndontcares++;
          nexpand*=2;
        }
      }
      /* if the cube contins don't cares, start expanding the cube to a minterm */
      if(ndontcares) {
        /* start with all zero entries */
        for(size_t i=0; i<ndontcares; i++)
          cube[idontcares[i]]=0;
      }
    }
    /* constructor: CuddMintermIterator
     * iterate over minterms containing the support variables */ 
    CuddMintermIterator(BDD bdd) {
      done_=false;

      std::vector<unsigned int> sup=bdd.SupportIndices();
      ivars_.assign(sup.begin(),sup.end());
      nvars_=ivars_.size();


      for(size_t i=0; i<nvars_;i++) 
        std::cout << ivars_[i] << " ";
      std::cout<<std::endl;

      /* number of variables we iterate over is limited to highest number in size_t  */
      if(nvars_>sizeof(size_t)*8) {
          std::ostringstream os;
          os << "Error: CuddMintermIterator: number of variables we iterate over is limited to highest number in size_t.";
          throw std::invalid_argument(os.str().c_str());
      }

      /* initialize gen and grab first cube */
      DdManager *manager = bdd.manager();
      DdNode *node = bdd.getNode();
      gen = Cudd_FirstCube(manager, node , &cube, &value);
      if (Cudd_IsGenEmpty(gen)) {
        done_=true;
        return;
      }
      /* determine the number of minterms */
      nminterm=bdd.CountMinterm(nvars_);
      progress=1.0;
      /* check cube for don't cares */
      idontcares.reserve(nvars_);
      ndontcares=0;
      nexpand=1; 
      iterator=0;
      /* indices of the variables with don't cares */
      for(size_t i=0;i<nvars_; i++) {
        if (cube[ivars_[i]]==2) {
          idontcares[ndontcares]=ivars_[i];
          ndontcares++;
          nexpand*=2;
        }
      }
      /* if the cube contins don't cares, start expanding the cube to a minterm */
      if(ndontcares) {
        /* start with all zero entries */
        for(size_t i=0; i<ndontcares; i++)
          cube[idontcares[i]]=0;
      }
    }
    ~CuddMintermIterator() {
      Cudd_GenFree(gen);
    }
    inline void operator++(void) {

      iterator++;
      progress++;
      /* get new cube or expand cube further */
      if (iterator<nexpand) {
        /* still not don with this cube */
        /* generate binary value for iterator to fill cube */
        std::bitset<sizeof(size_t)*8> iterator2binary(iterator);
        for(size_t i=0; i<ndontcares; i++)
          cube[idontcares[i]]=iterator2binary[i];
      } else {
        /* restore the cube (put the don't cares) */
        for(size_t i=0; i<ndontcares; i++)
          cube[idontcares[i]]=2;
        /* now get the next cube */
        Cudd_NextCube(gen,&cube,&value);
        if(Cudd_IsGenEmpty(gen)){
          done_=true;
          return;
        }
       /* check for don't cares */
        ndontcares=0;
        nexpand=1; 
        iterator=0;
        for(size_t i=0;i<nvars_; i++) {
          if (cube[ivars_[i]]==2) {
            idontcares[ndontcares]=ivars_[i];
            ndontcares++;
            nexpand*=2;
          }
        }
        /* if the cube contins don't cares, start expanding the cube to a minterm */
        if(ndontcares) {
          /* start with all zero entries */
          for(size_t i=0; i<ndontcares; i++)
            cube[idontcares[i]]=0;
        }

      }
    }

    /* function: done
     * 1 if the final minterm is reached
     * 0 otherwise
     */
    inline bool done(void) { return done_; }
    /* function: copyMinterm 
     * copy the current minterm to mterm
     * mterm needs to be allocated in calling scope */
    inline void copyMinterm(int *mterm) {
      for(size_t i=0; i<nvars_; i++)
        mterm[i]=cube[ivars_[i]];
    }
    /* function: printMinterm 
     */
    inline void printMinterm() {
      for(size_t i=0; i<nvars_; i++)
        std::cout << cube[ivars_[i]];
      std::cout<< std::endl;
    
    }
    /* function: currentMinter()
     * get an const int* pointer to the current minterm
     * a modification of  the integer array (containing the minterm) is not
     * allowed
     */
    inline const int* currentMinterm() {
      return cube;
    }
    /* function: printProgress
     */
    inline void printProgress(void) { 
      if((size_t)(progress/nminterm*100)>=counter_){
        if((counter_%10)==0)
          std::cout << counter_ ;
        else
          std::cout << ".";
        counter_++;
      }
      std::flush(std::cout); 
    }
};


#endif /* CUDDMINTERMITERATOR_HH_ */
