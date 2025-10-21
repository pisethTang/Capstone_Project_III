# Gnuplot script: plot_simple.gp

# Set output to a PNG file
set terminal pngcairo size 800,600
set output 'simple_ode_plot.png'

# Set labels and title
set title "Solution of x' = 3/(2t^2) + x/(2t)"
set xlabel "t"
set ylabel "x(t)"
set grid

# Plot the numerical solution from our file (column 1 vs 2)
# and also plot the analytic solution for comparison
plot 'simple_ode.dat' using 1:2 with points title 'Numerical (odeint)', \
     (sqrt(x) - 1/x) with lines title 'Analytics: sqrt(t) - 1/t'