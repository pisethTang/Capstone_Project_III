#include <iostream>
#include <fstream> // <-- Added for file output
#include <boost/numeric/odeint.hpp>

using namespace std;
using namespace boost::numeric::odeint;

// We will write data to this file
std::ofstream data_file("simple_ode.dat");

/*
 * we solve the simple ODE x' = 3/(2t^2) + x/(2t)
 * with initial condition x(1) = 0.
 * Analytic solution is x(t) = sqrt(t) - 1/t
 */

void rhs( const double x , double &dxdt , const double t )
{
    dxdt = 3.0/(2.0*t*t) + x/(2.0*t);
}

void write_cout( const double &x , const double t )
{
    // Write 't' and 'x' to the file, separated by a tab
    data_file << t << '\t' << x << endl;
}

// state_type = double
typedef runge_kutta_dopri5< double > stepper_type;

int main()
{
    double x = 0.0;
    // We integrate from t=1.0 to t=10.0, with an initial step size of 0.1
    integrate_adaptive( make_controlled( 1E-12 , 1E-12 , stepper_type() ) ,
            rhs , x , 1.0 , 10.0 , 0.1 , write_cout );
    
    data_file.close(); // Don't forget to close the file
}