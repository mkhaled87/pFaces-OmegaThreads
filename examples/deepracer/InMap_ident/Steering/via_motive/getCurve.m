Wheelbase = 0.165;
P = [0.05:0.05:1.0];
N = [0.00:0.05:1.0];

X = [P(end:-1:1) -1*N];
Y = [];
circlefun = @(b, X) (X(:,1).^2 + X(:,2).^2 + b(1)*X(:,1) + b(2)*X(:,2) + b(3));
beta0 = [0 0 0];

% source of dada
src = './data/';
addpath(src);

for i=1:length(X)    
    
    % prepare
    STEP_LEVEL = X(i);
    if STEP_LEVEL < 0
        DIRECTION = 'N';
    elseif STEP_LEVEL > 0
        DIRECTION = 'P';
    else
        DIRECTION = 'N';
    end
    if DIRECTION == 'N'
        STEP_LEVEL = STEP_LEVEL*-1;
    end
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
    
    % get R
    mdl = fitnlm(xy, zeros(size(xy,1),1),circlefun,beta0);
    B = mdl.Coefficients.Estimate;
    Xm = -B(1)/2;
    Ym = -B(2)/2;
    R = sqrt((Xm^2 + Ym^2) - B(3));
    if DIRECTION == 'N'
        R = R*-1;
    end
    
    % get Psi
    psi = atan(Wheelbase/R);
    Y= [Y psi];    
end

mdl = fit(X',Y','poly3')
plot(X,Y,'k');
hold on;


Y_fit = mdl.p1*X.^3 + mdl.p2*X.^2 + mdl.p3*X + mdl.p4;
plot(X,Y_fit,'b');




