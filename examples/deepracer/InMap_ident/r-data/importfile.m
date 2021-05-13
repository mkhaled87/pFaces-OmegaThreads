function detmodelsimdata = importfile(filename, dataLines)
%IMPORTFILE Import data from a text file
%  DETMODELSIMDATA = IMPORTFILE(FILENAME) reads data from text file
%  FILENAME for the default selection.  Returns the data as a table.
%
%  DETMODELSIMDATA = IMPORTFILE(FILE, DATALINES) reads data for the
%  specified row interval(s) of text file FILENAME. Specify DATALINES as
%  a positive scalar integer or a N-by-2 array of positive scalar
%  integers for dis-contiguous row intervals.
%
%  Example:
%  detmodelsimdata = importfile("/Users/mk/Workspace/GitLab/pFaces-OmegaThreads/examples/deepracer/InMap_ident/r-data/det_model_simdata.csv", [2, Inf]);
%
%  See also READTABLE.
%
% Auto-generated by MATLAB on 13-May-2021 09:55:26

%% Input handling

% If dataLines is not specified, define defaults
if nargin < 2
    dataLines = [2, Inf];
end

%% Setup the Import Options and import the data
opts = delimitedTextImportOptions("NumVariables", 14);

% Specify range and delimiter
opts.DataLines = dataLines;
opts.Delimiter = ",";

% Specify column names and types
opts.VariableNames = ["Var1", "Var2", "x_post_0", "x_post_1", "x_post_2", "x_post_3", "HR_LB_0", "HR_LB_1", "HR_LB_2", "HR_LB_3", "Var11", "Var12", "Var13", "Var14"];
opts.SelectedVariableNames = ["x_post_0", "x_post_1", "x_post_2", "x_post_3", "HR_LB_0", "HR_LB_1", "HR_LB_2", "HR_LB_3"];
opts.VariableTypes = ["string", "string", "double", "double", "double", "double", "double", "double", "double", "double", "string", "string", "string", "string"];

% Specify file level properties
opts.ExtraColumnsRule = "ignore";
opts.EmptyLineRule = "read";

% Specify variable properties
opts = setvaropts(opts, ["Var1", "Var2", "Var11", "Var12", "Var13", "Var14"], "WhitespaceRule", "preserve");
opts = setvaropts(opts, ["Var1", "Var2", "Var11", "Var12", "Var13", "Var14"], "EmptyFieldRule", "auto");

% Import the data
detmodelsimdata = readtable(filename, opts);

end