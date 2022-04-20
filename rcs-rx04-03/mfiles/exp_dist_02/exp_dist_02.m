% @file exp_dist_02.m
% @date 2021.08.19
% @info octave plot rcs-rx04-01 distance experements: 3,6,9,12,15,18(x2) ft
% 
load("b21.dat");
load("b22.dat");
load("b23.dat");
load("b24.dat");
load("b25.dat");
load("b26.dat");
load("b27.dat");
load("b28.dat");
load("d21.dat");
load("d22.dat");
load("d23.dat");
load("d24.dat");
load("d25.dat");
load("d26.dat");
load("d27.dat");
load("d28.dat");

% create single time series plot of baseline and data for 3,6,9,12,15,18,18,18,18
figure(1);
bb=[b21;b22;b23;b24;b25;b26;b27;b28];
dd=[d21;d22;d23;d24;d25;d26;d27;d28];
tt=1:1:length(bb);
tt=tt';
plot(tt,bb,tt,dd);
title("distance experiment 2: tx-01 -> rx-04; 3,6,9,12,15,18,18,18ft");
ylabel("Volts")
xlabel("adc sample")

