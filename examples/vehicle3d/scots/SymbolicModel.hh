/*
 * SymbolicModel.hh
 *
 *  created on: 09.10.2015
 *      author: rungger
 */

#ifndef SYMBOLICMODEL_HH_
#define SYMBOLICMODEL_HH_

#include <iostream>
#include <stdexcept>

#include "cuddObj.hh"
#include "dddmp.h"

#include "SymbolicSet.hh"


namespace scots {
/*
 * class: SymbolicModel
 *
 * base class for <SymbolicModelGrowthBound>
 * 
 * it stores the bdd of the transition relation and its bdd variable information
 * it provides an iterator to loop over all elements in the stateSpace and inputSpace
 *
 * the actual computation of the transition relation is computs in the derived
 * classes
 */
class SymbolicModel {
protected:
  /* var: ddmgr_ */
  Cudd *ddmgr_;
  /* var: stateSpace_ */
  SymbolicSet *stateSpace_;
  /* var: inputSpace_ */
  SymbolicSet *inputSpace_;
  /* var: stateSpacePost_ */
  SymbolicSet *stateSpacePost_;
  /* var: nssVars_ 
   * number of state space bdd variables (= number of pre/post variables)  */
  size_t nssVars_;
  /* var: preVars_ 
   * array of indices of the state space pre bdd variables  */
  size_t* preVars_;
  /* var: postVars_ 
   * array of indices of the state space post bdd variables  */
  size_t* postVars_;
  /* var: nisVars_ 
   * number of input space bdd variables */
  size_t nisVars_;
  /* var: inpVars_ 
   * array of indices of the input space bdd variables  */
  size_t* inpVars_;
  /* var: iterator_ 
   * CuddMintermIterator to iterate over all elements in stateSpace_ and
   * inputSpace_ */
  CuddMintermIterator *iterator_;
  BDD it;
  /* var: transitionRelation_ 
   * the bdd representation of the transition relation  X x U x X
   * the bdd variables of the transition relation are 
   * given by preVars_ x inpVars_ x postVars_ */
  BDD transitionRelation_;
  friend class FixedPoint;
public:
  /* constructor: SymbolicModel 
   *
   * Representation of the transition relation as BDD in the space 
   *
   *   preX  x U x postX
   *  
   * provide SymbolicSet for preX 
   * provide SymbolicSet for U
   * provide SymbolicSet for postX 
   *
   * the SymbolicSet for preX and postX need to be identical, except the
   * BDD variable IDs need to differ
   *
   * 
   */
  SymbolicModel(SymbolicSet *stateSpace, 
                SymbolicSet *inputSpace, 
                SymbolicSet *stateSpacePost): 
    stateSpace_(stateSpace), inputSpace_(inputSpace), stateSpacePost_(stateSpacePost) {
    if(stateSpace_->ddmgr_!=inputSpace_->ddmgr_ || stateSpacePost_->ddmgr_!=inputSpace_->ddmgr_ ) {
      std::ostringstream os;
      os << "Error: scots::SymbolicModel: stateSpace and inputSpace need to have the same dd manager.";
      throw std::invalid_argument(os.str().c_str());
    }
    /* check if stateSpace and stateSpacePost have the same domain and grid parameter */
    int differ = 0;
    if(stateSpace_->dim_!=stateSpacePost_->dim_)
      differ=1;
    for(size_t i=0; i<stateSpace_->dim_; i++) {
      if(stateSpace_->firstGridPoint_[i]!= stateSpacePost_->firstGridPoint_[i])
        differ=1;
      if(stateSpace_->nofGridPoints_[i]!= stateSpacePost_->nofGridPoints_[i])
        differ=1;
      if(stateSpace_->eta_[i]!= stateSpacePost_->eta_[i])
        differ=1;
    }
    if(differ) {
      std::ostringstream os;
      os << "Error: scots::SymbolicModel: stateSpace and stateSpacePost need have the same domain and grid parameter.";
      throw std::invalid_argument(os.str().c_str());
    }
    /* check if stateSpace and stateSpacePost have different BDD IDs */
    for(size_t i=0; i<stateSpace_->dim_; i++) 
      for(size_t j=0; j<stateSpace_->nofBddVars_[i]; j++) 
        if(stateSpace_->indBddVars_[i][j]==stateSpacePost_->indBddVars_[i][j])
          differ=1;
    if(differ) {
      std::ostringstream os;
      os << "Error: scots::SymbolicModel: stateSpace and stateSpacePost are not allowed to have the same BDD IDs.";
      throw std::invalid_argument(os.str().c_str());
    }
    ddmgr_=stateSpace_->ddmgr_;
    nssVars_=0;
    for(size_t i=0; i<stateSpace_->dim_; i++) 
      for(size_t j=0; j<stateSpace_->nofBddVars_[i]; j++) 
       nssVars_++; 
    nisVars_=0;
    for(size_t i=0; i<inputSpace_->dim_; i++) 
      for(size_t j=0; j<inputSpace_->nofBddVars_[i]; j++) 
       nisVars_++; 
    /* set the preVars_ to the variable indices of stateSpace_ 
     * set the postVars_ to the variable indices of stateSpacePost_ */
    preVars_ = new size_t[nssVars_];
    postVars_ = new size_t[nssVars_];
    for(size_t k=0, i=0; i<stateSpace_->dim_; i++) {
      for(size_t j=0; j<stateSpace_->nofBddVars_[i]; k++, j++) {
       preVars_[k]=stateSpace_->indBddVars_[i][j];
       postVars_[k]=stateSpacePost_->indBddVars_[i][j];
      }
    }
    inpVars_ = new size_t[nisVars_];
    for(size_t k=0, i=0; i<inputSpace_->dim_; i++) {
      for(size_t j=0; j<inputSpace_->nofBddVars_[i]; k++, j++) {
       inpVars_[k]=inputSpace_->indBddVars_[i][j];
      }
    }
    /* initialize the transition relation */
    transitionRelation_=stateSpace_->symbolicSet_*inputSpace_->symbolicSet_;
    it=transitionRelation_;
  };

  ~SymbolicModel(void) {
    delete[] preVars_;
    delete[] postVars_;
    delete[] inpVars_;
  };
  /* function:  getSize 
   * get the number of elements in the transition relation */
  inline double getSize(void) {
    return transitionRelation_.CountMinterm(2*nssVars_+nisVars_);
  };
  /* function:  getTransitionRelation 
   * get the SymbolicSet which represents transition relation in X x U x X */
  inline SymbolicSet getTransitionRelation(void) const {
    /* create SymbolicSet representing the post stateSpace_*/
    /* make a cope of the SymbolicSet of the preSet */
    SymbolicSet post(*stateSpace_);
    /* set the BDD indices to the BDD indices used for the post stateSpace_ */
    post.setIndBddVars(postVars_);
    /* create SymbolicSet representing the stateSpace_ x inputSpace_ */
    SymbolicSet sis(*stateSpace_,*inputSpace_);
    /* create SymbolicSet representing the stateSpace_ x inputSpace_ x stateSpace_ */
    SymbolicSet tr(sis,post);
    /* fill SymbolicSet with transition relation */
    tr.setSymbolicSet(transitionRelation_);
    return tr;
  }; 
  /* function:  setTransitionRelation 
   * set the transitionRelation_ BDD to transitionRelation */
  void setTransitionRelation(BDD transitionRelation) {
    transitionRelation_=transitionRelation;
  };  /* iterator methods */
  /* function: begin
   * initilize the iterator and compute the first element */
  inline void begin(void) {
    size_t nvars_=nssVars_+nisVars_;
    std::vector<size_t> ivars_;
    ivars_.reserve(nvars_);
    for(size_t i=0; i<nssVars_; i++) 
      ivars_.push_back(preVars_[i]);
    for(size_t i=0; i<nisVars_; i++) 
      ivars_.push_back(inpVars_[i]);
    /* set up iterator */
    iterator_ = new CuddMintermIterator(it,ivars_,nvars_);
  }
  /* function: next
   * compute the next minterm */
  inline void next(void) {
    ++(*iterator_);
  }
  /* function: progress
   * print progess of iteratin in percent */
  inline void progress(void) const {
    iterator_->printProgress();
  }  /* function: done
   * changes to one if iterator reached the last element */
  inline int done(void) {
    if(iterator_->done()) {
      delete iterator_;
      iterator_=NULL;
      return 1;
    } else 
    return 0;
  }
  /* function: currentMinterm
   * returns the pointer to the current minterm in the iteration */
  inline const int* currentMinterm(void) const {
    if (iterator_)
      return iterator_->currentMinterm();
    else
      return NULL;
  }
}; /* close class def */
} /* close namespace */

#endif /* SYMBOLICMODEL_HH_ */
