#!/usr/bin/gnuplot

set term pngcairo enhanced solid size 1280,720 fontscale 1

set datafile separator ","
set decimalsign ","

set grid
#set rmargin 10
#set lmargin 1
#set tmargin 1
#set bmargin 0.5
set xlabel "Tiempo [minutos]"
set ylabel "Tensión [V]"

dv = 3.3/4096 # LSB
tm = 0.01     # periodo de muestreo
g = 81        # ganancia


set ytics nomirror
set y2label "Temperatura [°C]"
set y2tics

datos_2 = "../captura_continua_28-Sep-2016.csv"
datos_3 = "../captura_continua_29-Sep-2016_200026.csv"
datos_4 = "../captura_continua_30-Sep-2016_193235.csv"

set xrange [0:270]
set yrange [0:3.24]
set y2range [0:1000]

set output "horno_ensayos_con_ajuste_3.png"
set title "Ensayos del horno - respuesta a escalón"
set key bottom right

# respuesta a un escalón

f(x) = A*(1 - exp(-a*(x*60 - t0)))

plot datos_2 using ($1/60):($2) title "escalón 110V - horno abierto" with points pt 7 lc rgb "red", \
     datos_3 using ($1/60):($2) title "escalón 50V - horno cerrado" with points pt 7 lc rgb "web-green", \
     datos_4 using ($1/60):($2) title "escalón 86V - horno cerrado" with points pt 7 lc rgb "blue", \
     A=2.2212, a=1/1292.15, t0=77.5, f(x) title "E_{max}=2,22V {/Symbol t}=22 min" with lines lc rgb "orange" lw 2, \
     A=1.5946, a=1/5221.85, t0=70.52, f(x) title "E_{max}=1,59V {/Symbol t}=87 min" with lines lc rgb "dark-green" lw 2, \
     A=2.8363, a=1/3185.88, t0=95.12, f(x) title "E_ {max}=2,84V {/Symbol t}=53 min" with lines lc rgb "purple" lw 2

