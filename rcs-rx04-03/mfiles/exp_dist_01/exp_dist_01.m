% @file exp_dist_01.m
% @date 2021.08.19
% @info octave plot rcs-rx04-01 distance experements 
% baseline datasets are doubled in length because code was only capturing uNsamp,
% instead of 2*uNsamp; code is corrected for future runs
% think d1=1ft, d2=2ft, d3=3ft, d4-4ft, d5=d6=5ft
load("b1.dat");
load("b2.dat");
load("b3.dat");
load("b4.dat");
load("b5.dat");
load("b6.dat");
load("d1.dat");
load("d2.dat");
load("d3.dat");
load("d4.dat");
load("d5.dat");
load("d6.dat");
b1=[b1;b2];  % because high b1 only at first capture
b2=[b2;b2];
b3=[b3;b3];
b4=[b4;b4];
b5=[b5;b5];
b6=[b6;b6];
t=1:1:length(b1);
t=t';

% individual plots
% --hold-- figure(1);
% --hold-- plot(t,b1,t,d1);
% --hold-- title("1 ft");
% --hold-- figure(2);
% --hold-- plot(t,b3,t,d3);
% --hold-- title("3 ft");
% --hold-- figure(3);
% --hold-- plot(t,b6,t,d6);
% --hold-- title("5 ft");

% all data grouped
% --hold-- figure(4);
% --hold-- bb=[b1;b2;b3;b4;b5;b6];
% --hold-- dd=[d1;d2;d3;d4;d5;d6];
% --hold-- tt=1:1:length(bb);
% --hold-- tt=tt';
% --hold-- plot(tt,bb,tt,dd);
% --hold-- title("tx-01 -> rx-04; 1ft - 5ft");

% well pointed data grouped
figure(5);
bb=[b1;b3;b6];
dd=[d1;d3;d6];
tt=1:1:length(bb);
tt=tt';
plot(tt,bb,tt,dd);
title("tx-01 -> rx-04; 1ft, 3ft, 5ft");

% plot diffs
figure(6);
tt=1:1:length(diff(bb));
tt=tt';
plot(tt,diff(bb),tt,diff(dd));
title("diffs tx-01 -> rx-04; 1ft, 3ft, 5ft");
