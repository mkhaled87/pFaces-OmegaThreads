/*
 * RungeKutta4.hh
 *
 *  created on: 22.04.2015
 *      author: rungger
 */

#ifndef RUNGEKUTTA4_HH_
#define RUNGEKUTTA4_HH_

/* class: RungeKutta4 
 * Fixed step size ode solver implementing a Runge Kutta scheme of order 4 */
class OdeSolver {
private:
  /* dimension */
  const double dim_;
  /* number of intermediate steps */
  const int nint_;
  /* intermidiate step size = tau_/nint_ */
  const double h_;
public:
  /* function: OdeSolver
   *
   * Input:
   * dim - state space dimension 
   * nint - number of intermediate steps
   * tau - sampling time
   */
  OdeSolver(const int dim, const int nint, const double tau) : dim_(dim), nint_(nint), h_(tau/nint) { };

  /* function: ()
   *
   * Input:
   * rhs - rhs of ode  dx/dt = rhs(x,u)
   * x - current state x
   * u - current input u
   */
  template<class RHS, class X, class U>
  inline void operator()(RHS rhs, X &x, U &u) {
		X k[4];
		X tmp;

		for(int t=0; t<nint_; t++) {
			rhs(k[0],x,u);
			for(int i=0;i<dim_;i++)
				tmp[i]=x[i]+h_/2*k[0][i];

			rhs(k[1],tmp, u);
			for(int i=0;i<dim_;i++)
				tmp[i]=x[i]+h_/2*k[1][i];

			rhs(k[2],tmp, u);
			for(int i=0;i<dim_;i++)
				tmp[i]=x[i]+h_*k[2][i];

			rhs(k[3],tmp, u);
			for(int i=0; i<dim_; i++)
				x[i] = x[i] + (h_/6)*(k[0][i] + 2*k[1][i] + 2*k[2][i] + k[3][i]);
		}
	}
};

#endif /* RUNGEKUTTA4_HH_ */
