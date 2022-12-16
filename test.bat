@ECHO OFF

rem Testing example: pickupdelivery
cd examples\pickupdelivery
pfaces -CG -k omega@..\..\kernel-pack -cfg .\pickupdelivery.cfg -d 1 -p
cd ..\..
