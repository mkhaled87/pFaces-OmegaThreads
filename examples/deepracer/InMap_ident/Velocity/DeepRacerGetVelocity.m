clear;
close all;
clc
MotiveDataFile = './data/FW_70.csv';

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
    disp('Info: the followin data lines are excluded from the file: ')
    disp(invalid_lines)
    
    timed_matrix(invalid_lines,:) = [];
end

% compute
time = timed_matrix(:,1);
xyz = timed_matrix(:,2:4);
distances = [];
time_diffs = [];
velocities = [];
for i = 1:size(xyz,1)-1

    % space diff (x is in position 1, y is in position 3)
    end_xy = xyz(i+1,[1 3]);
    start_xy = xyz(i,[1 3]);
    distance = norm(end_xy - start_xy);
    distances = [distances distance];
    
    % time diff
    end_time = time(i+1,:);
    start_time = time(i,:);
    time_diff = end_time - start_time;
    time_diffs = [time_diffs time_diff];
    
    velocities = [velocities (distance/time_diff)];    
end

%figure;
%bar(time(2:end), distances);

figure;
plot(time(2:end), velocities);
title(MotiveDataFile);
xlabel('Time (s)');
ylabel('Speed (m/s)');


disp('Next: clean the signal velocities by rtrimming the delay and drop to get a nice step responce signal.')
disp('Then, use the tool pidTuner (step_time = time_diff) to get the constransts K and T for a one-pool sysetm.')
disp('Then, use the ode has a=-1/T, and b=K/T.')
