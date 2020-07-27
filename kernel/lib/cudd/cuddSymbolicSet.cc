#include <sstream>
#include <numeric> 
#include <cassert> 
#include <bitset> 
#include <stdexcept> 
#include <algorithm>
#include <cmath>

#include "cuddSymbolicset.hh"


// ----------------------------------------------
// class: cuddMintermIterator
// ----------------------------------------------
 /* constructor: CuddMintermIterator
 * iterate over minterms containing the variables defined by the indices
 * ivars
 *
 * the variables ivars must conatin all support variables of the bdd  */
cuddMintermIterator::cuddMintermIterator(BDD bdd, std::vector<size_t> &ivars, size_t nvars) {
	ivars_ = ivars;
	nvars_ = nvars;
	done_ = false;
	counter_ = 0;

	if (nvars_>sizeof(size_t) * 8) {
		std::ostringstream os;
		os << "Error: CuddMintermIterator: number of variables we iterate over is limited to highest number in size_t.";
		throw std::invalid_argument(os.str().c_str());
	}

	/* check if bdd depens only on bdd variables with indices ivars */
	std::vector<unsigned int> sup = bdd.SupportIndices();
	for (size_t i = 0; i<sup.size(); i++) {
		int marker = 0;
		for (size_t j = 0; j<nvars; j++) {
			if (sup[i] == ivars[j])
				marker = 1;
		}
		if (!marker) {
			std::ostringstream os;
			os << "Error: CuddMintermIterator: the bdd depends on variables with index outside ivars.";
			throw std::invalid_argument(os.str().c_str());
		}
	}
	/* initialize gen and grab first cube */
	DdManager *manager = bdd.manager();
	DdNode *node = bdd.getNode();
	gen = Cudd_FirstCube(manager, node, &cube, &value);
	if (Cudd_IsGenEmpty(gen)) {
		done_ = true;
		return;
	}
	/* determine the number of minterms */
	nminterm = bdd.CountMinterm(nvars);
	progress = 1;

	/* check cube for don't cares */
	idontcares.resize(nvars);
	ndontcares = 0;
	nexpand = 1;
	iterator = 0;
	/* indices of the variables with don't cares */
	for (size_t i = 0; i<nvars; i++) {
		if (cube[ivars[i]] == 2) {
			idontcares[ndontcares] = ivars_[i];
			ndontcares++;
			nexpand *= 2;
		}
	}
	/* if the cube contins don't cares, start expanding the cube to a minterm */
	if (ndontcares) {
		/* start with all zero entries */
		for (size_t i = 0; i<ndontcares; i++)
			cube[idontcares[i]] = 0;
	}
}
/* constructor: CuddMintermIterator
* iterate over minterms containing the support variables */
cuddMintermIterator::cuddMintermIterator(BDD bdd) {
	done_ = false;

	std::vector<unsigned int> sup = bdd.SupportIndices();
	ivars_.assign(sup.begin(), sup.end());
	nvars_ = ivars_.size();


	for (size_t i = 0; i<nvars_; i++)
		std::cout << ivars_[i] << " ";
	std::cout << std::endl;

	/* number of variables we iterate over is limited to highest number in size_t  */
	if (nvars_>sizeof(size_t) * 8) {
		std::ostringstream os;
		os << "Error: CuddMintermIterator: number of variables we iterate over is limited to highest number in size_t.";
		throw std::invalid_argument(os.str().c_str());
	}

	/* initialize gen and grab first cube */
	DdManager *manager = bdd.manager();
	DdNode *node = bdd.getNode();
	gen = Cudd_FirstCube(manager, node, &cube, &value);
	if (Cudd_IsGenEmpty(gen)) {
		done_ = true;
		return;
	}
	/* determine the number of minterms */
	nminterm = bdd.CountMinterm(nvars_);
	progress = 1.0;
	/* check cube for don't cares */
	idontcares.resize(nvars_);
	ndontcares = 0;
	nexpand = 1;
	iterator = 0;
	/* indices of the variables with don't cares */
	for (size_t i = 0; i<nvars_; i++) {
		if (cube[ivars_[i]] == 2) {
			idontcares[ndontcares] = ivars_[i];
			ndontcares++;
			nexpand *= 2;
		}
	}
	/* if the cube contins don't cares, start expanding the cube to a minterm */
	if (ndontcares) {
		/* start with all zero entries */
		for (size_t i = 0; i<ndontcares; i++)
			cube[idontcares[i]] = 0;
	}
}
/* destructor: CuddMintermIterator
* */
cuddMintermIterator::~cuddMintermIterator() {
	Cudd_GenFree(gen);
}
void cuddMintermIterator::operator++(void) {

	iterator++;
	progress++;
	/* get new cube or expand cube further */
	if (iterator<nexpand) {
		/* still not don with this cube */
		/* generate binary value for iterator to fill cube */
		std::bitset<sizeof(size_t) * 8> iterator2binary(iterator);
		for (size_t i = 0; i<ndontcares; i++)
			cube[idontcares[i]] = iterator2binary[i];
	}
	else {
		/* restore the cube (put the don't cares) */
		for (size_t i = 0; i<ndontcares; i++)
			cube[idontcares[i]] = 2;
		/* now get the next cube */
		Cudd_NextCube(gen, &cube, &value);
		if (Cudd_IsGenEmpty(gen)) {
			done_ = true;
			return;
		}
		/* check for don't cares */
		ndontcares = 0;
		nexpand = 1;
		iterator = 0;
		for (size_t i = 0; i<nvars_; i++) {
			if (cube[ivars_[i]] == 2) {
				idontcares[ndontcares] = ivars_[i];
				ndontcares++;
				nexpand *= 2;
			}
		}
		/* if the cube contins don't cares, start expanding the cube to a minterm */
		if (ndontcares) {
			/* start with all zero entries */
			for (size_t i = 0; i<ndontcares; i++)
				cube[idontcares[i]] = 0;
		}

	}
}
/* function: done
* 1 if the final minterm is reached
* 0 otherwise
*/
bool cuddMintermIterator::done(void) { return done_; }
/* function: copyMinterm
* copy the current minterm to mterm
* mterm needs to be allocated in calling scope */
void cuddMintermIterator::copyMinterm(int *mterm) {
	for (size_t i = 0; i<nvars_; i++)
		mterm[i] = cube[ivars_[i]];
}
/* function: printMinterm
*/
void cuddMintermIterator::printMinterm() {
	for (size_t i = 0; i<nvars_; i++)
		std::cout << cube[ivars_[i]];
	std::cout << std::endl;

}
/* function: currentMinter()
* get an const int* pointer to the current minterm
* a modification of  the integer array (containing the minterm) is not
* allowed
*/
const int* cuddMintermIterator::currentMinterm() {
	return cube;
}
/* function: printProgress
*/
void cuddMintermIterator::printProgress(void) {
	if ((size_t)(progress / nminterm * 100) >= counter_) {
		if ((counter_ % 10) == 0)
			std::cout << counter_;
		else
			std::cout << ".";
		counter_++;
	}
	std::cout << std::flush;
}



// ----------------------------------------------
// class: SymbolicSet
// ----------------------------------------------
/* constructor: cuddSymbolicSet
* provide uniform grid parameters and domain defining hyper interval
*
* Input:
* ddmgr - the cudd manager
* lb    - lower left corner of the domain hyper interval
* ub    - upper right corner of the domain hyper interval
* eta   - grid point distances
*/
cuddSymbolicSet::cuddSymbolicSet(Cudd &ddmgr, const size_t dim, const double* lb, const double* ub, const double* eta, const double* z) {
	for (size_t i = 0; i<dim; i++) {
		if ((ub[i] - lb[i])<eta[i]) {
			std::ostringstream os;
			os << "Error: cuddSymbolicSet::cuddSymbolicSet:  each interval must contain at least one cell.";
			throw std::invalid_argument(os.str().c_str());
		}
	}
	ddmgr_ = &ddmgr;
	dim_ = dim;
	z_ = new double[dim];
	eta_ = new double[dim];
	lastGridPoint_ = new double[dim];
	firstGridPoint_ = new double[dim];
	nofGridPoints_ = new size_t[dim];
	nofBddVars_ = new size_t[dim];
	indBddVars_ = new size_t*[dim];
	symbolicSet_ = ddmgr.bddZero();
	iterator_ = NULL;

	double Nl, Nu;

	/* determine number of bits in each dimension and initialize the Bdd variables */
	for (size_t i = 0; i<dim; i++) {
		if (z == NULL)
			z_[i] = 0;
		else
			z_[i] = z[i];

		eta_[i] = eta[i];

		Nl = std::ceil(lb[i] / eta[i]);
		Nu = std::floor(ub[i] / eta[i]);
		/* number of grid points */
		nofGridPoints_[i] = (size_t)std::abs(Nu - Nl) + 1;
		/* number of bits */
		if (nofGridPoints_[i] == 1)
			nofBddVars_[i] = 1;
		else
			nofBddVars_[i] = (size_t)std::ceil(std::log2(nofGridPoints_[i]));
		/* determine the indices */
		indBddVars_[i] = new size_t[nofBddVars_[i]];
		for (size_t j = 0; j<nofBddVars_[i]; j++) {
			BDD var = ddmgr.bddVar();
			indBddVars_[i][j] = var.NodeReadIndex();
		}
		/* first and last grid point coordinates */
		lastGridPoint_[i] = Nu*eta[i];
		firstGridPoint_[i] = Nl*eta[i];
	}
	/* number of total variables */
	nvars_ = 0;
	for (size_t i = 0; i<dim_; i++)
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			nvars_++;
}
cuddSymbolicSet::cuddSymbolicSet(Cudd &ddmgr, const size_t dim, const float* lb, const float* ub, const float* eta, const float* z) {
	for (size_t i = 0; i<dim; i++) {
		if ((ub[i] - lb[i])<eta[i]) {
			std::ostringstream os;
			os << "Error: cuddSymbolicSet::cuddSymbolicSet:  each interval must contain at least one cell.";
			throw std::invalid_argument(os.str().c_str());
		}
	}
	ddmgr_ = &ddmgr;
	dim_ = dim;
	z_ = new double[dim];
	eta_ = new double[dim];
	lastGridPoint_ = new double[dim];
	firstGridPoint_ = new double[dim];
	nofGridPoints_ = new size_t[dim];
	nofBddVars_ = new size_t[dim];
	indBddVars_ = new size_t*[dim];
	symbolicSet_ = ddmgr.bddZero();
	iterator_ = NULL;

	double Nl, Nu;

	/* determine number of bits in each dimension and initialize the Bdd variables */
	for (size_t i = 0; i<dim; i++) {
		if (z == NULL)
			z_[i] = 0;
		else
			z_[i] = z[i];

		eta_[i] = eta[i];

		Nl = std::ceil(lb[i] / eta[i]);
		Nu = std::floor(ub[i] / eta[i]);
		/* number of grid points */
		nofGridPoints_[i] = (size_t)std::abs(Nu - Nl) + 1;
		/* number of bits */
		if (nofGridPoints_[i] == 1)
			nofBddVars_[i] = 1;
		else
			nofBddVars_[i] = (size_t)std::ceil(std::log2(nofGridPoints_[i]));
		/* determine the indices */
		indBddVars_[i] = new size_t[nofBddVars_[i]];
		for (size_t j = 0; j<nofBddVars_[i]; j++) {
			BDD var = ddmgr.bddVar();
			indBddVars_[i][j] = var.NodeReadIndex();
		}
		/* first and last grid point coordinates */
		lastGridPoint_[i] = Nu*eta[i];
		firstGridPoint_[i] = Nl*eta[i];
	}
	/* number of total variables */
	nvars_ = 0;
	for (size_t i = 0; i<dim_; i++)
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			nvars_++;
}
/* constructor:  SymbolicSet
* read the symbolic set from file:
* newID=0 - the bdd variable ids (used for the symbolic set) are taken from file
* newID=1 - the bdd variable ids (used for the symbolic set) are newly generated */
cuddSymbolicSet::cuddSymbolicSet(Cudd &ddmgr, const char *filename, int newID) {
	ddmgr_ = &ddmgr;
	iterator_ = NULL;
	/* read the SymbolicSet members from file */
	readMembersFromFile(filename);
	int* composeids = NULL;
	Dddmp_VarMatchType match = DDDMP_VAR_MATCHIDS;
	/* do we need to create new variables ? */
	if (newID) {
		/* we have to create new variable id's and load the bdd with those new ids */
		match = DDDMP_VAR_COMPOSEIDS;
		/* allocate memory for comopsids */
		size_t maxoldid = 0;
		for (size_t i = 0; i<dim_; i++)
			for (size_t j = 0; j<nofBddVars_[i]; j++)
				maxoldid = ((maxoldid < indBddVars_[i][j]) ? indBddVars_[i][j] : maxoldid);
		composeids = new int[maxoldid + 1];
		/* match old id's (read from file) with newly created ones */
		for (size_t i = 0; i<dim_; i++) {
			for (size_t j = 0; j<nofBddVars_[i]; j++) {
				BDD bdd = ddmgr.bddVar();
				composeids[indBddVars_[i][j]] = bdd.NodeReadIndex();
				indBddVars_[i][j] = bdd.NodeReadIndex();
			}
		}
		/* number of total variables */
	}
	/* load bdd */
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		std::ostringstream os;
		os << "Error: Unable to open file:" << filename << "'.";
		throw std::runtime_error(os.str().c_str());
	}
	DdNode *bdd = Dddmp_cuddBddLoad(ddmgr.getManager(), \
		match, \
		NULL, \
		NULL, \
		composeids, \
		DDDMP_MODE_BINARY, \
		NULL, \
		file);
	fclose(file);
	BDD tmp(ddmgr, bdd);
	symbolicSet_ = tmp;

	delete[] composeids;
}
/* constructor: SymbolicSet
*
* copy constructor with optinal newID flag
*
* newID=0 - the bdd variable ids (used for the SymbolicSet) are taken from other
* newID=1 - the bdd variable ids (used for the SymbolicSet) are newly generated
*/
cuddSymbolicSet::cuddSymbolicSet(const cuddSymbolicSet& other, int newID) {
	*this = other;
	this->clear();
	/* create new bdd variable ids */
	if (newID) {
		for (size_t i = 0; i<dim_; i++)
			for (size_t j = 0; j<nofBddVars_[i]; j++) {
				BDD var = ddmgr_->bddVar();
				indBddVars_[i][j] = var.NodeReadIndex();
			}
	}
}
/* constructor: SymbolicSet
*
* copy constructor which returns the projection of the given <SymbolicSet> onto the
* dimensions stored in std::vector<size_t> pdim
*
* For example, if pdim={1,2}, then the <SymbolicSet> resulting from the
* projeciton of the <SymbolicSet> other onto the first two dimension is returned
*
*/
cuddSymbolicSet::cuddSymbolicSet(const cuddSymbolicSet& other, std::vector<size_t> pdim) {
	dim_ = pdim.size();
	ddmgr_ = other.ddmgr_;
	if (!dim_) {
		std::ostringstream os;
		os << "Error: SymbolicSet::SymbolicSet(const SymbolicSet& other, std::vector<size_t> pdim): projection indices pdim should be non-empty.";
		throw std::runtime_error(os.str().c_str());
	}
	/* check if values in pdim are in range [1;n] */
	size_t oor = 0;
	for (auto i : pdim) {
		if ((i == 0) || (i>other.dim_))
			oor = 1;
	}
	if (oor) {
		std::ostringstream os;
		os << "Error: SymbolicSet::SymbolicSet(const SymbolicSet& other, std::vector<size_t> pdim): the elements in pdim need to be within [1...other.dim_].";
		throw std::runtime_error(os.str().c_str());
	}
	z_ = new double[dim_];
	eta_ = new double[dim_];
	lastGridPoint_ = new double[dim_];
	firstGridPoint_ = new double[dim_];
	nofGridPoints_ = new size_t[dim_];
	nofBddVars_ = new size_t[dim_];
	indBddVars_ = new size_t*[dim_];
	symbolicSet_ = ddmgr_->bddZero();
	iterator_ = NULL;
	for (size_t i = 0; i<dim_; i++) {
		z_[i] = other.z_[pdim[i] - 1];
		eta_[i] = other.eta_[pdim[i] - 1];
		lastGridPoint_[i] = other.lastGridPoint_[pdim[i] - 1];
		firstGridPoint_[i] = other.firstGridPoint_[pdim[i] - 1];
		nofGridPoints_[i] = other.nofGridPoints_[pdim[i] - 1];
		nofBddVars_[i] = other.nofBddVars_[pdim[i] - 1];
	}
	for (size_t i = 0; i<dim_; i++) {
		indBddVars_[i] = new size_t[other.nofBddVars_[pdim[i] - 1]];
		for (size_t j = 0; j<other.nofBddVars_[pdim[i] - 1]; j++)
			indBddVars_[i][j] = other.indBddVars_[pdim[i] - 1][j];
	}
	/* number of total variables */
	nvars_ = 0;
	for (size_t i = 0; i<dim_; i++)
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			nvars_++;

}
/* constructor: SymbolicSet
*
* create SymbolicSet from two other SymbolicSets
* the uniform grid results from the Cartesian product
* of the uniform grids of the two input SymbolicSets
*/
cuddSymbolicSet::cuddSymbolicSet(const cuddSymbolicSet& set1, const cuddSymbolicSet& set2) {
	if (set1.ddmgr_ == set2.ddmgr_)
		ddmgr_ = set1.ddmgr_;
	else {
		std::ostringstream os;
		os << "Error: SymbolicSet::SymbolicSet(const SymbolicSet& set1, const SymbolicSet& set2): set1 and set2 do not have the same dd manager.";
		throw std::runtime_error(os.str().c_str());
	}
	dim_ = set1.dim_ + set2.dim_;
	z_ = new double[dim_];
	eta_ = new double[dim_];
	lastGridPoint_ = new double[dim_];
	firstGridPoint_ = new double[dim_];
	nofGridPoints_ = new size_t[dim_];
	nofBddVars_ = new size_t[dim_];
	indBddVars_ = new size_t*[dim_];
	symbolicSet_ = ddmgr_->bddZero();
	iterator_ = NULL;
	for (size_t i = 0; i<set1.dim_; i++) {
		z_[i] = set1.z_[i];
		eta_[i] = set1.eta_[i];
		lastGridPoint_[i] = set1.lastGridPoint_[i];
		firstGridPoint_[i] = set1.firstGridPoint_[i];
		nofGridPoints_[i] = set1.nofGridPoints_[i];
		nofBddVars_[i] = set1.nofBddVars_[i];
	}
	for (size_t i = 0; i<set2.dim_; i++) {
		z_[set1.dim_ + i] = set1.z_[i];
		eta_[set1.dim_ + i] = set2.eta_[i];
		lastGridPoint_[set1.dim_ + i] = set2.lastGridPoint_[i];
		firstGridPoint_[set1.dim_ + i] = set2.firstGridPoint_[i];
		nofGridPoints_[set1.dim_ + i] = set2.nofGridPoints_[i];
		nofBddVars_[set1.dim_ + i] = set2.nofBddVars_[i];
	}
	for (size_t i = 0; i<set1.dim_; i++) {
		indBddVars_[i] = new size_t[set1.nofBddVars_[i]];
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			indBddVars_[i][j] = set1.indBddVars_[i][j];
	}
	for (size_t i = 0; i<set2.dim_; i++) {
		indBddVars_[set1.dim_ + i] = new size_t[set2.nofBddVars_[i]];
		for (size_t j = 0; j<nofBddVars_[set1.dim_ + i]; j++)
			indBddVars_[set1.dim_ + i][j] = set2.indBddVars_[i][j];
	}
	/* number of total variables */
	nvars_ = 0;
	for (size_t i = 0; i<dim_; i++)
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			nvars_++;
}
/* destructor: ~cuddSymbolicSet
* */
cuddSymbolicSet::~cuddSymbolicSet() {
	delete[] z_;
	delete[] eta_;
	delete[] firstGridPoint_;
	delete[] lastGridPoint_;
	delete[] nofGridPoints_;
	delete[] nofBddVars_;
	for (size_t i = 0; i<dim_; i++)
		delete[] indBddVars_[i];
	delete[] indBddVars_;
}
/* operator =: 
 * copy assignment operator */
cuddSymbolicSet& cuddSymbolicSet::operator=(const cuddSymbolicSet& other) {
	ddmgr_ = other.ddmgr_;
	dim_ = other.dim_;
	nvars_ = other.nvars_;
	symbolicSet_ = other.symbolicSet_;
	iterator_ = NULL;
	z_ = new double[dim_];
	eta_ = new double[dim_];
	lastGridPoint_ = new double[dim_];
	firstGridPoint_ = new double[dim_];
	nofGridPoints_ = new size_t[dim_];
	nofBddVars_ = new size_t[dim_];
	indBddVars_ = new size_t*[dim_];
	for (size_t i = 0; i<dim_; i++) {
		z_[i] = other.z_[i];
		eta_[i] = other.eta_[i];
		lastGridPoint_[i] = other.lastGridPoint_[i];
		firstGridPoint_[i] = other.firstGridPoint_[i];
		nofGridPoints_[i] = other.nofGridPoints_[i];
		nofBddVars_[i] = other.nofBddVars_[i];
	}
	for (size_t i = 0; i<dim_; i++) {
		indBddVars_[i] = new size_t[other.nofBddVars_[i]];
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			indBddVars_[i][j] = other.indBddVars_[i][j];
	}
	return *this;
}
/* function setFromBits
 * sets the symbolic-set based on a list of bits. One-valued bits represent members in the
 * symbolic set wilhe zero-valued bits are not elements of the set. The argument size is used
 * is used to check against the max numbe rof elements the symbolic set can hold.
 * */
void cuddSymbolicSet::setFromBits(const char* data, size_t numChunks, size_t chunkSize, size_t bitIndexInchunk){
	if (std::log2(numChunks) > nvars_)
		throw std::runtime_error("cuddSymbolicSet::setFromBits: Data size does not match the current symbolic-set.");

	if(dim_ != 1)
		throw std::runtime_error("cuddSymbolicSet::setFromBits: This function is only supported for one-dimensional symbolic sets to avoid confsion when mapping indicies to support bdd vars.");


	std::cout << "----------------------------------------------------------" << std::endl;
	std::cout << "cuddSymbolicSet::setFromBits: This function is not fully tested !!!" << std::endl;
	std::cout << "----------------------------------------------------------" << std::endl;

	const size_t nofBddVars = nofBddVars_[0];
	int* phase = new int[nofBddVars];
	BDD* bddVars = new BDD[nofBddVars];

	/* make symbolicSet_ empty */
	symbolicSet_ = ddmgr_->bddZero();

	const char* pInteresstingByteInDataElement;
	size_t bitByteIndex = bitIndexInchunk / 8;
	char bitByteMask = 1<<(bitIndexInchunk % 8);
	for (size_t x = 0; x < numChunks; x++)
	{
		pInteresstingByteInDataElement = data + x*chunkSize + bitByteIndex;

		if (*pInteresstingByteInDataElement & bitByteMask) {
			int *p = phase;
			for (; x; x /= 2) *(p++) = 0 + x % 2;
			BDD element = ddmgr_->bddComputeCube(bddVars, (int*)phase, nofBddVars);

			symbolicSet_ += element;
		}
	}

	delete[] phase;

}
 /* function: mintermToElement
 * compute the element of the symbolic set associated with the minterm */
void cuddSymbolicSet::mintermToElement(const int* minterm, double* element) const {
	for (size_t i = 0; i<dim_; i++) {
		size_t idx = 0;
		for (size_t c = 1, j = 0; j<nofBddVars_[i]; c *= 2, j++)
			idx += minterm[indBddVars_[i][j]] * c;
		element[i] = eta_[i] * idx + firstGridPoint_[i];
	}
}
/* function:  getDimension
*
* get the dimension of the real space in which the points that are represented by
* the symbolic set live in */
size_t cuddSymbolicSet::getDimension(void) const {
	return dim_;
}
/* function:  getSymbolicSet
*
* get the BDD  that stores the SymbolicSet
*/
BDD cuddSymbolicSet::getSymbolicSet(void) const {
	return symbolicSet_;
}
/* function:  getCube
*
* returns a BDD with ones in the domain of the SymbolicSet
*/
BDD cuddSymbolicSet::getCube(void) const {
	/* create cube */
	BDD* vars = new BDD[nvars_];
	for (size_t k = 0, i = 0; i<dim_; i++) {
		for (size_t j = 0; j<nofBddVars_[i]; k++, j++)
			vars[k] = ddmgr_->bddVar(indBddVars_[i][j]);
	}
	BDD cube = ddmgr_->bddComputeCube(vars, NULL, nvars_);
	delete[] vars;

	return cube;
}
/* function: setSymbolicSet
*
* set the symbolic set to symset
*/
void cuddSymbolicSet::setSymbolicSet(const BDD symset) {
	/*get variable ids not used in this set*/
	std::vector<size_t> coIndBddVars;
	for (size_t i = 0; i< (size_t)ddmgr_->ReadSize(); i++) {
		bool isinside = 0;
		for (size_t d = 0; d<dim_; d++)
			for (size_t j = 0; j<nofBddVars_[d]; j++)
				if (i == indBddVars_[d][j])
					isinside = true;

		if (!isinside)
			coIndBddVars.push_back(i);
	}
	/* create cube to existentially abstract all bdd ids that are
	* not one of the indBddVar of this SymbolicSet
	*/
	size_t t = coIndBddVars.size();
	BDD* vars = new BDD[t];
	for (size_t k = 0; k<t; k++)
		vars[k] = ddmgr_->bddVar(coIndBddVars[k]);
	BDD cube = ddmgr_->bddComputeCube(vars, NULL, t);
	delete[] vars;

	symbolicSet_ = symset.ExistAbstract(cube);
}
/* function:  getnofbddvars
*
* get the pointer to the size_t array that contains the number of bdd
* variables
*/
size_t* cuddSymbolicSet::getNofBddVars(void) const {
	return nofBddVars_;
}
/* function:  getIndBddVars
*
* get the pointer to the size_t array of indices of the bdd
* variables
*/
size_t** cuddSymbolicSet::getIndBddVars(void) const {
	return indBddVars_;
}
/* function:  setIndBddVars
*
* set the indices of the bdd variables to the indices provided in
* size_t* newIndBddVars
* NOTE: the size of pointer newIndBddVars needs to be dim_*nofBddVars_!
*/
void cuddSymbolicSet::setIndBddVars(size_t* newIndBddVars) {
	for (size_t i = 0, k = 0; i<dim_; i++)
		for (size_t j = 0; j<nofBddVars_[i]; k++, j++)
			indBddVars_[i][j] = newIndBddVars[k];
}
/* function:  getSize
* get the number of grid points in the symbolic set */
double cuddSymbolicSet::getSize(void) {
	return symbolicSet_.CountMinterm(nvars_);
}
/* function:  getSize
*
* project the SymbolicSet on the dimensions specified in projectDimension and then
* count the number of elements
*/
double cuddSymbolicSet::getSize(std::vector<size_t> projectDimension) {
	size_t n = projectDimension.size();
	if (n>dim_) {
		std::ostringstream os;
		os << "Error: cuddSymbolicSet::getSize(projectDimension): number of dimensions onto which the SymbolicSet should be projected exceeds dimension.";
		throw std::invalid_argument(os.str().c_str());
	}
	for (size_t i = 0; i<n; i++) {
		if (projectDimension[i] >= dim_) {
			std::ostringstream os;
			os << "Error: cuddSymbolicSet::getSize(projectDimension): dimensions onto which the SymbolicSet should be projected exceeds dimension.";
			throw std::invalid_argument(os.str().c_str());
		}
	}
	/* compute complement dim */
	std::vector<size_t> codim;
	for (size_t i = 0; i< dim_; i++) {
		if (!(std::find(projectDimension.begin(), projectDimension.end(), i) != projectDimension.end()))
			codim.push_back(i);
	}
	/* create cube to be used in the ExistAbstract command */
	size_t t = 0;
	for (size_t i = 0; i<codim.size(); i++)
		t += nofBddVars_[codim[i]];
	BDD* vars = new BDD[t];
	for (size_t k = 0, i = 0; i<codim.size(); i++) {
		for (size_t j = 0; j<nofBddVars_[codim[i]]; k++, j++)
			vars[k] = ddmgr_->bddVar(indBddVars_[codim[i]][j]);
	}
	BDD cube = ddmgr_->bddComputeCube(vars, NULL, t);
	BDD bdd = symbolicSet_.ExistAbstract(cube);
	delete[] vars;

	return bdd.CountMinterm(nvars_ - t);
}
/* function: clear
* empty the symbolic set*/
void cuddSymbolicSet::clear() {
	symbolicSet_ = ddmgr_->bddZero();
}
/* function: begin
* initilize the iterator and compute the first element */
void cuddSymbolicSet::begin(void) {
	std::vector<size_t> ivars_;
	ivars_.reserve(nvars_);
	for (size_t i = 0; i<dim_; i++)
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			ivars_.push_back(indBddVars_[i][j]);
	/* set up iterator */
	iterator_ = new cuddMintermIterator(symbolicSet_, ivars_, nvars_);
}
/* function: next
* compute the next element */
void cuddSymbolicSet::next(void) {
	++(*iterator_);
}
/* function: progress
* print progess of iteratin in percent */
void cuddSymbolicSet::progress(void) {
	iterator_->printProgress();
}
/* function: done
* changes to one if iterator reached the last element */
int cuddSymbolicSet::done(void) {
	if (iterator_->done()) {
		delete iterator_;
		iterator_ = NULL;
		return 1;
	}
	else
		return 0;
}
/* function: currentMinterm
* returns the pointer to the current minterm in the iteration */
const int* cuddSymbolicSet::currentMinterm(void) {
	if (iterator_)
		return iterator_->currentMinterm();
	else
		throw std::runtime_error("No iterator is under work !");
}

// by: MKhaled [08.2018, Berkeley, CA]
// returns the flat index of the current minterm
// flat means that we assume all dimensions are flattened to one dimension
// here the return value represents the index in the flat-domain of the element
// from the higher dimensional domain after flattening
size_t cuddSymbolicSet::currentMintermFlatValue(void) {
	if (iterator_){
		const int* minterm = iterator_->currentMinterm();
		size_t val = 0;
		size_t mintermBitIndex = 0;
		for (size_t d = 0; d < dim_; d++){
			size_t per_dim_val = 0;
			size_t perDimValueBitIndex = 0;
			for (size_t i = 0; i < nofBddVars_[d]; i++){
				if (minterm[mintermBitIndex]) {
					per_dim_val |= 1 << perDimValueBitIndex;
				}
				mintermBitIndex++;
				perDimValueBitIndex++;
			}

			size_t sub_dim_volume = 1;
			for (int i = 0; i < d; i++)
				sub_dim_volume *= nofGridPoints_[i];

			val += per_dim_val*sub_dim_volume;
		}
		return val;
	}
	else
		throw std::runtime_error("No iterator is under work !");
}
/* function: addGridPoints
* initially the symbolic set is empty
*
* with this command all grid points of the uniform grid are added to the set*/
void cuddSymbolicSet::addGridPoints(void) {
	symbolicSet_ |= cuddSymbolicSet::computeGridPoints();
}
/* function: addByFunction
* a grid point x is added to the SymbolicSet if(function(x)) is true
*/
template<class F> void cuddSymbolicSet::addByFunction(F &function) {

	/* set up grid to iterate over */
	BDD grid = cuddSymbolicSet::computeGridPoints();
	/* set to be added */
	BDD set = ddmgr_->bddZero();

	/* idx of all variables */
	std::vector<size_t> ivars_;
	ivars_.reserve(nvars_);
	BDD* bddVars = new BDD[nvars_];
	size_t k = 0;
	for (size_t i = 0; i<dim_; i++) {
		for (size_t j = 0; j<nofBddVars_[i]; j++) {
			ivars_.push_back(indBddVars_[i][j]);
			bddVars[k++] = ddmgr_->bddVar(indBddVars_[i][j]);
		}
	}

	/* set up iterator */
	cuddMintermIterator it(grid, ivars_, nvars_);
	/* current grid point in iteration */
	double *x = new double[dim_];
	/* get pointer to minterm */
	const int* minterm = it.currentMinterm();
	int* cube = (int*)minterm;

	/* loop over all elements */
	while (!it.done()) {
		/* convert current minterm to element (=grid point) */
		mintermToElement(minterm, x);
		/* cube is inside set */
		if (function(x))
			/* this is a bottelneck! */
			set |= ddmgr_->bddComputeCube(bddVars, cube, nvars_);
		++it; ++k;
	}
	delete[] x;
	delete[] bddVars;

	symbolicSet_ |= set;
}
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
void cuddSymbolicSet::addPolytope(const size_t p, const double* H, const double* h, const ApproximationType type) {
	BDD polytope = cuddSymbolicSet::computePolytope(p, H, h, type);
	symbolicSet_ |= polytope;
}
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
void cuddSymbolicSet::remPolytope(const size_t p, const double* H, const double* h, const ApproximationType type) {
	BDD polytope = cuddSymbolicSet::computePolytope(p, H, h, type);
	symbolicSet_ &= (!polytope);
}
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
void cuddSymbolicSet::addEllipsoid(const double* L, const double* c, const ApproximationType type) {
	BDD ellisoid = cuddSymbolicSet::computeEllipsoid(L, c, type);
	symbolicSet_ |= ellisoid;
}
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
void cuddSymbolicSet::remEllipsoid(const double* L, const double* c, const ApproximationType type) {
	BDD ellisoid = cuddSymbolicSet::computeEllipsoid(L, c, type);
	symbolicSet_ &= (!ellisoid);
}
/* function: printInfo
* print some numbers related to the symbolic set*/
void cuddSymbolicSet::printInfo(int verbosity) const {
	size_t totNoBddVars = 0;
	size_t totNoGridPoints = 1;
	for (size_t i = 0; i<dim_; i++) {
		totNoBddVars += nofBddVars_[i];
		totNoGridPoints *= nofGridPoints_[i];
	}

	std::cout << "First grid point: ";
	for (size_t i = 0; i<dim_; i++)
		std::cout << firstGridPoint_[i] << " ";
	std::cout << std::endl;
	if (verbosity) {
		std::cout << "Last grid point: ";
		for (size_t i = 0; i<dim_; i++)
			std::cout << lastGridPoint_[i] << " ";
		std::cout << std::endl;
	}


	std::cout << "Grid node distance (eta)in each dimension: ";
	for (size_t i = 0; i<dim_; i++)
		std::cout << eta_[i] << " ";
	std::cout << std::endl;

	std::cout << "Number of grid points in each dimension: ";
	for (size_t i = 0; i<dim_; i++)
		std::cout << nofGridPoints_[i] << " ";
	std::cout << std::endl;


	std::cout << "Number of grid points in domain: " << totNoGridPoints << std::endl;
	if (symbolicSet_ != ddmgr_->bddZero())
		std::cout << "Number of elements in the symbolic set: " << symbolicSet_.CountMinterm(totNoBddVars) << std::endl;
	else {
		std::cout << "Symbolic set does not contain any grid points." << std::endl;
	}

	if (verbosity) {
		std::cout << "Number of binary variables: " << totNoBddVars << std::endl;
		for (size_t i = 0; i<dim_; i++) {
			std::cout << "Bdd variable indices (=IDs) in dim " << i + 1 << ": ";
			for (size_t j = 0; j<nofBddVars_[i]; j++)
				std::cout << indBddVars_[i][j] << " ";
			std::cout << std::endl;
		}
	}
	if (verbosity>1)
		symbolicSet_.PrintMinterm();
}
/* function:  writeToFile
* store the bdd with all information to file*/
void cuddSymbolicSet::writeToFile(const char *filename) {
	/* produce string with parameters to be written into file */
	std::stringstream dim;
	std::stringstream z;
	std::stringstream eta;
	std::stringstream first;
	std::stringstream last;
	std::stringstream nofGridPoints;
	std::stringstream indBddVars;

	dim << "#symset dimension: " << dim_ << std::endl;
	z << "#symset measurement error bound: ";
	eta << "#symset grid parameter eta: ";
	first << "#symset coordinate of first grid point: ";
	last << "#symset coordinate of last grid point: ";
	nofGridPoints << "#symset number of grid points (per dim): ";
	indBddVars << "#symset index of bdd variables: ";

	for (size_t i = 0; i<dim_; i++) {
		z << z_[i] << " ";
		eta << eta_[i] << " ";
		first << firstGridPoint_[i] << " ";
		last << lastGridPoint_[i] << " ";
		nofGridPoints << nofGridPoints_[i] << " ";
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			indBddVars << indBddVars_[i][j] << " ";
	}
	z << std::endl;
	eta << std::endl;
	first << std::endl;
	last << std::endl;
	nofGridPoints << std::endl;
	indBddVars << std::endl;

	std::stringstream paramss;
	paramss << "################################################################################" << std::endl
		<< "########### symbolic controller synthesis toolbox information added ############" << std::endl
		<< "################################################################################" << std::endl
		<< dim.str() \
		<< eta.str() \
		<< z.str() \
		<< first.str() \
		<< last.str() \
		<< nofGridPoints.str() \
		<< indBddVars.str();
	std::string param = paramss.str();


	FILE *file = fopen(filename, "w");
	if (file == NULL) {
		std::ostringstream os;
		os << "Error: Unable to open file for writing." << filename << "'.";
		throw std::runtime_error(os.str().c_str());
	}
	if (param.size() != fwrite(param.c_str(), 1, param.size(), file)) {
		throw "Error: Unable to write set information to file.";
	}

	/* before we save the BDD to file, we save it to another manager,
	* because the current manager is classified as ADD manager */
	Cudd mdest;
	BDD tosave = symbolicSet_.Transfer(mdest);

	int storeReturnValue = Dddmp_cuddBddStore(
		mdest.getManager(),
		NULL,
		tosave.getNode(),
		//(char**)varnameschar, // char ** varnames, IN: array of variable names (or NULL)
		NULL, // char ** varnames, IN: array of variable names (or NULL)
		NULL,
		DDDMP_MODE_BINARY,
		// DDDMP_VARNAMES,
		DDDMP_VARIDS,
		NULL,
		file
	);

	fclose(file);
	if (storeReturnValue != DDDMP_SUCCESS)
		throw "Error: Unable to write BDD to file.";
	else
		std::cout << "Symbolic set saved to file: " << filename << std::endl;
}
/* function:  getZ
* return constant pointer to the double array z_*/
const double* cuddSymbolicSet::getZ() const {
	return z_;
}
/* function:  getEta
* return constant pointer to the double array eta_*/
const double* cuddSymbolicSet::getEta() const {
	return eta_;
}
/* function: complement
* compute the of the symbolic set with respect to the uniform grid*/
void cuddSymbolicSet::complement() {
	/* generate ADD variables and ADD for the grid points in each dimension */
	ADD constant;
	ADD* aVar = new ADD[dim_];
	ADD** addVars = new ADD*[dim_];
	for (size_t i = 0; i<dim_; i++) {
		aVar[i] = ddmgr_->addZero();
		addVars[i] = new ADD[nofBddVars_[i]];
		CUDD_VALUE_TYPE c = 1;
		for (size_t j = 0; j<nofBddVars_[i]; j++) {
			constant = ddmgr_->constant(c);
			addVars[i][j] = ddmgr_->addVar(indBddVars_[i][j]);
			addVars[i][j] *= constant;
			aVar[i] += addVars[i][j];
			c *= 2;
		}
	}
	/* set grid points outside of the uniform grid to infinity */
	BDD outside = ddmgr_->bddZero();
	for (size_t i = 0; i<dim_; i++) {
		BDD bdd = aVar[i].BddThreshold(nofGridPoints_[i]);
		outside += bdd;;
	}
	for (size_t i = 0; i<dim_; i++)
		delete[] addVars[i];
	delete[] aVar;

	BDD inside = !outside;
	inside &= !symbolicSet_;
	symbolicSet_ = inside;
}
/* function:  getFirstGridPoint
* the first grid point is saved to a double array of size dim_*/
const double* cuddSymbolicSet::getFirstGridPoint() const {
	return firstGridPoint_;
}
/* function:  getLastGridPoint
* the last grid point is saved to a double array of size dim_*/
const double* cuddSymbolicSet::getLastGridPoint() const {
	return lastGridPoint_;
}
/* function:  getNofGridPoints
* return pointer to size_t array containing the number of grid points*/
const size_t* cuddSymbolicSet::getNofGridPoints() const {
	return nofGridPoints_;
}
/* function:  copyZ
* z_ is written to the double array z of size dim_*/
void cuddSymbolicSet::copyZ(double z[]) const {
	for (size_t i = 0; i<dim_; i++)
		z[i] = z_[i];
}
/* function:  copyEta
* eta_ is written to the double array eta of size dim_*/
void cuddSymbolicSet::copyEta(double eta[]) const {
	for (size_t i = 0; i<dim_; i++)
		eta[i] = eta_[i];
}
/* function:  copyFirstGridPoint
* the first grid point is saved to a double array of size dim_*/
void cuddSymbolicSet::copyFirstGridPoint(double firstGridPoint[]) const {
	for (size_t i = 0; i<dim_; i++)
		firstGridPoint[i] = firstGridPoint_[i];
}
/* function:  copyLastGridPoint
* the last grid point is saved to a double array of size dim_*/
void cuddSymbolicSet::copyLastGridPoint(double lastGridPoint[]) const {
	for (size_t i = 0; i<dim_; i++)
		lastGridPoint[i] = lastGridPoint_[i];
}
/* function:  copyNofGridPoints
* copy the size_t array containing the number of grid points to nofGridPoints*/
void cuddSymbolicSet::getNofGridPoints(size_t nofGridPoints[]) const {
	for (size_t i = 0; i<dim_; i++)
		nofGridPoints[i] = nofGridPoints_[i];
}
/* function: copyGridPoints
* the grid points in the symbolic set are saved to the double array of size (dim_) x (setSize) */
void cuddSymbolicSet::copyGridPoints(double gridPoints[]) const {
	std::vector<size_t> ivars_;
	ivars_.reserve(nvars_);
	for (size_t i = 0; i<dim_; i++)
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			ivars_.push_back(indBddVars_[i][j]);
	/* number of minterms in the symbolic set */
	size_t nofMinterms = symbolicSet_.CountMinterm(nvars_);
	/* set up iterator */
	cuddMintermIterator it(symbolicSet_, ivars_, nvars_);
	/* get pointer to minterm */
	const int* minterm = it.currentMinterm();
	size_t k = 0;
	while (!it.done()) {
		//it.printMinterm();
		//it.printProgress();
		for (size_t i = 0; i<dim_; i++) {
			int c = 1;
			double num = 0;
			for (size_t j = 0; j<nofBddVars_[i]; j++) {
				num += c*(double)minterm[indBddVars_[i][j]];
				c *= 2;
			}
			gridPoints[i*nofMinterms + k] = firstGridPoint_[i] + num*eta_[i];
		}
		//for(size_t i=0; i<dim_; i++) 
		//  std::cout << gridPoints[i*nofMinterms+k] << " " ;
		//std::cout << std::endl;
		++it; ++k;
	}
}
void cuddSymbolicSet::copyGridPointIndicies(size_t gridPoints[], bool sortIt) const {
	std::vector<size_t> ivars_;
	ivars_.reserve(nvars_);
	for (size_t i = 0; i<dim_; i++)
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			ivars_.push_back(indBddVars_[i][j]);
	/* number of minterms in the symbolic set */
	size_t nofMinterms = symbolicSet_.CountMinterm(nvars_);
	/* set up iterator */
	cuddMintermIterator it(symbolicSet_, ivars_, nvars_);
	/* get pointer to minterm */
	const int* minterm = it.currentMinterm();
	size_t k = 0;
	while (!it.done()) {
		//it.printMinterm();
		//it.printProgress();
		for (size_t i = 0; i<dim_; i++) {
			int c = 1;
			double num = 0;
			for (size_t j = 0; j<nofBddVars_[i]; j++) {
				num += c*(double)minterm[indBddVars_[i][j]];
				c *= 2;
			}
			gridPoints[i*nofMinterms + k] = num;
		}
		//for(size_t i=0; i<dim_; i++) 
		//  std::cout << gridPoints[i*nofMinterms+k] << " " ;
		//std::cout << std::endl;
		++it; ++k;
	}

	if (sortIt) {
		std::sort(gridPoints, gridPoints + k);
	}
}
/* function: copyGridPoints
*
* the SymbolicSet is projected onto the dimensions specified in  projectDimension
*
* the grid points in the resultilng set are saved to the double array of size (dim_) x (setSize)
*/
void cuddSymbolicSet::copyGridPoints(double gridPoints[], std::vector<size_t> projectDimension, bool sortIt) const {
	/* check input data */
	size_t n = projectDimension.size();
	if (n >= dim_) {
		std::ostringstream os;
		os << "Error: cuddSymbolicSet::getSize(projectDimension): number of dimensions onto which the SymbolicSet should be projected exceeds dimension.";
		throw std::invalid_argument(os.str().c_str());
	}
	for (size_t i = 0; i<n; i++) {
		if (projectDimension[i] >= dim_) {
			std::ostringstream os;
			os << "Error: cuddSymbolicSet::getSize(projectDimension): dimensions onto which the SymbolicSet should be projected exceeds dimension.";
			throw std::invalid_argument(os.str().c_str());
		}
	}
	/* compute complement dim */
	std::vector<size_t> codim;
	for (size_t i = 0; i< dim_; i++) {
		if (!(std::find(projectDimension.begin(), projectDimension.end(), i) != projectDimension.end()))
			codim.push_back(i);
	}
	/* create cube to be used in the ExistAbstract command */
	size_t t = 0;
	for (size_t i = 0; i<codim.size(); i++)
		t += nofBddVars_[codim[i]];
	BDD* vars = new BDD[t];
	for (size_t k = 0, i = 0; i<codim.size(); i++) {
		for (size_t j = 0; j<nofBddVars_[codim[i]]; k++, j++)
			vars[k] = ddmgr_->bddVar(indBddVars_[codim[i]][j]);
	}
	BDD cube = ddmgr_->bddComputeCube(vars, NULL, t);
	/* projected set */
	BDD bdd = symbolicSet_.ExistAbstract(cube);
	/* number of variables */
	size_t  pnvars = nvars_ - t;
	std::vector<size_t> ivars;
	ivars.reserve(pnvars);
	for (size_t i = 0; i<n; i++)
		for (size_t j = 0; j<nofBddVars_[projectDimension[i]]; j++)
			ivars.push_back(indBddVars_[projectDimension[i]][j]);
	/* number of minterms in the symbolic set */
	size_t nofMinterms = bdd.CountMinterm(pnvars);
	/* set up iterator */
	cuddMintermIterator it(bdd, ivars, pnvars);
	/* get pointer to minterm */
	const int* minterm = it.currentMinterm();
	size_t k = 0;
	while (!it.done()) {
		//it.printMinterm();
		//it.printProgress();
		for (size_t i = 0; i<n; i++) {
			int c = 1;
			double num = 0;
			for (size_t j = 0; j<nofBddVars_[projectDimension[i]]; j++) {
				num += c*(double)minterm[indBddVars_[projectDimension[i]][j]];
				c *= 2;
			}
			gridPoints[i*nofMinterms + k] = firstGridPoint_[projectDimension[i]] + num*eta_[projectDimension[i]];
		}
		//for(size_t i=0; i<dim_; i++) 
		//  std::cout << gridPoints[i*nofMinterms+k] << " " ;
		//std::cout << std::endl;
		++it; ++k;
	}
	delete[] vars;


	if (sortIt) {
		std::sort(gridPoints, gridPoints + k);
	}
}
void cuddSymbolicSet::copyGridPointIndicies(size_t gridPoints[], std::vector<size_t> projectDimension, bool sortIt) const {
	/* check input data */
	size_t n = projectDimension.size();
	if (n >= dim_) {
		std::ostringstream os;
		os << "Error: cuddSymbolicSet::getSize(projectDimension): number of dimensions onto which the SymbolicSet should be projected exceeds dimension.";
		throw std::invalid_argument(os.str().c_str());
	}
	for (size_t i = 0; i<n; i++) {
		if (projectDimension[i] >= dim_) {
			std::ostringstream os;
			os << "Error: cuddSymbolicSet::getSize(projectDimension): dimensions onto which the SymbolicSet should be projected exceeds dimension.";
			throw std::invalid_argument(os.str().c_str());
		}
	}
	/* compute complement dim */
	std::vector<size_t> codim;
	for (size_t i = 0; i< dim_; i++) {
		if (!(std::find(projectDimension.begin(), projectDimension.end(), i) != projectDimension.end()))
			codim.push_back(i);
	}
	/* create cube to be used in the ExistAbstract command */
	size_t t = 0;
	for (size_t i = 0; i<codim.size(); i++)
		t += nofBddVars_[codim[i]];
	BDD* vars = new BDD[t];
	for (size_t k = 0, i = 0; i<codim.size(); i++) {
		for (size_t j = 0; j<nofBddVars_[codim[i]]; k++, j++)
			vars[k] = ddmgr_->bddVar(indBddVars_[codim[i]][j]);
	}
	BDD cube = ddmgr_->bddComputeCube(vars, NULL, t);
	/* projected set */
	BDD bdd = symbolicSet_.ExistAbstract(cube);
	/* number of variables */
	size_t  pnvars = nvars_ - t;
	std::vector<size_t> ivars;
	ivars.reserve(pnvars);
	for (size_t i = 0; i<n; i++)
		for (size_t j = 0; j<nofBddVars_[projectDimension[i]]; j++)
			ivars.push_back(indBddVars_[projectDimension[i]][j]);
	/* number of minterms in the symbolic set */
	size_t nofMinterms = bdd.CountMinterm(pnvars);
	/* set up iterator */
	cuddMintermIterator it(bdd, ivars, pnvars);
	/* get pointer to minterm */
	const int* minterm = it.currentMinterm();
	size_t k = 0;
	while (!it.done()) {
		//it.printMinterm();
		//it.printProgress();
		for (size_t i = 0; i<n; i++) {
			int c = 1;
			size_t num = 0;
			for (size_t j = 0; j<nofBddVars_[projectDimension[i]]; j++) {
				num += c*minterm[indBddVars_[projectDimension[i]][j]];
				c *= 2;
			}
			gridPoints[i*nofMinterms + k] = num;
		}
		//for(size_t i=0; i<dim_; i++) 
		//  std::cout << gridPoints[i*nofMinterms+k] << " " ;
		//std::cout << std::endl;
		++it; ++k;
	}
	delete[] vars;

	if (sortIt) {
		std::sort(gridPoints, gridPoints + k);
	}
}
/* function: isElement
*
* check if an element x is an element of the SymolicSet
*
*  x -- x is an element of the domain
*
*/
bool cuddSymbolicSet::isElement(std::vector<double> x) {
	if (!(x.size() == dim_)) {
		std::ostringstream os;
		os << "Error: cuddSymbolicSet::isElement(x): x must be of size dim_.";
		throw std::invalid_argument(os.str().c_str());
	}
	/* check if x is inside domain */
	for (size_t i = 0; i<dim_; i++)
		if (x[i]>(lastGridPoint_[i] + eta_[i] / 2) || x[i]<(firstGridPoint_[i] - eta_[i] / 2))
			return false;

	/* combute BDD representation of x */
	BDD cube = ddmgr_->bddOne();
	for (size_t i = 0; i<dim_; i++) {
		int *phase = new int[nofBddVars_[i]];
		BDD *vars = new BDD[nofBddVars_[i]];
		for (size_t j = 0; j<nofBddVars_[i]; j++) {
			phase[j] = 0;
			vars[j] = ddmgr_->bddVar(indBddVars_[i][j]);
		}
		int *p = phase;
		int idx = std::lround((x[i] - firstGridPoint_[i]) / eta_[i]);
		for (; idx; idx /= 2) *(p++) = 0 + idx % 2;
		cube &= ddmgr_->bddComputeCube(vars, phase, nofBddVars_[i]);
		delete[] phase;
		delete[] vars;
	}
	/* check if cube is element of symbolicSet_ */
	if ((cube & symbolicSet_) != ddmgr_->bddZero())
		return true;
	else
		return false;
}
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
std::vector<std::vector<double>> cuddSymbolicSet::setValuedMap(std::vector<double> x, std::vector<size_t> ind) {
	if (!(x.size() == ind.size())) {
		std::ostringstream os;
		os << "Error: cuddSymbolicSet::setValuedMap(x,ind): x and ind must be of the same length.";
		throw std::invalid_argument(os.str().c_str());
	}
	if (ind.size() >= dim_) {
		std::ostringstream os;
		os << "Error: cuddSymbolicSet::setValuedMap(x,ind): size of ind cannot exceed dim_.";
		throw std::invalid_argument(os.str().c_str());
	}
	size_t n = x.size();
	for (size_t i = 0; i<n; i++) {
		if (ind[i] >= dim_) {
			std::cout << ind[i] << std::endl;
			std::ostringstream os;
			os << "Error: cuddSymbolicSet::setValuedMap(x,ind): ind[i] exceeds the number of the grid dimension.";
			throw std::invalid_argument(os.str().c_str());
		}
	}
	/* is x outside domian of SymbolicSet ? */
	for (size_t i = 0; i<n; i++) {
		size_t j = ind[i];
		if (x[j]>(lastGridPoint_[j] + eta_[j] / 2) || x[j]<(firstGridPoint_[j] - eta_[j] / 2)) {
			std::vector<std::vector<double>> image;
			return image;
		}
	}
	/* compute co-domain indices */
	std::vector<size_t> coind;
	for (size_t i = 0; i< dim_; i++) {
		if (!(std::find(ind.begin(), ind.end(), i) != ind.end()))
			coind.push_back(i);
	}

	/* create cube that represents x */
	BDD cube = ddmgr_->bddOne();
	for (size_t i = 0; i<n; i++) {
		size_t nvars = nofBddVars_[ind[i]];
		int *phase = new int[nvars];
		BDD *vars = new BDD[nvars];
		for (size_t j = 0; j<nvars; j++) {
			phase[j] = 0;
			vars[j] = ddmgr_->bddVar(indBddVars_[ind[i]][j]);
		}
		int *p = phase;
		int idx = std::lround((x[i] - firstGridPoint_[ind[i]]) / eta_[ind[i]]);
		for (; idx; idx /= 2) *(p++) = 0 + idx % 2;
		cube &= ddmgr_->bddComputeCube(vars, phase, nvars);
		delete[] phase;
		delete[] vars;
	}
	/* extract the elements from symbolicSet_ associated with proj */
	BDD proj = cube & symbolicSet_;
	/* setup return matrix */
	std::vector<std::vector<double>> image;
	/* variables are used to iterate over all elements in proj */
	std::vector<size_t> ivars;
	for (size_t i = 0; i<dim_; i++) {
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			ivars.push_back(indBddVars_[i][j]);
	}
	double* element = new double[dim_];
	cuddMintermIterator  it(proj, ivars, nvars_);
	for (; !it.done(); ++it) {
		mintermToElement(it.currentMinterm(), element);
		std::vector<double> v(coind.size());
		for (size_t i = 0; i<coind.size(); i++)
			v[i] = element[coind[i]];
		image.push_back(v);
	}
	delete[] element;
	return image;
}
/* function: readMembersFromFile
 * reads the member variables from the file */
void cuddSymbolicSet::readMembersFromFile(const char *filename) {
	/* open file */
	std::ifstream bddfile(filename);
	if (!bddfile.good()) {
		std::ostringstream os;
		os << "Error: Unable to open file:" << filename << "'.";
		throw std::runtime_error(os.str().c_str());
	}
	/* read dimension from file */
	std::string line;
	while (!bddfile.eof()) {
		std::getline(bddfile, line);
		if (line.substr(0, 7) == "#symset") {
			if (line.find("dimension") != std::string::npos) {
				std::istringstream sline(line.substr(line.find(":") + 1));
				sline >> dim_;
			}
		}
	}
	if (dim_ == 0) {
		std::ostringstream os;
		os << "Error: Could not read dimension from file: " << filename << ". ";
		os << "Was " << filename << " created with cuddSymbolicSet::writeToFile?";
		throw std::runtime_error(os.str().c_str());
	}
	z_ = new double[dim_];
	eta_ = new double[dim_];
	lastGridPoint_ = new double[dim_];
	firstGridPoint_ = new double[dim_];
	nofGridPoints_ = new size_t[dim_];
	nofBddVars_ = new size_t[dim_];
	/* read eta/first/last/no of grid points/no of bdd vars */
	bddfile.clear();
	bddfile.seekg(0, std::ios::beg);
	int check = 0;
	while (!bddfile.eof()) {
		std::getline(bddfile, line);
		if (line.substr(0, 7) == "#symset") {
			/* read eta */
			if (line.find("eta") != std::string::npos) {
				check++;
				std::istringstream sline(line.substr(line.find(":") + 1));
				for (size_t i = 0; i<dim_; i++)
					sline >> eta_[i];
			}
			/* read z */
			if (line.find("measurement") != std::string::npos) {
				check++;
				std::istringstream sline(line.substr(line.find(":") + 1));
				for (size_t i = 0; i<dim_; i++)
					sline >> z_[i];
			}
			/* read first grid point*/
			if (line.find("first") != std::string::npos) {
				check++;
				std::istringstream sline(line.substr(line.find(":") + 1));
				for (size_t i = 0; i<dim_; i++)
					sline >> firstGridPoint_[i];
			}
			/* read last grid point*/
			if (line.find("last") != std::string::npos) {
				check++;
				std::istringstream sline(line.substr(line.find(":") + 1));
				for (size_t i = 0; i<dim_; i++)
					sline >> lastGridPoint_[i];
			}
			/* read no of grid points */
			if (line.find("number") != std::string::npos) {
				check++;
				std::istringstream sline(line.substr(line.find(":") + 1));
				for (size_t i = 0; i<dim_; i++) {
					sline >> nofGridPoints_[i];
					if (nofGridPoints_[i] == 1)
						nofBddVars_[i] = 1;
					else
						nofBddVars_[i] = (size_t)std::ceil(std::log2(nofGridPoints_[i]));
				}
			}
		}
		if (check == 5)
			break;
	}
	if (check<5) {
		std::ostringstream os;
		os << "Error: Could not read all parameters from file: " << filename << ". ";
		os << "Was " << filename << " created with cuddSymbolicSet::writeToFile?";
		throw std::runtime_error(os.str().c_str());
	}
	/* read index of bdd vars  */
	indBddVars_ = new size_t*[dim_];
	bddfile.clear();
	bddfile.seekg(0, std::ios::beg);
	check = 0;
	while (!bddfile.eof()) {
		std::getline(bddfile, line);
		if (line.substr(0, 7) == "#symset") {
			if (line.find("index") != std::string::npos) {
				check++;
				std::istringstream sline(line.substr(line.find(":") + 1));
				for (size_t i = 0; i<dim_; i++) {
					indBddVars_[i] = new size_t[nofBddVars_[i]];
					for (size_t j = 0; j<nofBddVars_[i]; j++)
						sline >> indBddVars_[i][j];
				}
			}
		}
	}
	if (check != 1) {
		std::ostringstream os;
		os << "Error: Could not read bdd indices from file: " << filename << ".";
		os << "Was " << filename << " created with cuddSymbolicSet::writeToFile?";
		throw std::runtime_error(os.str().c_str());
	}  /* close file */
	bddfile.close();
	/* number of total variables */
	nvars_ = 0;
	for (size_t i = 0; i<dim_; i++)
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			nvars_++;

} /* end readMembersFromFile */
/* function: computeGridPoints 
 *compute a bdd containing all the grid points */
BDD cuddSymbolicSet::computeGridPoints(void) {
	BDD grid = ddmgr_->bddOne();
	BDD **bddVars = new BDD*[dim_];
	BDD *symset = new BDD[dim_];
	for (size_t i = 0; i<dim_; i++) {
		symset[i] = ddmgr_->bddZero();
		bddVars[i] = new BDD[nofBddVars_[i]];
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			bddVars[i][j] = ddmgr_->bddVar(indBddVars_[i][j]);
	}
	for (size_t i = 0; i<dim_; i++) {

		int *phase = new int[nofBddVars_[i]];
		for (size_t j = 0; j<nofBddVars_[i]; j++)
			phase[j] = 0;
		for (size_t j = 0; j<nofGridPoints_[i]; j++) {
			int *p = phase;
			int x = j;
			for (; x; x /= 2) *(p++) = 0 + x % 2;
			symset[i] += ddmgr_->bddComputeCube(bddVars[i], (int*)phase, nofBddVars_[i]);
		}
		delete[] phase;
		grid &= symset[i];
	}
	for (size_t i = 0; i<dim_; i++)
		delete[] bddVars[i];
	delete[] bddVars;
	delete[] symset;
	return grid;
}
/* function computePolytope:
 * compute symbolic representation (=bdd) of a polytope { x | Hx<= h }*/
BDD cuddSymbolicSet::computePolytope(const size_t p, const double* H, const double* h, const ApproximationType type) {
	/* generate ADD variables and ADD for the grid points in each dimension */
	ADD constant;
	ADD* aVar = new ADD[dim_];
	ADD** addVars = new ADD*[dim_];
	for (size_t i = 0; i<dim_; i++) {
		aVar[i] = ddmgr_->addZero();
		addVars[i] = new ADD[nofBddVars_[i]];
		CUDD_VALUE_TYPE c = 1;
		for (size_t j = 0; j<nofBddVars_[i]; j++) {
			constant = ddmgr_->constant(c);
			addVars[i][j] = ddmgr_->addVar(indBddVars_[i][j]);
			addVars[i][j] *= constant;
			aVar[i] += addVars[i][j];
			c *= 2;
		}
	}
	/* set grid points outside of the uniform grid to infinity */
	ADD outside = ddmgr_->addZero();
	for (size_t i = 0; i<dim_; i++) {
		BDD bdd = aVar[i].BddThreshold(nofGridPoints_[i]);
		ADD add = bdd.Add()*ddmgr_->minusInfinity();
		outside += add;;
	}

	/* compute values for polytope */
	BDD polytope = ddmgr_->bddOne();
	for (size_t i = 0; i<p; i++) {
		/* update set of outside nodes */
		BDD bdd = !polytope;
		outside += bdd.Add()*ddmgr_->minusInfinity();;
		ADD halfspace = outside;
		for (size_t j = 0; j<dim_; j++) {
			if (H[i*dim_ + j] == 0)
				continue;
			ADD c = ddmgr_->constant((-H[i*dim_ + j]));
			ADD eta = ddmgr_->constant(eta_[j]);
			ADD radius = ddmgr_->constant((eta_[j] / 2.0 + z_[j]));
			ADD first = ddmgr_->constant(firstGridPoint_[j]);
			if ((H[i*dim_ + j] >= 0 && type == OUTER) || (H[i*dim_ + j] < 0 && type == INNER))
				halfspace += ((aVar[j] * eta) + (first - radius))*c;
			else
				halfspace += ((aVar[j] * eta) + (first + radius))*c;
		}
		/* add halfspace to polytope */
		if (type == OUTER)
			polytope &= halfspace.BddThreshold(-(h[i] + ddmgr_->ReadEpsilon()));
		else
			polytope &= halfspace.BddThreshold(-(h[i] - ddmgr_->ReadEpsilon()));
	}
	delete[] aVar;
	for (size_t i = 0; i<dim_; i++)
		delete[]   addVars[i];
	delete[] addVars;
	return polytope;
}
/* function: computeEllipsoid
 * compute symbolic representation (=bdd) of an ellisoid { x | (x-center)' L' L (x-center) <= 1 } */
BDD cuddSymbolicSet::computeEllipsoid(const double* L, const double* center, const ApproximationType type) {
	/* compute the smallest radius over all 2-norm balls that contain
	* the set  L([-eta[0]/2, eta[0]2]x ... x [eta[dim-1]/2, eta[dim-1]/2]) */
	double r = cuddSymbolicSet::computeEllipsoidRadius(L);
	/* now threshold against the grid points are checked  |Lx+c|<= t */
	double t = 1;
	if (type == INNER)
		t = 1 - r - ddmgr_->ReadEpsilon();
	if (type == OUTER)
		t = 1 + r + ddmgr_->ReadEpsilon();
	if (t <= 0)
		return ddmgr_->bddZero();

	BDD grid = cuddSymbolicSet::computeGridPoints();

	BDD ellipsoid = ddmgr_->bddZero();
	double *x = new double[dim_];
	double *y = new double[dim_];
	double val;
	/* idx of all variables */
	std::vector<size_t> ivars_;
	ivars_.reserve(nvars_);
	BDD* bddVars = new BDD[nvars_];
	size_t k = 0;
	for (size_t i = 0; i<dim_; i++) {
		for (size_t j = 0; j<nofBddVars_[i]; j++) {
			ivars_.push_back(indBddVars_[i][j]);
			bddVars[k++] = ddmgr_->bddVar(indBddVars_[i][j]);
		}
	}
	/* set up iterator */
	cuddMintermIterator it(grid, ivars_, nvars_);
	/* get pointer to minterm */
	const int* minterm = it.currentMinterm();
	int* cube = new int[nvars_];
	BDD bddcube;
	k = 0;
	while (!it.done()) {
		//it.printMinterm();
		//it.printProgress();
		for (size_t l = 0, i = 0; i<dim_; i++) {
			int c = 1;
			double num = 0;
			for (size_t j = 0; j<nofBddVars_[i]; j++) {
				cube[l] = minterm[indBddVars_[i][j]];
				num += c*(double)cube[l];
				c *= 2;
				l++;
			}
			x[i] = firstGridPoint_[i] + num*eta_[i] - center[i];
			y[i] = 0;
		}
		for (size_t i = 0; i<dim_; i++)
			for (size_t j = 0; j<dim_; j++)
				y[i] += L[dim_*i + j] * x[j];
		val = 0;
		for (size_t i = 0; i<dim_; i++)
			val += y[i] * y[i];
		/* cube is inside set */
		if (val <= (t*t)) {
			/* this is a bottelneck! */
			bddcube = ddmgr_->bddComputeCube(bddVars, cube, nvars_);
			ellipsoid += bddcube;
		}
		++it; ++k;
	}
	delete[] x;
	delete[] y;
	delete[] cube;
	delete[] bddVars;

	return ellipsoid;
}
/* function computeEllipsoidRadius: 
 * compute smallest radius of ball that contains L*cell */
double cuddSymbolicSet::computeEllipsoidRadius(const double* L) {
	/* compute smallest radius of ball that contains L*cell */
	Cudd mgr;
	ADD* y = new ADD[dim_];
	ADD* x = new ADD[dim_];
	ADD* v = new ADD[dim_];
	for (size_t i = 0; i<dim_; i++) {
		v[i] = mgr.addVar(i);
		x[i] = v[i] * mgr.constant(eta_[i]) - mgr.constant(eta_[i] / 2.0 + z_[i]);
	}
	/* compute y= Lx */
	ADD lij;
	for (size_t i = 0; i<dim_; i++) {
		y[i] = mgr.addZero();
		for (size_t j = 0; j<dim_; j++) {
			lij = mgr.constant(L[dim_*i + j]);
			y[i] += lij*x[j];
		}
	}
	/* compute r = x'L'Lx */
	ADD r = mgr.addZero();
	for (size_t i = 0; i<dim_; i++)
		r += y[i] * y[i];
	ADD max = r.FindMax();
	delete[] x;
	delete[] y;
	delete[] v;

	return std::sqrt(Cudd_V(max.getNode()));
}


/* shifts all bdd Variables t sstart with vddVar with index 0 with no spacing of vars are scattered */
void cuddSymbolicSet::permuteVarsToVarZero() {
	
	// prepare the permute-map
	size_t allManagerVarsCount = ddmgr_->ReadSize();
	std::vector<int> perMuteMap(allManagerVarsCount);
	for (size_t i = 0; i < allManagerVarsCount; i++)
		perMuteMap[i] = i;

	// permuting the map
	size_t flatIdx = 0;
	for (size_t d = 0; d < dim_; d++){
		for (size_t i = 0; i < nofBddVars_[d]; i++){
			perMuteMap[flatIdx] = indBddVars_[d][i];
			perMuteMap[indBddVars_[d][i]] = flatIdx;
			flatIdx++;
		}
	}

	// apply permute
	symbolicSet_ = symbolicSet_.Permute(perMuteMap.data());

	// set new indicies
	flatIdx = 0;
	for (size_t d = 0; d < dim_; d++) {
		for (size_t i = 0; i < nofBddVars_[d]; i++) {
			indBddVars_[d][i] = flatIdx;
			flatIdx++;
		}
	}
}

