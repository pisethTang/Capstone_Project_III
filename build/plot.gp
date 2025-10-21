# ---------------------------------
# Gnuplot Script: plot.gp
# ---------------------------------

# 1. Set the output format and file name
set terminal pngcairo size 800,600
set output 'lorenz_plot.png'

# 2. Set labels and styling
set title "Lorenz Attractor"
set xlabel "x-axis (x[0])"
set ylabel "z-axis (x[2])"
set size ratio -1  # Makes the aspect ratio 1:1

# 3. Plot the data
# 'using 2:4' means plot column 2 vs. column 4
# 'with lines' just draws the line
plot 'lorenz.dat' using 2:4 with lines title 'Lorenz System'

# 4. Clean up (optional but good practice)
unset output
unset terminal

print "Plot saved to lorenz_plot.png"