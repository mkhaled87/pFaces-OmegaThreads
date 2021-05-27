clear;
close all;
clc

% source of dada
src = './data/';
addpath(src);

% configs
STEP_LEVEL = 1.0;
DIRECTION = 'P';
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
xy = timed_matrix(:,[2 4]);

% plots
figure;
plot(xy(1,1), xy(1,2), 'ok');
hold on;
plot(xy(:,1), xy(:,2), 'b');
title(strrep(MotiveDataFile, '_', '\_'));
xlabel('X');
ylabel('Y');
legend('Start')


% estimate the radius via nonlinear regression to a circle

