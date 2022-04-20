% @file rx_tx_skew_01.m
% @date 2021.09.10
% @info octave plot rcs-rx04-02 timing skew to tx01
% 
load("rx.dat");
% create single time series plot of tx to rx skew
figure(1);
t=1:1:length(rx);
t=t';
plot(t,rx);
title("tx->rx timing skew rcs-rx04-02 2021.09.10");
ylabel("uSeconds skew at same tx->rx distance of 1ft");
xlabel("4 second pulse intervals");
text(35,-250, "avg(diff(rx))) = -18.07 us/pulse");
grid;
