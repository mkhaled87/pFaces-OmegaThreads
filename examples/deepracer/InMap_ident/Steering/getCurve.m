clear
clc

AWSDRInputMapIdent = importfile("AWS_DR_InputMapIdent.xlsx", "Steering Input Map", [8, 48]);

X = AWSDRInputMapIdent.min;
Y = AWSDRInputMapIdent.VarName5;

fit(X,Y,'poly3')
