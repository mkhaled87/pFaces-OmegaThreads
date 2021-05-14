clear
clc

% Note: collec the data in "det_model_simdata.csv" using a determisintic
% cocnstructin of a symbolic model (i.e, r=0 for radius dynamics) whihc
% will resultin post-HR as single point obtained by solving the ode
% starting from the centers of the q-cells.

% the quantizers used to construct the symbolic model
q = [0.1,  0.1,  0.2,  0.3];

% get the collected simulation data
data = importfile("det_model_simdata.csv");

% xPosts from the ode solver of the simulation (each row is one post state)
xPosts_realworld = [data.x_post_0 data.x_post_1 data.x_post_2 data.x_post_3];

% xPosts from the construction of the symoblic mocel (i.e, solving the ode with thte center of tha q-cell)
xPosts_symmodel = [data.HR_LB_0, data.HR_LB_1, data.HR_LB_2, data.HR_LB_3];

% computer the deviation
xPosts_deviation = xPosts_realworld - xPosts_symmodel;

% fix the angle deviation to be wintin the pi-range
xPosts_deviation(:,3) = wrapToPi(xPosts_deviation(:,3));

% min and max
deviation_min = [min(xPosts_deviation(:,1)) min(xPosts_deviation(:,2)) min(xPosts_deviation(:,3)) min(xPosts_deviation(:,4))];
deviation_max = [max(xPosts_deviation(:,1)) max(xPosts_deviation(:,2)) max(xPosts_deviation(:,3)) max(xPosts_deviation(:,4))];

% print
disp('r_post_lb: ')
disp(deviation_min)
disp('r_post_ub: ')
disp(deviation_max)
disp(' => r_post should be: ')
disp(deviation_max-deviation_min)
disp('Use these values for r-dynamics in the symolic model construction.')




