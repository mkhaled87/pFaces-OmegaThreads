clear;
close all;
clc

% source of dada
src = './data/';
addpath(src);

% configs
STEP_LEVEL = 1.00;
DIRECTION = 'N';
MotiveDataFile = [src DIRECTION '_' num2str(STEP_LEVEL*100) '.csv'];

% inport the data file => gets a table [time,x,y,z] for marker1
data_table = ImportMotiveDataFile(MotiveDataFile);

% convert table to matrix
timed_matrix = data_table{:,:};

% remove any lines with missing data
invalid_lines = [];
for i = 1:size(timed_matrix,1)
    if isnan(timed_matrix(i,2))
        invalid_lines = [invalid_lines i];
    end
end
if ~isempty(invalid_lines)
    disp(['Info: ' num2str(length(invalid_lines)) ' data lines are excluded from the file: '])
    timed_matrix(invalid_lines,:) = [];
end

% collect
time = timed_matrix(:,1);
xy = timed_matrix(:,[4 2]);

% plots
figure;
plot(xy(1,1), xy(1,2), 'ok');
hold on;
plot(xy(:,1), xy(:,2), '*b');
title(strrep(MotiveDataFile, '_', '\_'));
xlabel('X');
ylabel('Y');



% estimate the radius via nonlinear regression to a circle
circlefun = @(b, X) (X(:,1).^2 + X(:,2).^2 + b(1)*X(:,1) + b(2)*X(:,2) + b(3));
beta0 = [0 0 0];
mdl = fitnlm(xy, zeros(size(xy,1),1),circlefun,beta0);
B = mdl.Coefficients.Estimate;
Xm = -B(1)/2;
Ym = -B(2)/2;
R = sqrt((Xm^2 + Ym^2) - B(3));
D = R*2;
plot(Xm, Ym, '*r');
text(Xm, Ym+0.05, ['R = ' num2str(R)])
plot([Xm Xm+R], [Ym Ym], 'r');
rectangle('Position', [(Xm-R) (Ym-R) D D], 'Curvature', [1,1], 'EdgeColor', 'green');
legend('Start','Data','Center','Radius')
sign = '+';
if DIRECTION == 'N'
    sign = '-';
end
disp(['R = ' sign num2str(R)])
disp(['D = ' sign num2str(D)])


