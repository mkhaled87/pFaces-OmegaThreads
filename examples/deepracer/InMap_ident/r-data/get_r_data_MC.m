clear;
clc;
addpath('../../');


% sample time
tau = 0.5;

% the quantizers used to construct the symbolic model
xq = [ 0.1,  0.1,  0.2,  0.3];
xl = [-1.8, -1.8, -3.2, -1.5]; 
xu = [ 1.8,  1.8,  3.2,  1.5];
uq = [ 0.20,  1.0];
ul = [-0.80, -3.0];
uu = [ 0.80,  3.0];

% the ranges
x3_vals = xl(3):xq(3):xu(3);
x4_vals = xl(4):xq(4):xu(4);
u1_vals = ul(1):uq(1):uu(1);
u2_vals = ul(2):uq(2):uu(2);

% we fix x and y as their change will not affect r
x1 = 0.0;
x2 = 0.0;

% the data aray to store the values
rmin_per_xu = zeros(length(x3_vals),length(x4_vals),length(u1_vals),length(u2_vals),4);
rmax_per_xu = zeros(length(x3_vals),length(x4_vals),length(u1_vals),length(u2_vals),4);

% number of MC simulations in in the grid cell
N_SIMULATIONS = 1000;

% init RNG/uniform
rng(0,'twister');

% for all (x3,x4,u1,u2)
x3_idx = 0;
x4_idx = 0;
u1_idx = 0;
u2_idx = 0;
for x3 = x3_vals
    x3_idx = x3_idx + 1;
    for x4 = x4_vals
        x4_idx = x4_idx + 1;
        for u1 = u1_vals
            u1_idx = u1_idx + 1;
            for u2 = u2_vals
                u2_idx = u2_idx + 1;
                
                % simulate the center
                [t, center_posts]=ode45(@deepracer_ode,[0 tau], [x1,x2,x3,x4], [],[u1,u2]);
                center_post = center_posts(end,:);
                
                % MC startting from the grid cell
                for n=1:N_SIMULATIONS
                    x = [ ...
                        rand_range(x1-xq(1)/2, x1+xq(1)/2), ...
                        rand_range(x2-xq(2)/2, x2+xq(2)/2), ...
                        rand_range(x3-xq(3)/2, x3+xq(3)/2), ...
                        rand_range(x4-xq(4)/2, x4+xq(4)/2), ...
                    ];
                    [t, xu_posts]=ode45(@deepracer_ode,[0 tau], x, [], [u1, u2]);
                    xu_post = xu_posts(end,:);
                    
                    % MC-based post
                    r = xu_post-center_post;
                    
                    for i=1:4
                        if r(i) < rmin_per_xu(x3_idx,x4_idx,u1_idx,u2_idx,i)
                            rmin_per_xu(x3_idx,x4_idx,u1_idx,u2_idx,i) = r(i);
                        end
                        if r(i) > rmax_per_xu(x3_idx,x4_idx,u1_idx,u2_idx,i)
                            rmax_per_xu(x3_idx,x4_idx,u1_idx,u2_idx,i) = r(i);
                        end
                    end
                end
            end
            u2_idx = 0;
        end
        u1_idx = 0;
    end
    x4_idx = 0;
end


function r=rand_range(l,u)
    r = (u-l).*rand(1,1) + l;
end
