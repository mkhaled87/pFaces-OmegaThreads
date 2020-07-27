/*
 * SymbolicSet.hh
 *
 *  	 created: Jul 2018
 *   org. author: Matthias Rungger
 *	  adapted by: Mahmoud Khaled
 */

#ifndef CUDD_SYMBOLICSET_HH_
#define CUDD_SYMBOLICSET_HH_


#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "cuddObj.hh"
#include "dddmp.h"


/* class: cuddMintermIterator
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
class cuddMintermIterator {
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
    /* constructor: cuddMintermIterator
     * iterate over minterms containing the variables defined by the indices
     * ivars
     *
     * the variables ivars must conatin all support variables of the bdd  */
    cuddMintermIterator(BDD bdd, std::vector<size_t> &ivars, size_t nvars);
	
    /* constructor: cuddMintermIterator
     * iterate over minterms containing the support variables */ 
    cuddMintermIterator(BDD bdd);
	
    ~cuddMintermIterator();
	
    void operator++(void);

    /* function: done
     * 1 if the final minterm is reached
     * 0 otherwise
     */
    bool done(void);
	
    /* function: copyMinterm 
     * copy the current minterm to mterm
     * mterm needs to be allocated in calling scope */
    void copyMinterm(int *mterm);
	
    /* function: printMinterm 
     */
    void printMinterm();
	
    /* function: currentMinter()
     * get an const int* pointer to the current minterm
     * a modification of  the integer array (containing the minterm) is not
     * allowed
     */
    const int* currentMinterm();
	
    /* function: printProgress
     */
    void printProgress(void);
};


/* class: SymbolicSet
 *
 * symbolic (bdd) implementation of a set of points aligned on a uniform grid
 *
 *
 * Properties: 
 * - the grid points are distributied uniformly in each dimension 
 * - the domain of the unfiorm grid is defined by a hyper interval
 * - each grid pont is associated with a cell, i.e. a hyper rectangle with
 *   radius eta/2+z centered at the grid point 
 * 
 * Grid point alignment:
 * - the origin is a grid point (not necessarily contained in the set)
 * - the distance of the grid points in each dimension i is defined by eta[i]
 *
 * See 
 * - http://arxiv.org/abs/1503.03715 for theoretical background
 *
 */
class cuddSymbolicSet {
private:
  /* var: ddmgr_  
   * the binary decision diagram manager */
	Cudd *ddmgr_;
  /* var: dim_ 
   * dimension of the real space */
  size_t dim_;
  /* var: eta_
   * dim_-dimensional vector containing the grid node distances */
  double* eta_;
  /* var: z_
   * dim_-dimensional vector containing the measurement error bound */
  double* z_;
  /* var: firstGridPoint_
   * dim_-dimensinal vector containing the real values of the first grid point */
  double* firstGridPoint_;
  /* var: firstGridPoint_
   * dim_-dimensinal vector containing the real values of the last grid point */
  double* lastGridPoint_;
  /* var: nofGridPoints_
   * integer array[dim_] containing the grid points in each dimension */
  size_t* nofGridPoints_;
  /* read the SymbolicSet information from file*/
  /* var: nofBddVars_ 
   * integer array[dim_] containing the number of bdd variables in each dimension */
  size_t *nofBddVars_;
  /* var: indBddVars_ 
   * 2D integer array[dim_][nofBddVars_] containing the indices (=IDs) of the bdd variables */
  size_t **indBddVars_;
  /* var: nvars_ 
   * total number of bdd variables representing the support of the set */
  size_t nvars_;
  /* var: symbolicSet_ 
   * the bdd representing the set of points */
  BDD symbolicSet_;
  /* var: iterator_ 
   * class to iterate over all elements in the symbolic set*/
  cuddMintermIterator *iterator_;
public:

  enum ApproximationType {INNER, OUTER};

  /* constructor: SymbolicSet 
   * provide uniform grid parameters and domain defining hyper interval
   * 
   * Input:
   * ddmgr - the cudd manager
   * lb    - lower left corner of the domain hyper interval
   * ub    - upper right corner of the domain hyper interval
   * eta   - grid point distances
   */
  cuddSymbolicSet(Cudd &ddmgr, const size_t dim, const double* lb, const double* ub, const double* eta, const double* z = NULL);
  cuddSymbolicSet(Cudd &ddmgr, const size_t dim, const float* lb, const float* ub, const float* eta, const float* z = NULL);
	
  /* constructor:  SymbolicSet
   * read the symbolic set from file:
   * newID=0 - the bdd variable ids (used for the symbolic set) are taken from file 
   * newID=1 - the bdd variable ids (used for the symbolic set) are newly generated */
  cuddSymbolicSet(Cudd &ddmgr, const char *filename, int newID=0);
  
  
  /* constructor: SymbolicSet
   * 
   * copy constructor with optinal newID flag
   *
   * newID=0 - the bdd variable ids (used for the SymbolicSet) are taken from other
   * newID=1 - the bdd variable ids (used for the SymbolicSet) are newly generated 
   */
  cuddSymbolicSet(const cuddSymbolicSet& other, int newID=0);
  
  
  /* constructor: SymbolicSet
   * 
   * copy constructor which returns the projection of the given <SymbolicSet> onto the 
   * dimensions stored in std::vector<size_t> pdim 
   *
   * For example, if pdim={1,2}, then the <SymbolicSet> resulting from the
   * projeciton of the <SymbolicSet> other onto the first two dimension is returned
   *
   */
  cuddSymbolicSet(const cuddSymbolicSet& other, std::vector<size_t> pdim);
  
  
  /* constructor: SymbolicSet
   * 
   * create SymbolicSet from two other SymbolicSets
   * the uniform grid results from the Cartesian product 
   * of the uniform grids of the two input SymbolicSets
   */
  cuddSymbolicSet(const cuddSymbolicSet& set1, const cuddSymbolicSet& set2);
  
  
  ~cuddSymbolicSet();
  
  
  /* function: copy assignment operator */
  cuddSymbolicSet& operator=(const cuddSymbolicSet& other);

  /* function: setFromBits
  * sets the symbolic-set based on a list of bits. One-valued bits represent members in the
  * symbolic set wilhe zero-valued bits are not elements of the set. The argument size is used
  * is used to check against the max numbe rof elements the symbolic set can hold.
  * The data is assumed to consist of chuncks with count=size. the biy to check is located at
  * index bitIndexInchunk in the chunk;
  * */
  void setFromBits(const char* data, size_t numChunks, size_t chunkSize, size_t bitIndexInchunk);
 
  /* function: mintermToElement
   * compute the element of the symbolic set associated with the minterm */
  void mintermToElement(const int* minterm, double* element) const;
   
  /* function:  getDimension 
   *
   * get the dimension of the real space in which the points that are represented by
   * the symbolic set live in */
  size_t getDimension(void) const;
  
  /* function:  getSymbolicSet 
   *
   * get the BDD  that stores the SymbolicSet
   */
  BDD getSymbolicSet(void) const;
  
  /* function:  getCube
   *
   * returns a BDD with ones in the domain of the SymbolicSet
   */
  BDD getCube(void) const;
  
  /* function: setSymbolicSet 
   *
   * set the symbolic set to symset
   */
  void setSymbolicSet(const BDD symset);
  
  /* function:  getnofbddvars 
   * 
   * get the pointer to the size_t array that contains the number of bdd
   * variables 
   */
  size_t* getNofBddVars(void) const;
  
  /* function:  getIndBddVars 
   *
   * get the pointer to the size_t array of indices of the bdd
   * variables 
   */
  size_t** getIndBddVars(void) const;
  
  /* function:  setIndBddVars 
   *
   * set the indices of the bdd variables to the indices provided in 
   * size_t* newIndBddVars
   * NOTE: the size of pointer newIndBddVars needs to be dim_*nofBddVars_!
   */
  void setIndBddVars(size_t* newIndBddVars);
  
  /* function:  getSize
   * get the number of grid points in the symbolic set */
  double getSize(void);
  
  /* function:  getSize
   *
   * project the SymbolicSet on the dimensions specified in projectDimension and then
   * count the number of elements
   */
  double getSize(std::vector<size_t> projectDimension);
  
  /* function: clear
   * empty the symbolic set*/
  void clear();
  
  /* iterator methods */
  /* function: begin
   * initilize the iterator and compute the first element */
  void begin(void);
  
  /* function: next
   * compute the next element */
  void next(void);
  
  /* function: progress
   * print progess of iteratin in percent */
  void progress(void);
  
  /* function: done
   * changes to one if iterator reached the last element */
  int done(void);
  
  /* function: currentMinterm
   * returns the pointer to the current minterm in the iteration */
  const int* currentMinterm(void);
  size_t currentMintermFlatValue(void);
  
  /* function: addGridPoints
   * initially the symbolic set is empty
   *
   * with this command all grid points of the uniform grid are added to the set*/
  void addGridPoints(void);
  
  /* function: addByFunction
   * a grid point x is added to the SymbolicSet if(function(x)) is true
   */
  template<class F>
  void addByFunction(F &function);

  /* function: addPolytope
   *
   * add grid points that are in polytope
   * P = { x | H x <= h }
   *
   * Input:
   * p - number of halfspaces
   * H - is a double vector of size dim*p which contains the normal vectors n1...np of the half
   *     spaces. H = { n1, n2, n3 ... np }
   * h - double vector of size p
   * type - approximation type is either INNER or OUTER:
   *        - INNER -> grid points whose cells are completely contained in P are *added* to the symolic set
   *        - OUTER -> grid points whose cells overlap with P are *added* to the symolic set
   */
  void addPolytope(const size_t p, const double* H, const double* h, const ApproximationType type);
  
  
  /* function: remPolytope
   *
   * remove grid points that are in polytope
   * P = { x | H x <= h }
   *
   * Input:
   * p - number of halfspaces
   * H - is a double vector of size dim*p which contains the normal vectors n1...np of the half
   *     spaces. H = { n1, n2, n3 ... np }
   * h - double vector of size p
   * type - approximation type is either INNER or OUTER:
   *        - INNER -> grid points whose cells are completely contained in P are *removed* from the symolic set
   *        - OUTER -> grid points whose cells overlap with P are *removed* from the symolic set
   */
  void remPolytope(const size_t p, const double* H, const double* h, const ApproximationType type);
  
  /* function: addEllipsoid
   *
   * add grid points that are in ellipsoid
   * E = { x |  (x-c)' L' L (x-c) <= 1 }
   *
   * Input:
   * L - is a double vector of size dim*dim  so that L'*L is positive definite
   * c - double vector of size dim containing the center of the ellispoid
   * type - approximation type is either INNER or OUTER:
   *        - INNER -> grid points whose cells are completely contained in E are *added* to the symolic set
   *        - OUTER -> grid points whose cells overlap with E are *added* to the symolic set
   */
  void addEllipsoid(const double* L, const double* c, const ApproximationType type);
  

  /* function: remEllipsoid
   *
   * remove grid points that are in ellipsoid
   * E = { x |  (x-c)' L' L (x-c) <= 1 }
   *
   * Input:
   * L - is a double vector of size dim*dim  so that L'*L is positive definite
   * c - double vector of size dim containing the center of the ellispoid
   * type - approximation type is either INNER or OUTER:
   *        - INNER -> grid points whose cells are completely contained in E are *removed* from the symolic set
   *        - OUTER -> grid points whose cells overlap with E are *removed* from the symolic set
   */
  void remEllipsoid(const double* L, const double* c, const ApproximationType type);
  
  /* function: printInfo
   * print some numbers related to the symbolic set*/
  void printInfo(int verbosity=0) const;
  
  /* function:  writeToFile
   * store the bdd with all information to file*/
  void writeToFile(const char *filename);
  
  /* function:  getZ
   * return constant pointer to the double array z_*/
  const double* getZ() const;
  
  /* function:  getEta
   * return constant pointer to the double array eta_*/
  const double* getEta() const;
  
  /* function: complement
   * compute the of the symbolic set with respect to the uniform grid*/
  void complement();
  
  /* function:  getFirstGridPoint 
   * the first grid point is saved to a double array of size dim_*/
  const double* getFirstGridPoint() const;
  
  /* function:  getLastGridPoint 
   * the last grid point is saved to a double array of size dim_*/
  const double* getLastGridPoint() const;
  
  /* function:  getNofGridPoints
   * return pointer to size_t array containing the number of grid points*/
  const size_t* getNofGridPoints() const;
  
  /* function:  copyZ
   * z_ is written to the double array z of size dim_*/
  void copyZ(double z[]) const;
  
  /* function:  copyEta
   * eta_ is written to the double array eta of size dim_*/
  void copyEta(double eta[]) const;
  
  /* function:  copyFirstGridPoint 
   * the first grid point is saved to a double array of size dim_*/
  void copyFirstGridPoint(double firstGridPoint[]) const;
  
  /* function:  copyLastGridPoint 
   * the last grid point is saved to a double array of size dim_*/
  void copyLastGridPoint(double lastGridPoint[]) const;
  
  /* function:  copyNofGridPoints
   * copy the size_t array containing the number of grid points to nofGridPoints*/
  void getNofGridPoints(size_t nofGridPoints[]) const;
  
  /* function: copyGridPoints
   * the grid points in the symbolic set are saved to the double array of size (dim_) x (setSize) */
  void copyGridPoints(double gridPoints[]) const;
  void copyGridPointIndicies(size_t gridPoints[], bool sortIt) const;

  /* function: copyGridPoints
   *
   * the SymbolicSet is projected onto the dimensions specified in  projectDimension 
   * 
   * the grid points in the resultilng set are saved to the double array of size (dim_) x (setSize) 
   */
  void copyGridPoints(double gridPoints[], std::vector<size_t> projectDimension, bool sortIt = false) const;
  void copyGridPointIndicies(size_t gridPoints[], std::vector<size_t> projectDimension, bool sortIt = false) const;
  
  /* function: isElement
   *
   * check if an element x is an element of the SymolicSet
   *
   *  x -- x is an element of the domain
   *
   */
  bool isElement(std::vector<double> x);
  
  /* function: setValuedMap
   *
   * we interpret the SymbolicSet as relation R and identify the relation with a
   * set valued map:
   * 
   * ind -- contains the  dimensions that span the domain of the set valued map
   * the ramaining indices of [0 .. dim_-1] make up the codimension; 
   *
   *  x -- x is an element of the domain
   *
   *  if x is not an element the domain of the set valued map, an empty vector is returned
   *
   */
  std::vector<std::vector<double>> setValuedMap(std::vector<double> x, std::vector<size_t> ind);

  /* shifts all bdd Variables t sstart with vddVar with index 0 with no spacing of vars are scattered */
  void permuteVarsToVarZero();
  
private:
  void readMembersFromFile(const char *filename);

  /* compute a bdd containing all the grid points */
  BDD computeGridPoints(void);

  /* compute symbolic representation (=bdd) of a polytope { x | Hx<= h }*/
  BDD computePolytope(const size_t p, const double* H, const double* h, const ApproximationType type);
  
  /* compute symbolic representation (=bdd) of an ellisoid { x | (x-center)' L' L (x-center) <= 1 } */
  BDD computeEllipsoid(const double* L, const double* center, const ApproximationType type);
  
  /* compute smallest radius of ball that contains L*cell */
  double computeEllipsoidRadius(const double* L);
}; 

#endif /* CUDD_SYMBOLICSET_HH_ */