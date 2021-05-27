clear;
close all;
clc

% source of dada
src = './data_newbattery/';
addpath(src);

% configs
STEP_LEVEL = 0.45;
DIRECTION = 'FW';
MotiveDataFile = [src DIRECTION '_' num2str(STEP_LEVEL*100) '.csv'];
trim_initial_delay = true;
trim_threshold = 0.04;
trim_break_response = true;
sfhv_window_sec = 0.1; %sfhv = Search for Higher Value

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

% trimming
org_velocities = velocities;
org_time = time;
if trim_initial_delay
    above_threshhold_idxs = find((velocities < trim_threshold) == 0);
    trim_idx = above_threshhold_idxs(1);
    velocities = velocities(trim_idx:end);
    time = time(trim_idx:end);
    time = time - time(1);
end
if trim_break_response
    sfhv_window = floor(sfhv_window_sec/time_diff);
    flipped_velocities = velocities(end:-1:1);
    for i = 1:length(flipped_velocities)
        found_higher = false;
        for f = i:i+sfhv_window
            if flipped_velocities(f) > flipped_velocities(i)
                found_higher = true;
                break;
            end
        end
        if ~found_higher
            trim_idx = length(flipped_velocities) - i;
            break;
        end
    end
    velocities = velocities(1:trim_idx);
    time = time(1:trim_idx+1);
end

% plots
%figure;
%bar(time(2:end), distances);
figure;
plot(org_time(2:end), org_velocities, 'k')
hold on;
plot(time(2:end), velocities, 'b');
title(strrep(MotiveDataFile, '_', '\_'));
xlabel('Time (s)');
ylabel('Forward Velocity (m/s)');
legend('Original','Trimmed')


disp('Next: clean the signal (velocities) by trimming the initial_delay if not trimmed to get a nice step-responce signal.')
disp('Then, use the tool (pidTuner, step_time = time_diff, in=STEP_LEVEL, out=velocities) to get the constransts K and T')
disp('for a one-pool system. Then, we have an ODE of the system (dot_v = av + bu) where (a := -1/T, and b := K/T).')
