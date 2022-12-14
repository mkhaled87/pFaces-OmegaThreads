#!/bin/sh

cd examples/robot2d
pfaces -CG -d 1 -k omega@../../kernel-pack -cfg robot.cfg
cd ..