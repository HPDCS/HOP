set terminal epscairo font 'Linux-Libertine, 14'

set xlabel "Pages"

unset key
unset xtics

set auto x
set grid y
set yrange [0:*]

set style data histogram
set style fill solid border -1
set style histogram gap 1

set ylabel "Cumulative Accesses"
set output "bin.eps"

plot    'data.dat' u 2:xtic(1);

