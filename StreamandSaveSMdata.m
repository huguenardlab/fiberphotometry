%--------------------------------------------------------------------------
%StreamandSaveSMdata
%By: Brielle Ferguson
%Date: 1/10/18
%Script will take serial monitor data generated from the arduino
%"OperantTaskMasterScript" sketch during a behavioral session. The OperantTaskMasterScript
% sketch must be uploaded first through the Arduino IDE before running this 
% script in MATLAB.
%Assumptions:
% - Arduino sketch is already loaded
% - cd is set to the folder for saving data
% - path includes the helper functions cleanUpOperantData and sepCueLengthandAccuracy
%Inputs: 
% - animalID: string collected collected in input box
%Outputs: 
% - behData: a cell array with trial information about the behavioral
%   session. This can be analyzed with the function readAnalyzePlotBehData, which
%   is called at the end of the session to plot some summary data about the
%   behavioral session.
%--------------------------------------------------------------------------

%% set up serial port
clearvars
delete(instrfindall);%make sure no serial ports are already open
serialPort = 'COM5'; %the port the arduino is connected to
baudeRate = 115200; %make sure this matches the rate in the arduino script that you're streaming your serial monitor data
s = serial(serialPort,'Timeout', 5000, 'BaudRate', baudeRate, 'Terminator', 'LF');
s.ReadAsyncMode = 'continuous';
fopen(s);

%% pre-allocate variables to be populated in the loop
timeStamp = {};
formatOut = 'hh:mm:ss.fff';
behData = {};
index = 1;
running = true;

%% create figure asking user for filename and then indicating that the session is running

fig1 = figure('pos',[500 500 550 200]);

% filename edit
h1 = uicontrol(fig1,...
    'Style','edit',...
    'Position', [60 100 400 20]);

% filename prompt
h2 = uicontrol(fig1,...
    'Style','text',...
    'String', 'Filename:',...
    'Position', [10 100 50 20]);

% continue button
h3 = uicontrol(fig1, 'Style', 'pushbutton', 'String', 'continue', 'Callback', 'uiresume(gcbf);');
uiwait

baseFileName = h1.String;
textFileName = [h1.String '.txt'];

if exist(fullfile(cd,baseFileName),'file')
    warning([baseFileName ' already exists and will be overwritten in this session.'])
end

close(fig1);

%figure indicating that MATLAB is collecting Arduino data. User can click end to
%end the session 
fig2 = figure('pos',[847 500 296 200]);

h4 = uicontrol(fig2,...
    'Style','text',...
    'String', 'Collecting Serial Monitor Data...',...
    'Position', [58 35 184 124], 'FontSize', 15);
h5 = uicontrol(fig2, 'Style', 'pushbutton', 'String', 'End Session', 'Position', [89 60 117 20],'Callback', 'delete(gcbf);');

%% display and save serial monitor data until the user ends the session         
while true
    drawnow
    
    if  (s.BytesAvailable > 0)
        %read event and time information from the arduino
        event = fgetl(s); 
        timeString = char(datetime('now','Format', 'HH:mm:ss.SSS'));
        behData{index} = [timeString, ' -> ', event];  
        
        %stream data to the command window
        disp(behData{index});
        index = index + 1;
    end
    
    %check to see if user has clicked "End Session" button
    if ~ishandle(h5)
        disp('End Session');
        break;
    end
end

%% save and  plot data
behData = transpose(behData);
save behData behData baseFileName;
delete(instrfindall);%close the serial port

%plot behavioral data 
figure;[summary, trials] = readAnalyzePlotBehData(behData, baseFileName);




