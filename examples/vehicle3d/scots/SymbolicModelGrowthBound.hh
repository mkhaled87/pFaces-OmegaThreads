/*
 * SymbolicModelGrowthBound.hh
 *
 *  created on: 09.10.2015
 *      author: rungger
 */

#ifndef SYMBOLICMODELGROWTHBOUND_HH_
#define SYMBOLICMODELGROWTHBOUND_HH_

#include <iostream>
#include <stdexcept>


#include "TicToc.hh"

#include "cuddObj.hh"
#include "cudd.h"
#include "dddmp.h"


#include "SymbolicSet.hh"
#include "SymbolicModel.hh"

namespace scots {
/*
 * class: SymbolicModelGrowthBound
 *
 * Derived class from <SymbolicModel>
 * 
 * Constructs a symbolic model according to
 * the theory in http://arxiv.org/abs/1503.03715
 *
 */

template<class stateType_, class inputType_>
class SymbolicModelGrowthBound: public SymbolicModel {
public:
  /* constructor: see <SymbolicModel>
   */
  using SymbolicModel::SymbolicModel;
  /* function:  computeTransitionRelation
   *
   * provide the solution of the system at sampling time and
   * provide the solution of the linear system associated with the growth bound 
   * at sampling time 
   *
   * see the example directory for the specific format
   *
   */
  template<class F1, class F2>
  void computeTransitionRelation(F1 &system_post, F2 &radius_post) {

    /* create the BDD's with numbers 0,1,2,.., #gridPoints */
    size_t dim=stateSpace_->getDimension();
    const size_t* nvars= stateSpace_->getNofBddVars();
    BDD **bddVars = new BDD*[dim];
    for(size_t n=0, i=0; i<dim; i++) {
      bddVars[i]= new BDD[nvars[i]];
      for(size_t j=0; j<nvars[i]; j++)  {
        bddVars[i][j]=ddmgr_->bddVar(postVars_[n+j]);
      }
      n+=nvars[i];
    }
    const size_t* ngp= stateSpace_->getNofGridPoints();
    BDD **num = new BDD*[dim];
    for(size_t i=0; i<dim; i++) {
      num[i] = new BDD[ngp[i]];
      int *phase = new int[nvars[i]];
      for(size_t j=0;j<nvars[i];j++)
        phase[j]=0;
      for(size_t j=0;j<ngp[i];j++) {
        int *p=phase;
        int x=j;
        for (; x; x/=2) *(p++)=0+x%2;
        num[i][j]= ddmgr_->bddComputeCube(bddVars[i],(int*)phase,nvars[i]);
      }
      delete[] phase;
      delete[] bddVars[i];
    }
    delete[] bddVars;

    /* bdd nodes in pre and input variables */
    DdManager *mgr = ddmgr_->getManager();
    size_t ndom=nssVars_+nisVars_;
    int*  phase = new int[ndom];
    DdNode**  dvars = new DdNode*[ndom];
    for(size_t i=0;i<nssVars_; i++)
      dvars[i]=Cudd_bddIthVar(mgr,preVars_[i]);
    for(size_t i=0;i<nisVars_; i++)
      dvars[nssVars_+i]=Cudd_bddIthVar(mgr,inpVars_[i]);

    /* initialize cell radius
     * used to compute the growth bound */
    stateType_ eta;
    stateSpace_->copyEta(&eta[0]);
    stateType_ z;
    stateSpace_->copyZ(&z[0]);
    stateType_ r;

    stateType_ first;
    stateSpace_->copyFirstGridPoint(&first[0]);

    transitionRelation_=ddmgr_->bddZero();
    const int* minterm;

    /* compute constraint set against the post is checked */
    size_t n=ddmgr_->ReadSize();
    int* permute = new int[n];
    for(size_t i=0; i<nssVars_; i++)
      permute[preVars_[i]]=postVars_[i];
    BDD ss = stateSpace_->getSymbolicSet();
    BDD constraints=ss.Permute(permute);
    delete[] permute;

    /** big loop over all state elements and input elements **/
    for(begin(); !done(); next()) {
      progress();
      minterm=currentMinterm();

      /* current state */
      stateType_ x;
      stateSpace_->mintermToElement(minterm,&x[0]);
      /* current input */
      inputType_ u;
      inputSpace_->mintermToElement(minterm,&u[0]);
      /* cell radius (including measurement errors) */
      for(size_t i=0; i<dim; i++)
        r[i]=eta[i]/2.0+z[i];

      /* integrate system and radius growth bound */
      /* the result is stored in x and r */
      system_post(x,u);
      radius_post(r,u);

      /* determine the cells which intersect with the attainable set*/
      /* start with the computation of the indices */
      BDD post=ddmgr_->bddOne();
      for(size_t i=0; i<dim; i++) { 
        int lb = std::lround(((x[i]-r[i]-z[i]-first[i])/eta[i]));
        int ub = std::lround(((x[i]+r[i]+z[i]-first[i])/eta[i]));
        if(lb<0 || ub>=(int)ngp[i]) {
          post=ddmgr_->bddZero();
          break;
        }
        BDD zz=ddmgr_->bddZero();
        for(int j=lb; j<=ub; j++) {
          zz|=num[i][j];
        }
        post &= zz;
      }
      if(!(post==ddmgr_->bddZero()) && post<= constraints) {
        /* compute bdd for the current x and u element and add x' */
        for(size_t i=0;i<nssVars_; i++)
          phase[i]=minterm[preVars_[i]];
        for(size_t i=0;i<nisVars_; i++)
          phase[nssVars_+i]=minterm[inpVars_[i]];
        BDD current(*ddmgr_,Cudd_bddComputeCube(mgr,dvars,phase,ndom));
        current&=post;
        transitionRelation_ +=current;
      }

    }

    for(size_t i=0; i<dim; i++) 
      delete[] num[i];
    delete[] num;
    delete[] dvars;
    delete[] phase;
  }
}; /* close class def */
} /* close namespace */

#endif /* SYMBOLICMODELGROWTHBOUND_HH_ */
