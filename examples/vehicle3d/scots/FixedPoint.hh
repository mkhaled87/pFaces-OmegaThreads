/*
 * FixedPoint.hh
 *
 *  created on: 09.10.2015
 *      author: rungger
 */

#ifndef FIXEDPOINT_HH_
#define FIXEDPOINT_HH_

#include <iostream>
#include <stdexcept>

#include "cuddObj.hh"
#include "SymbolicModel.hh"


namespace scots {
/*
 * class: FixedPoint
 *
 * 
 * it provides  some minimal and maximal fixed point computations to 
 * synthesize controllers for simple invarinace and reachability 
 * spezifications
 *
 */
class FixedPoint {
protected:
  /* var: ddmgr_ */
  Cudd *ddmgr_;
  /* var: symbolicModel_ 
   * stores the transition relation */
  SymbolicModel *symbolicModel_;
  /* var: permute 
   * stores the permutation array used to swap pre with post variables */
  int* permute_;

  /* helper BDDs */
  /* transition relation */
  BDD R_;  
  /* transition relation with cubePost_ abstracted */
  BDD RR_;  

   /* cubes with input and post variables; used in the existential abstraction  */
  BDD cubePost_;
  BDD cubeInput_;
  
public:

  /* function: FixedPoint 
   *
   * initialize the FixedPoint object with a <SymbolicModel> containing the
   * transition relation
   */
  FixedPoint(SymbolicModel *symbolicModel) {
     symbolicModel_=symbolicModel;
    ddmgr_=symbolicModel_->ddmgr_;
     /* the permutation array */
    size_t n=ddmgr_->ReadSize();
    permute_ = new int[n];
    for(size_t i=0; i<n; i++)
      permute_[i]=i;
    for(size_t i=0; i<symbolicModel_->nssVars_; i++)
      permute_[symbolicModel_->preVars_[i]]=symbolicModel_->postVars_[i];
    /* create a cube with the input Vars */
    BDD* vars = new BDD[symbolicModel_->nisVars_];
    for (size_t i=0; i<symbolicModel_->nisVars_; i++)
      vars[i]=ddmgr_->bddVar(symbolicModel_->inpVars_[i]);
    cubeInput_ = ddmgr_->bddComputeCube(vars,NULL,symbolicModel_->nisVars_);
    delete[] vars;
    /* create a cube with the post Vars */
    vars = new BDD[symbolicModel_->nssVars_];
    for (size_t i=0; i<symbolicModel_->nssVars_; i++)
      vars[i]=ddmgr_->bddVar(symbolicModel_->postVars_[i]);   
    cubePost_ = ddmgr_->bddComputeCube(vars,NULL,symbolicModel_->nssVars_);
    delete[] vars;

    /* copy the transition relation */
    R_=symbolicModel_->transitionRelation_;
    RR_=R_.ExistAbstract(cubePost_);
  }
  ~FixedPoint() {
    delete[] permute_;
  }

  /* function: pre 
   *
   * computes the enforcable predecessor 
   *  
   * pre(Z) = { (x,u) | exists x': (x,u,x') in transitionRelation 
   *                    and (x,u,x') in transitionRelation  => x' in Z } 
   *
   */
  BDD pre(BDD Z)  {
    /* project onto state alphabet */
    Z=Z.ExistAbstract(cubePost_*cubeInput_);
    /* swap variables */
    Z=Z.Permute(permute_);
    /* find the (state, inputs) pairs with a post outside the safe set */
    BDD nZ = !Z;
    BDD F = R_.AndAbstract(nZ,cubePost_); 
    /* the remaining (state, input) pairs make up the pre */
    BDD nF = !F;
    BDD preZ= RR_.AndAbstract(nF,cubePost_);
    return preZ;
  }

  /* function: safe 
   *  
   * computation of the maximal fixed point mu Z.pre(Z) & S
   *
   */
  BDD safe(BDD S, int verbose=0)  {
    /* check if safe is a subset of the state space */
    std::vector<unsigned int> sup = S.SupportIndices();
    for(size_t i=0; i<sup.size();i++) {
      int marker=0;
      for(size_t j=0; j<symbolicModel_->nssVars_; j++) {
        if (sup[i]==symbolicModel_->preVars_[j])
          marker=1;
      }
      if(!marker) {
          std::ostringstream os;
          os << "Error: safe: the inital set depends on variables  outside of the state space.";
          throw std::invalid_argument(os.str().c_str());
      }
    }
    if(verbose) 
      std::cout << "Iterations: ";

    BDD Z = ddmgr_->bddZero();
    BDD ZZ = ddmgr_->bddOne();
    /* as long as not converged */
    size_t i;
    for(i=1; ZZ != Z; i++ ) {
      Z=ZZ;
      ZZ=FixedPoint::pre(Z) & S;

      /* print progress */
      if(verbose) {
        std::cout << ".";
        std::flush(std::cout);
        if(!(i%80))
          std::cout << std::endl;
      }
    }
    if(verbose) 
      std::cout << " number: " << i << std::endl;
    return Z;
  } 
  
  /* function: reach 
   *  
   * computation of the minimal fixed point mu Z.pre(Z) | T
   *
   */
  BDD reach(const BDD &T, int verbose=0)  {
    /* check if target is a subset of the state space */
    std::vector<unsigned int> sup = T.SupportIndices();
    for(size_t i=0; i<sup.size();i++) {
      int marker=0;
      for(size_t j=0; j<symbolicModel_->nssVars_; j++) {
        if (sup[i]==symbolicModel_->preVars_[j])
          marker=1;
      }
      if(!marker) {
        std::ostringstream os;
        os << "Error: reach: the target set depends on variables outside of the state space.";
        throw std::invalid_argument(os.str().c_str());
      }
    }
    if(verbose) 
      std::cout << "Iterations: ";

    BDD Z = ddmgr_->bddOne();
    BDD ZZ = ddmgr_->bddZero();
    /* the controller */
    BDD C = ddmgr_->bddZero();
    /* as long as not converged */
    size_t i;
    for(i=1; ZZ != Z; i++ ) {
      Z=ZZ;
      ZZ=FixedPoint::pre(Z) | T;
      /* new (state/input) pairs */
      BDD N = ZZ & (!(C.ExistAbstract(cubeInput_)));
      /* add new (state/input) pairs to the controller */
      C=C | N;
      /* print progress */
      if(verbose) {
        std::cout << ".";
        std::flush(std::cout);
        if(!(i%80))
          std::cout << std::endl;
      }
    }
    if(verbose) 
      std::cout << " number: " << i << std::endl;
    return C;
  }

  /* function: reachAvoid 
   *  
   * controller synthesis to enforce reach avoid specification
   *
   * computation of the minimal fixed point mu Z.(pre(Z) | T) & !A
   *
   */
  BDD reachAvoid(const BDD &T, const BDD &A, int verbose=0)  {
    /* check if target is a subset of the state space */
    std::vector<unsigned int> sup = T.SupportIndices();
    for(size_t i=0; i<sup.size();i++) {
      int marker=0;
      for(size_t j=0; j<symbolicModel_->nssVars_; j++) {
        if (sup[i]==symbolicModel_->preVars_[j])
          marker=1;
      }
      if(!marker) {
          std::ostringstream os;
          os << "Error: reachAvoid: the inital set depends on variables  outside of the state space.";
          throw std::invalid_argument(os.str().c_str());
      }
    }
    if(verbose) 
      std::cout << "Iterations: ";

    BDD RR=RR_;
    /* remove avoid (state/input) pairs from transition relation */
    RR_= RR & (!A);
    BDD TT= T & (!A);

    BDD Z = ddmgr_->bddOne();
    BDD ZZ = ddmgr_->bddZero();
    /* the controller */
    BDD C = ddmgr_->bddZero();
    /* as long as not converged */
    size_t i;
    for(i=1; ZZ != Z; i++ ) {
      Z=ZZ;
      ZZ=FixedPoint::pre(Z) | TT;
      /* new (state/input) pairs */
      BDD N = ZZ & (!(C.ExistAbstract(cubeInput_)));
      /* add new (state/input) pairs to the controller */
      C=C | N;
      /* print progress */
      if(verbose) {
        std::cout << ".";
        std::flush(std::cout);
        if(!(i%80))
          std::cout << std::endl;
      }
    }
    if(verbose) 
      std::cout << " number: " << i << std::endl;
    /* restor transition relation */
    RR_=RR;
    return C;
  }
}; /* close class def */
} /* close namespace */

#endif /* FIXEDPOINT_HH_ */
