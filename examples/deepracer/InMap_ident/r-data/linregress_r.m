clear;
close all;
clc;

%load data
load("0_0_x3_x4_u1_u2_to_r.mat");

% the ranges
x3_vals = xl(3):xq(3):xu(3);
x4_vals = xl(4):xq(4):xu(4);
u1_vals = ul(1):uq(1):uu(1);
u2_vals = ul(2):uq(2):uu(2);

% combine min/max tables into one for r
size_xu = size(rmax_per_xu,1)*size(rmax_per_xu,2)*size(rmax_per_xu,3)*size(rmax_per_xu,4);
data_table = zeros(size_xu,8);
data_table_idx = 1;
for i = 1:size(rmax_per_xu,1)
    for j = 1:size(rmax_per_xu,2)
        for k = 1:size(rmax_per_xu,3)
            for l = 1:size(rmax_per_xu,4)
                r = (rmax_per_xu(i,j,k,l,:)-rmin_per_xu(i,j,k,l,:))./2;
                
                data_table(data_table_idx,1) = x3_vals(i);
                data_table(data_table_idx,2) = x4_vals(j);
                data_table(data_table_idx,3) = u1_vals(k);
                data_table(data_table_idx,4) = u2_vals(l);
                data_table(data_table_idx,5) = r(1,1,1,1,1);
                data_table(data_table_idx,6) = r(1,1,1,1,2);
                data_table(data_table_idx,7) = r(1,1,1,1,3);
                data_table(data_table_idx,8) = r(1,1,1,1,4);
                
                data_table_idx = data_table_idx+1;
            end            
        end
    end
end

% show hiistograms
figure;
hist(data_table(:,5));
title('Histogram r1')
figure;
hist(data_table(:,6));
title('Histogram r2')
figure;
hist(data_table(:,7));
title('Histogram r3')
figure;
hist(data_table(:,8));
title('Histogram r4')


% build tables xu -> r
tbl_r1 = table(data_table(:,1),data_table(:,2),data_table(:,3),data_table(:,4),data_table(:,5), ...
    'VariableNames', {'x3','x4','u1','u2','r1'});
tbl_r2 = table(data_table(:,1),data_table(:,2),data_table(:,3),data_table(:,4),data_table(:,6), ...
    'VariableNames', {'x3','x4','u1','u2','r2'});
tbl_r3 = table(data_table(:,1),data_table(:,2),data_table(:,3),data_table(:,4),data_table(:,7), ...
    'VariableNames', {'x3','x4','u1','u2','r3'});
tbl_r4 = table(data_table(:,1),data_table(:,2),data_table(:,3),data_table(:,4),data_table(:,8), ...
    'VariableNames', {'x3','x4','u1','u2','r4'});


% linear regression
r1_mdl = fitlm(tbl_r1,'r1~1+x3+x4+u1+u2');
r2_mdl = fitlm(tbl_r2,'r2~1+x3+x4+u1+u2');
r3_mdl = fitlm(tbl_r3,'r3~1+x3+x4+u1+u2');
r4_mdl = fitlm(tbl_r4,'r4~1+x3+x4+u1+u2');



