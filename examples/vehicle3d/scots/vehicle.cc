/*
 * vehicle.cc
 *
 *  created on: 30.09.2015
 *      author: rungger
 */

/*
 * information about this example is given in the readme file
 *
 */

#include <array>
#include <iostream>

#include "cuddObj.hh"

#include "SymbolicSet.hh"
#include "SymbolicModelGrowthBound.hh"

#include "TicToc.hh"
#include "RungeKutta4.hh"
#include "FixedPoint.hh"

#ifndef M_PI
#define M_PI 3.14159265359
#endif


/* state space dim */
#define sDIM 3
#define iDIM 2

/* data types for the ode solver */
typedef std::array<double,3> state_type;
typedef std::array<double,2> input_type;

/* sampling time */
const double tau = 0.3;
/* number of intermediate steps in the ode solver */
const int nint=5;
OdeSolver ode_solver(sDIM,nint,tau);

/* we integrate the vehicle ode by 0.3 sec (the result is stored in x)  */
auto  vehicle_post = [](state_type &x, input_type &u) -> void {

  /* the ode describing the vehicle */
  auto rhs =[](state_type& xx,  const state_type &x, input_type &u) {
      double alpha=std::atan(std::tan(u[1])/2.0);
      xx[0] = u[0]*std::cos(alpha+x[2])/std::cos(alpha);
      xx[1] = u[0]*std::sin(alpha+x[2])/std::cos(alpha);
      xx[2] = u[0]*std::tan(u[1]);
  };
  ode_solver(rhs,x,u);
};

/* computation of the growth bound (the result is stored in r)  */
auto radius_post = [](state_type &r, input_type &u) {
    double c = std::abs(u[0]*std::sqrt(std::tan(u[1])*std::tan(u[1])/4.0+1));
    r[0] = r[0]+c*r[2]*0.3;
    r[1] = r[1]+c*r[2]*0.3;
};

/* forward declaration of the functions to setup the state space
 * input space and obstacles of the vehicle example */
scots::SymbolicSet vehicleCreateStateSpace(Cudd &mgr);
scots::SymbolicSet vehicleCreateInputSpace(Cudd &mgr);

void vehicleCreateObstacles(scots::SymbolicSet &obs);


scots::SymbolicSet vehicleCreateStateSpace(Cudd &mgr) {

  /* setup the workspace of the synthesis problem and the uniform grid */
  /* lower bounds of the hyper rectangle */
  double lb[sDIM]={0,0,-M_PI-0.4};  
  /* upper bounds of the hyper rectangle */
  double ub[sDIM]={5,5,M_PI+0.4}; 
  /* grid node distance diameter */
  double eta[sDIM]={.2,.2,.2};   


  scots::SymbolicSet ss(mgr,sDIM,lb,ub,eta);

  /* add the grid points to the SymbolicSet ss */
  ss.addGridPoints();

 return ss;
}

void vehicleCreateObstacles(scots::SymbolicSet &obs) {

  /* add the obstacles to the symbolic set */
  /* the obstacles are defined as polytopes */
  /* define H* x <= h */
  double H[4*sDIM]={-1, 0, 0,
                    1, 0, 0,
                    0,-1, 0,
                    0, 1, 0};
  /* add outer approximation of P={ x | H x<= h1 } form state space */
  double h1[4] = {-1.5,5.0,-1.3, 3.7};
  obs.addPolytope(4,H,h1, scots::OUTER);

}

scots::SymbolicSet vehicleCreateInputSpace(Cudd &mgr) {

  /* lower bounds of the hyper rectangle */
  double lb[sDIM]={-2,-1};  
  /* upper bounds of the hyper rectangle */
  double ub[sDIM]={2,1}; 
  /* grid node distance diameter */
  double eta[sDIM]={.4,.2};   

  scots::SymbolicSet is(mgr,iDIM,lb,ub,eta);
  is.addGridPoints();

  return is;
}


/* Persistance:
 *
 * mu X. nu Y. ( pre(Y) & T & (!A)) | pre(X)
 *
 */
BDD persist(const Cudd& mgr, scots::FixedPoint& fp, const BDD& U, const BDD& T, const BDD& A) {
  size_t i,j;
  
  /* outer fp*/
  BDD X=mgr.bddOne();
  BDD XX=mgr.bddZero();

  /* inner fp*/
  BDD Y=mgr.bddZero();
  BDD YY=mgr.bddOne();

  /* the controller */
  BDD C=mgr.bddZero();

  /* as long as not converged */
  for(i=1; XX != X; i++) {
    X=XX;
    BDD preX=fp.pre(X);
    /* init inner fp */
    YY = mgr.bddOne();
    for(j=1; YY != Y; j++) {
      Y=YY;
      YY= ( fp.pre(Y) & (T & (!A)) ) | preX;
    }
    XX=YY;
    //std::cout << "Iterations inner: " << j << std::endl;
    std::cout << "." << std::flush;
    /* remove all (state/input) pairs that have been added
     * to the controller already in the previous iteration * */
    BDD N = XX & (!(C.ExistAbstract(U)));
    /* add the remaining pairs to the controller */
    C=C | N;
    //std::cout << C.CountMinterm(17) << std::endl;
  }
  //std::cout << "Iterations outer: " << i << std::endl;
  std::cout << std::endl;
  return C;
}

/* Recurrance:
 *
 * nu X. mu Y. ( pre(Y) & T & (!A)) | pre(X)
 *
 */
BDD recurr(const Cudd& mgr, scots::FixedPoint& fp, const BDD& U, const BDD& T, const BDD& A) {
  size_t i,j;
  
  /* outer fp*/
  BDD X=mgr.bddZero();
  BDD XX=mgr.bddOne();

  /* inner fp*/
  BDD Y=mgr.bddOne();
  BDD YY=mgr.bddZero();

  /* the controller */
  BDD C=mgr.bddZero();

  /* as long as not converged */
  for(i=1; XX != X; i++) {
    X=XX;
    BDD preX=fp.pre(X);
    /* init inner fp */
    YY = mgr.bddZero();
    for(j=1; YY != Y; j++) {
      Y=YY;
      YY= ( fp.pre(Y) & (T & (!A)) ) | preX;
    }
    XX=YY;
    //std::cout << "Iterations inner: " << j << std::endl;
    std::cout << "." << std::flush;
    /* remove all (state/input) pairs that have been added
     * to the controller already in the previous iteration * */
    BDD N = XX & (!(C.ExistAbstract(U)));
    /* add the remaining pairs to the controller */
    C=C | N;
    //std::cout << C.CountMinterm(17) << std::endl;
  }
  //std::cout << "Iterations outer: " << i << std::endl;
  std::cout << std::endl;
  return C;
}



int main() {
  /* to measure time */
  TicToc tt;
  /* there is one unique manager to organize the bdd variables */
  Cudd mgr;

  /****************************************************************************/
  /* construct SymbolicSet for the state space */
  /****************************************************************************/
  scots::SymbolicSet ss=vehicleCreateStateSpace(mgr);
  ss.writeToFile("vehicle_ss.bdd");

  /****************************************************************************/
  /* construct SymbolicSet for the obstacles */
  /****************************************************************************/
  /* first make a copy of the state space so that we obtain the grid
   * information in the new symbolic set */
  scots::SymbolicSet obs(ss);
  vehicleCreateObstacles(obs);
  obs.writeToFile("vehicle_obst.bdd");

  /****************************************************************************/
  /* we define the target set */
  /****************************************************************************/
  /* first make a copy of the state space so that we obtain the grid
   * information in the new symbolic set */
  scots::SymbolicSet target1(ss);
  /* define the target set as a symbolic set */
  double H1[4*sDIM]={-1, 0, 0,
                    1, 0, 0,
                    0,-1, 0,
                    0, 1, 0};
  /* compute inner approximation of P={ x | H x<= h1 }  */
  double h1[4] = {-3.7,6.0,-0, 1.3};
  target1.addPolytope(4,H1,h1, scots::INNER);
  target1.writeToFile("vehicle_target1.bdd");

  scots::SymbolicSet target2(ss);
  /* define the target set as a symbolic set */
  double H2[4*sDIM]={-1, 0, 0,
                    1, 0, 0,
                    0,-1, 0,
                    0, 1, 0};
  /* compute inner approximation of P={ x | H x<= h1 }  */
  double h2[4] = {-3.7,6.0,-3.7, 5.0};
  target2.addPolytope(4,H2,h2, scots::INNER);
  target2.writeToFile("vehicle_target2.bdd");  

  /****************************************************************************/
  /* construct SymbolicSet for the input space */
  /****************************************************************************/
  scots::SymbolicSet is=vehicleCreateInputSpace(mgr);

  /****************************************************************************/
  /* setup class for symbolic model computation */
  /****************************************************************************/
  /* first create SymbolicSet of post variables 
   * by copying the SymbolicSet of the state space and assigning new BDD IDs */
  scots::SymbolicSet sspost(ss,1);
  /* instantiate the SymbolicModel */
  scots::SymbolicModelGrowthBound<state_type,input_type> abstraction(&ss, &is, &sspost);
  /* compute the transition relation */
  tt.tic();
  std::cout << "Symbolic Model: " << std::flush;
  abstraction.computeTransitionRelation(vehicle_post, radius_post);
  std::cout << std::endl;
  std::cout << "Symbolic Model constructed and the ";
  tt.toc();
  /* get the number of elements in the transition relation */
  std::cout << std::endl << "Number of elements in the transition relation: " << abstraction.getSize() << std::endl;

  /****************************************************************************/
  /* we continue with the controller synthesis */
  /****************************************************************************/
  /* we setup a fixed point object to compute reachabilty controller */
  scots::FixedPoint fp(&abstraction);
  /* the fixed point algorithm operates on the BDD directly */
  BDD T1 = target1.getSymbolicSet();
  BDD T2 = target2.getSymbolicSet();
  BDD O = obs.getSymbolicSet();
  tt.tic();
  std::cout << "Controller Synthesis C1: " << std::flush;
  BDD C1 = persist(mgr, fp, is.getCube(), T1, O);
  std::cout << "Controller Synthesis C2: " << std::flush;
  BDD C2 = persist(mgr, fp, is.getCube(), T2, O);
  std::cout << "Controllers synthesized and the ";
  tt.toc();

  /****************************************************************************/
  /* last we store the controller as a SymbolicSet 
   * the underlying uniform grid is given by the Cartesian product of 
   * the uniform gird of the space and uniform gird of the input space */
  /****************************************************************************/
  scots::SymbolicSet controller1(ss,is);
  controller1.setSymbolicSet(C1);
  controller1.writeToFile("vehicle_controller1.bdd");
  scots::SymbolicSet controller2(ss,is);
  controller2.setSymbolicSet(C2);
  controller2.writeToFile("vehicle_controller1.bdd");  

  return 1;
}

