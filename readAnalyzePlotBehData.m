%--------------------------------------------------------------------------
%readAnalyzePlotBehData
%By: Brielle Ferguson
%Date: 1/10/18
%Function that will take serial monitor data collected from the arduino
%"OperantTaskMasterScript" sketch. It will read in that
%data, and then plot it onto a figure. The graph 
%will use the animalID the user has provided, and then will also display
% descriptive information read from the data (session length, left, 
% right, correct and incorrect trials). The plot will show trial
% initiation, and left and right choices over time. Correct choices will be in 
%green, while incorrect will be in red.
%Assumptions:
% - the variable behData is already loaded into matlab
%Inputs:
% - data: cell array of text data from arduino serial monitor
% - animalID: string of animal ID
% - optoStim: boolean, true if function should search for opto trials in data
%Outputs:
% - summary: structure with accuracy at each cue length, omissions, and other
%   descriptives about session
% - trials: structure with each trial including the fields, trial number,
%   accuracy, choice, cue length, response time, and opto.
%--------------------------------------------------------------------------


function [summary, trials] = readAnalyzePlotBehData(data, animalID, optoStim)
%% Set up processing, plotting, and analysis parameters

%pre-allocating variables for analysis
portEntry = {};
correctTrials = {};
incorrectTrials = {};
accuracy = []; 

%plotting data
noPlots = false; 
plotAllTrials = true; %create plot of all trials and descriptive session information  
plotToExternalFigure = false; %set to true if will be calling repeatedly from an outside script to generate multiple subplots

%include omissions in determing accuracy
calculateOmissions = true; %trials where animal takes longer than omissionThreshold to make choice will count as omissions
omissionThreshold = 5; %in seconds
numTrialTypes = 5; %number of cues or delays

%cue parameters
cueLengthVariable = true; %for AET set to true 
cueLength = 1; %if cueLengthVariable = false, will use this value to analyze data
numCues = 5; %Will be 5 (5s, 2s, 1s, 500ms, or 100ms) or 3 (5s, 2s, 100ms)
cueSet1 = true; %use for 5s, 2s, 500ms
cueSet2 = false; %use for 5s, 2s, 100ms
plotCueLengthData = true; %will plot accuracy at each cue length if cueLengthVariable = true

%delay parameters (still some bugs here, working on fixing)
delayLength = 0; %if delayLengthVariable = false, will use this value to analyze data 
delayLengthVariable = false; %set to true if used a variable delay during task
plotDelayLengthData = true; %will plot accuracy at each delay length if delayLengthVariable = true

%crossmodal analysis parameters
crossmodalTrials = false; %true if using visual and auditory trials

%opto parameters, if optoStim = true, will use these variables for plotting
%and analysis
optoStimThisTrial = false; %variable will be updated as matlab reads each trial, don't need to update
optoStimBlocks  = false; %set to true if opto stim is in fixed length blocks
optoStimRandom = true; %set to true if opto stim occurs randomly
plotOptoData = true; %will generate plots of accuracy in opto vs no opto trials

%% clean up data file, find time stamps, choices, and accuracy for each trial

%get rid of any extra trials, returns to center port, and the clean
%ports trials. 
[data] = cleanUpOperantData(data);

%pull out time of each entry in the log 
for i = 1: length(data)
string = char(data(i));
stringTime = string(1:12);
timeString = datetime(stringTime, 'Format', 'HH:mm:ss.SSS');
smTime(i) = timeString;     
end

%find all the choices 
for i = 1: length(data)
string = char(data(i));
portEntry(i) = {string(17)};
end

%find whether the choice was correct or incorrect 
for i = 1: length(data)
stringAccuracy = char(data(i));
    if contains(stringAccuracy, ' Correct')
    correctTrials(i) = {stringAccuracy(17)};
    end
    if contains(stringAccuracy, ' Incorrect')
    incorrectTrials(i) = {stringAccuracy(17)};
    end        
end
    
%% creating indexes of correct and incorrect choices with side information 
    
%at this point have an array the size of all the choices, empty except for 
%where there was a choice, either correct or incorrect. This will find each
%of those positions and save the index into a new array. 
correctTrialIndex = find(~cellfun(@isempty,correctTrials));
incorrectTrialIndex = find(~cellfun(@isempty,incorrectTrials));

%create index of all the trials  
allTrialIndex = sort([correctTrialIndex incorrectTrialIndex]);

%take those correct and incorrect indexes and separate them into left and right choices. 
leftCorrect = correctTrialIndex(all(ismember(char(portEntry(correctTrialIndex)),'L'),2));
rightCorrect = correctTrialIndex(all(ismember(char(portEntry(correctTrialIndex)),'R'),2));

leftIncorrect = incorrectTrialIndex(all(ismember(char(portEntry(incorrectTrialIndex)),'L'),2));
rightIncorrect = incorrectTrialIndex(all(ismember(char(portEntry(incorrectTrialIndex)),'R'),2));

%will calculate the percentage of correct total trials of all types  
percentCorrect = (length(correctTrialIndex)/(length(correctTrialIndex) + length(incorrectTrialIndex))) * 100;

%determine the total amount of choices made on each side. 
numLeftChoices = length(leftCorrect) + length(leftIncorrect);
numRightChoices = length(rightCorrect) + length(rightIncorrect);

%find the start of all trials and get sum to get the total number of
%trials
trialAvailable = find(all(ismember(char(portEntry),'T'),2));
numTrials = length(trialAvailable); 

%find the trial initiation point (always the next point in array after the trial is available)
centerChoices = trialAvailable + 1;

%useful if the animal has multiple sessions or had to restart the session
beginSession = find(all(ismember(char(portEntry), 'B'), 2)); 

%% calculate time and speed information about the session

%convert time data from time of day to absolute time elapsed from start and
%convert to seconds
smTime = smTime - smTime(1); 
smTime.Format = 's';

%find beginning and end of the session and total session length in seconds
%and minutes 
beginTime = smTime(1);
endTime = smTime(end) + minutes(1);
sessionLength = smTime(end) - smTime(1);
[h,m,~] = hms(sessionLength);
sessionMinutes = (h * 60) + m; 

%find amount of trials per minute 
speed = numTrials/sessionMinutes; 

%% create a structure with information about each trial. This is useful for analyzing with any other data that was collected during the behavioral session, fiber photometry, EEG, etc.  

for i = 1:length(allTrialIndex) 
   trials(i).number = i; 

   
   %whether the choice was correct and which side
   if ismember(allTrialIndex(i),correctTrialIndex) 
        trials(i).accuracy = 'Correct';
        
        if ismember(allTrialIndex(i),leftCorrect)
            trials(i).choice = 'L';
        else
            trials(i).choice = 'R';
        end
        
   else %whether the choice was incorrect and what side 
        trials(i).accuracy = 'Incorrect'; 
        
        if ismember(allTrialIndex(i),leftIncorrect)
            trials(i).choice = 'L';
        else
            trials(i).choice = 'R';
        end        
   end     
end


if ~cueLengthVariable && ~delayLengthVariable
%update trial structure with response time  
for i = 1:length(allTrialIndex) 
    
   %will find the time between the trial initiation and the choice
   %once cue length is determined will account for the cue time
   initiationTime = smTime(centerChoices(i));
   responseTime = smTime(allTrialIndex(i));
   s = seconds(responseTime - initiationTime); %convert from duration to double
   
   trials(i).responseTime = s - (cueLength + delayLength);
   
     
end

end



%% plot all trials 

%plot all trials and accuracy of with descriptive information about the session  

if plotToExternalFigure == false && noPlots == false
    figure;
end

if plotAllTrials == true && crossmodalTrials == false

%set x axis as the length of the session, and plot it as blank so the other
%data has values it can be plotted it against 
xAxis = zeros(size(smTime));
plot(smTime, xAxis, 'Linestyle', 'none') 
hold on 

%add lines to demarcate if the session mode was changed or there was a
%break between sessions in that day 
for i = 1: length(beginSession)
line([smTime(beginSession(i)) smTime(beginSession(i))], [-2 2]);
end

%plot all the left choices 
plot(smTime(leftIncorrect), 1.5 * ones(size(leftIncorrect)), 'o', 'color', 'r');
plot(smTime(leftCorrect), ones(size(leftCorrect)), 'o', 'color', 'g');

%plot all the trial availability and center choices to initiate trials
plot(smTime(centerChoices), zeros(size(centerChoices)), '>', 'color', 'k');

%plot all of the right choices
plot(smTime(rightIncorrect), -1.5 * (ones(size(rightIncorrect))), 'o', 'color', 'r');
plot(smTime(rightCorrect), -1 * ones(size(rightCorrect)), 'o', 'color', 'g');

%plot title, descriptive data, and set axes. 
title({[animalID, ', Session Length: ' num2str(sessionMinutes) 'min' '        Trials: ' num2str(numTrials) '      %Correct:  ' num2str(percentCorrect)], ['Correct: ' num2str(length(correctTrialIndex)) '     Incorrect: ' num2str(length(incorrectTrialIndex)) '      Left: ' num2str(numLeftChoices) '     Right: ' num2str(numRightChoices)]}, 'FontSize', 9) 
set(gca,'ylim',[-2 2]);
set(gca, 'xlim', [beginTime endTime])
yticks([-1 0 1])
yticklabels({'right', 'center', 'left'})

elseif plotAllTrials == true && crossmodalTrials == true
%will fill arrays with the choice made at each type of trial for each cue
%modality
dataAuditory = {}; 
dataVisual = {}; 

for i = 1: length(data) - 3 %will loop through the data until the last available trial
    stringTrialModality = char(data(i));
    stringChoice = char(data(i+2)); 

    if contains(stringTrialModality, ' Auditory') 
        dataAuditory(i+2) = {stringChoice(17)};
    end
    
    if contains(stringTrialModality, ' Visual') 
        dataVisual(i+2) = {stringChoice(17)};   
    end
    
end
        
%will find the positions of the trials of each cue modality
dataAuditoryIndex = find(~cellfun(@isempty,dataAuditory));
dataVisualIndex = find(~cellfun(@isempty,dataVisual));

%will take the indexes of each trial type and see which were correct or
%incorrect by comparing to the correct and incorrect trial index. 
leftCorrectA = dataAuditoryIndex(ismember(dataAuditoryIndex,leftCorrect));
leftIncorrectA = dataAuditoryIndex(ismember(dataAuditoryIndex,leftIncorrect));

rightCorrectA = dataAuditoryIndex(ismember(dataAuditoryIndex,rightCorrect));
rightIncorrectA = dataAuditoryIndex(ismember(dataAuditoryIndex,rightIncorrect));

leftCorrectV = dataVisualIndex(ismember(dataVisualIndex,leftCorrect));
leftIncorrectV = dataVisualIndex(ismember(dataVisualIndex,leftIncorrect));

rightCorrectV = dataVisualIndex(ismember(dataVisualIndex,rightCorrect));
rightIncorrectV = dataVisualIndex(ismember(dataVisualIndex,rightIncorrect));
        
        
%set x axis as the length of the session, and plot it as blank so the other
%data has values it can be plotted it against 
xAxis = zeros(size(smTime));
plot(smTime, xAxis, 'Linestyle', 'none') 
hold on 

%add lines to demarcate if the session mode was changed or there was a
%break between sessions in that day 
for i = 1: length(beginSession)
line([smTime(beginSession(i)) smTime(beginSession(i))], [-2 2]);
end

%plot all the left choices 
plot(smTime(leftIncorrectV), 1.5 * ones(size(leftIncorrectV)), 'o', 'color', 'r'); %visual 
plot(smTime(leftCorrectV), ones(size(leftCorrectV)), 'o', 'color', 'g');
plot(smTime(leftIncorrectA), 1.5 * ones(size(leftIncorrectA)), '^', 'color', 'r'); %auditory
plot(smTime(leftCorrectA), ones(size(leftCorrectA)), '^', 'color', 'g');

%plot all the trial availability and center choices to initiate trials
plot(smTime(centerChoices), zeros(size(centerChoices)), '>', 'color', 'k');

%plot all of the right choices
plot(smTime(rightIncorrectV), -1.5 * (ones(size(rightIncorrectV))), 'o', 'color', 'r'); %visual
plot(smTime(rightCorrectV), -1 * ones(size(rightCorrectV)), 'o', 'color', 'g');
plot(smTime(rightIncorrectA), -1.5 * (ones(size(rightIncorrectA))), '^', 'color', 'r'); %auditory
plot(smTime(rightCorrectA), -1 * ones(size(rightCorrectA)), '^', 'color', 'g');

%plot title and descriptive data, and set axes. 
title({[animalID, ', Session Length: ' num2str(sessionMinutes) 'min' '        Trials: ' num2str(numTrials) '      %Correct:  ' num2str(percentCorrect)], ['Correct: ' num2str(length(correctTrialIndex)) '     Incorrect: ' num2str(length(incorrectTrialIndex)) '      Left: ' num2str(numLeftChoices) '     Right: ' num2str(numRightChoices)]}, 'FontSize', 9) 
set(gca,'ylim',[-2 2]);
set(gca, 'xlim', [beginTime endTime])
yticks([-1 0 1])
yticklabels({'right', 'center', 'left'})     


%update summary structure with crossmodal session trial information 
summary.percentCorrectA = ((length(leftCorrectA)+length(rightCorrectA))/(length(leftCorrectA) + length(leftIncorrectA) + length(rightCorrectA) + length(rightIncorrectA))) * 100;
summary.percentCorrectV = ((length(leftCorrectV)+length(rightCorrectV))/(length(leftCorrectV) + length(leftIncorrectV) + length(rightCorrectV) + length(rightIncorrectV))) * 100;


end
%% if cue length is variable will extract data about cue lengths, accuracy, and choices

if cueLengthVariable == true

data100ms = {};
data500ms = {};
data1000ms = {};
data2000ms = {};
data5000ms = {}; 
dataOpto = {};

%will fill arrays with the choice made at each type of trial of the various
%cue lengths listed below. 
for i = 1: length(data) - 3 %will loop through the data until the last available trial
    stringCueLength = char(data(i));
    if optoStimRandom
        stringChoice = char(data(i+2:i+3)); 
        if contains(stringChoice(1,:), 'Optogenetic')
        optoStimThisTrial = true; 
        stringChoice = stringChoice(2,:);
        else
        optoStimThisTrial = false;
            stringChoice = stringChoice(1,:);
        end        
    else
        stringChoice = char(data(i+2)); 
    end
    
%search through trial for each cue length
    if contains(stringCueLength, ' 100') && ~contains(stringCueLength, ' 1000') && ~contains(stringCueLength, ' Reward') && ~contains(stringCueLength, ' Completed')                  
        if optoStimThisTrial
            dataOpto(i+2) = {'true'};
            data100ms(i+3) = {stringChoice(17)}; 
        else
            dataOpto(i+2) = {'false'};  
            data100ms(i+2) = {stringChoice(17)}; 
        end
    end
    
    if contains(stringCueLength, ' 500') && ~contains(stringCueLength, ' 5000') && ~contains(stringCueLength, ' Reward') 
        if optoStimThisTrial
            dataOpto(i+2) = {'true'};
            data500ms(i+3) = {stringChoice(17)};
        else
            dataOpto(i+2) = {'false'};    
            data500ms(i+2) = {stringChoice(17)};
        end   
    end
    
    if contains(stringCueLength, ' 1000') && ~contains(stringCueLength, ' Reward') 
        if optoStimThisTrial
            dataOpto(i+2) = {'true'};
            data1000ms(i+3) = {stringChoice(17)};
        else
            dataOpto(i+2) = {'false'};    
            data1000ms(i+2) = {stringChoice(17)};
        end   
    end
    
    if contains(stringCueLength, ' 2000') && ~contains(stringCueLength, ' Reward') 
        if optoStimThisTrial
            dataOpto(i+2) = {'true'};
            data2000ms(i+3) = {stringChoice(17)};
        else
            dataOpto(i+2) = {'false'}; 
            data2000ms(i+2) = {stringChoice(17)};
        end   
    end
    
    if contains(stringCueLength, ' 5000') && ~contains(stringCueLength, ' Reward') 
        if optoStimThisTrial
            dataOpto(i+2) = {'true'};
            data5000ms(i+3) = {stringChoice(17)};
        else
            dataOpto(i+2) = {'false'}; 
            data5000ms(i+2) = {stringChoice(17)};
        end   
    
    end

end

%will find the positions of the trials of each cue length
data100msIndex = find(~cellfun(@isempty,data100ms));
data500msIndex = find(~cellfun(@isempty,data500ms));
data1000msIndex = find(~cellfun(@isempty,data1000ms));
data2000msIndex = find(~cellfun(@isempty,data2000ms));
data5000msIndex = find(~cellfun(@isempty,data5000ms));
if optoStimRandom 
    dataOptoIndex = find(~cellfun(@isempty,dataOpto));
    dataOpto = dataOpto(dataOptoIndex);
end


%will take the indexes of each trial type and see which were correct or
%incorrect by comparing to the correct and incorrect trial index. 
correct100ms = data100msIndex(ismember(data100msIndex,correctTrialIndex));
incorrect100ms = data100msIndex(~ismember(data100msIndex,correctTrialIndex));

correct500ms = data500msIndex(ismember(data500msIndex,correctTrialIndex));
incorrect500ms = data500msIndex(~ismember(data500msIndex,correctTrialIndex));

correct1000ms = data1000msIndex(ismember(data1000msIndex,correctTrialIndex));
incorrect1000ms = data1000msIndex(~ismember(data1000msIndex,correctTrialIndex));

correct2000ms = data2000msIndex(ismember(data2000msIndex,correctTrialIndex));
incorrect2000ms = data2000msIndex(~ismember(data2000msIndex,correctTrialIndex));

correct5000ms = data5000msIndex(ismember(data5000msIndex,correctTrialIndex));
incorrect5000ms = data5000msIndex(~ismember(data5000msIndex,correctTrialIndex));

%will calculate the number of trials of each type 
numTrials100ms = length(data100msIndex); 
numTrials500ms = length(data500msIndex);
numTrials1000ms = length(data1000msIndex);
numTrials2000ms = length(data2000msIndex);
numTrials5000ms = length(data5000msIndex);

%will calculate the percent correct to each trial type 
percentCorrect100ms = 100* (length(correct100ms)/(length(correct100ms) + length(incorrect100ms))); 
percentCorrect500ms = 100 * (length(correct500ms)/(length(correct500ms) + length(incorrect500ms))); 
percentCorrect1000ms = 100 * (length(correct1000ms)/(length(correct1000ms) + length(incorrect1000ms))); 
percentCorrect2000ms = 100 * (length(correct2000ms)/(length(correct2000ms) + length(incorrect2000ms))); 
percentCorrect5000ms = 100 * (length(correct5000ms)/(length(correct5000ms) + length(incorrect5000ms))); 

%will create an accuracy table of performance at each cue length
if numCues == 5
accuracy = [percentCorrect5000ms percentCorrect2000ms percentCorrect1000ms percentCorrect500ms percentCorrect100ms]; 
end

if numCues == 3 && cueSet1
accuracy = [percentCorrect5000ms percentCorrect2000ms percentCorrect500ms]; 
end

if numCues == 3 && cueSet2
accuracy = [percentCorrect5000ms percentCorrect2000ms percentCorrect100ms]; 
end

%if optoStim is true, will get rid of any extra trials that don't have
%corresponding trial information. Placed here to catch a bug, but can't remember
%exactly why. May be that if analyzing blocks of autostim the code rounds up the
%size of a pre-defined block that may be greater than where the user stopped the
%behavioral session
if optoStim
    if length(allTrialIndex) > length(dataOpto)
        dataOpto{length(allTrialIndex)} = 'false';
    end
end

%update trial structure with cue information 
for i = 1:length(allTrialIndex) 
    
   %will find the time between the trial initiation and the choice
   %once cue length is determined will account for the cue time
   trials(i).initiationTime = seconds(smTime(centerChoices(i))); 
   responseTime = smTime(allTrialIndex(i)); %time after trial inititiation that animal makes a response
   s = seconds(responseTime) - trials(i).initiationTime; %convert from duration to double
   
   if ismember(allTrialIndex(i),data100msIndex)
        trials(i).cuelength = 100;
        trials(i).responseTime = s - .100;
   elseif ismember(allTrialIndex(i),data500msIndex)
        trials(i).cuelength = 500;
        trials(i).responseTime = s - .500;
   elseif ismember(allTrialIndex(i),data1000msIndex)
        trials(i).cuelength = 1000;
        trials(i).responseTime = s - 1;
   elseif ismember(allTrialIndex(i),data2000msIndex)
        trials(i).cuelength = 2000;
        trials(i).responseTime = s - 2;
   elseif ismember(allTrialIndex(i),data5000msIndex)
        trials(i).cuelength = 5000;   
        trials(i).responseTime = s - 5;
   end    
   
   if optoStim
       
   if contains(char(dataOpto(i)),'true')
      trials(i).opto =  'true';
   else
       trials(i).opto = 'false';
   end
   end
     
end

%pull accuracy values table and convert to string so can be plotted on graph
for i = 1: numCues 
    currentDataPoint = num2str(accuracy(i)); 
    dataPointsA(i) = {currentDataPoint};
end

%plot line graph with accuracy at each cue length
if plotCueLengthData == true
    figure; 
    plot(accuracy, '-o');
    if numCues == 3
        xAxis = 1:3;
        xticks(xAxis);
        xticklabels({'5s', '2s', '500ms'});  
    end
    if numCues == 5
        xAxis = 1:5;
        xticks(xAxis);
        xticklabels({'5s', '2s', '1s', '500ms', '100ms'});  
    end
    yticks([25 50 75 100]);
    yAxis = accuracy + 3;
    text(xAxis, yAxis, dataPointsA); 
    yline(50, '--k');
    title([ 'Accuracy,  ', animalID ]);
    ylabel('Percent correct');
    set(gca,'ylim',[20 120]);
end

%add accuracy values to summary structure
summary.accuracy = accuracy; 

end

%% will only execute if the delay length is variable (delayLengthVariable defined as true). 

if delayLengthVariable == true

data3000ms = {};
data4000ms = {};
data5000ms = {};
dataOpto = {};

%will fill arrays with the choice made at each type of trial of the various
%cue lengths listed below. 
for i = 1: length(data) - 3 %will loop through the data until the last available trial
    stringCueLength = char(data(i));
    if optoStimRandom
        stringChoice = char(data(i+2:i+3)); 
        if contains(stringChoice(1,:), 'Optogenetic')
        optoStimThisTrial = true; 
            %dataOpto(i+2) = {'true'};
        stringChoice = stringChoice(2,:);
        else
        optoStimThisTrial = false;
            %dataOpto(i+2) = {'false'};
            stringChoice = stringChoice(1,:);
        end        
    else
        stringChoice = char(data(i+2)); 
    end
    

    if contains(stringCueLength, ' 3000') && ~contains(stringCueLength, ' Reward') && ~contains(stringCueLength, ' Completed')                  
        if optoStimThisTrial
            dataOpto(i+2) = {'true'};
            data3000ms(i+3) = {stringChoice(17)}; 
        else
            dataOpto(i+2) = {'false'};  
            data3000ms(i+2) = {stringChoice(17)}; 
        end
    end
    
    if contains(stringCueLength, ' 4000') && ~contains(stringCueLength, ' Reward') 
        if optoStimThisTrial
            dataOpto(i+2) = {'true'};
            data4000ms(i+3) = {stringChoice(17)};
        else
            dataOpto(i+2) = {'false'};    
            data4000ms(i+2) = {stringChoice(17)};
        end   
    end
    
    if contains(stringCueLength, ' 5000') && ~contains(stringCueLength, ' Reward') 
        if optoStimThisTrial
            dataOpto(i+2) = {'true'};
            data5000ms(i+3) = {stringChoice(17)};
        else
            dataOpto(i+2) = {'false'};    
            data5000ms(i+2) = {stringChoice(17)};
        end   
    end   

end

%will find the positions of the trials of each cue length
data3000msIndex = find(~cellfun(@isempty,data3000ms));
data4000msIndex = find(~cellfun(@isempty,data4000ms));
data5000msIndex = find(~cellfun(@isempty,data5000ms));

if optoStimRandom 
    dataOptoIndex = find(~cellfun(@isempty,dataOpto));
    dataOpto = dataOpto(dataOptoIndex);
end


%will take the indexes of each trial type and see which were correct or
%incorrect by comparing to the correct and incorrect trial index. 
correct3000ms = data3000msIndex(ismember(data3000msIndex,correctTrialIndex));
incorrect3000ms = data3000msIndex(~ismember(data3000msIndex,correctTrialIndex));

correct4000ms = data4000msIndex(ismember(data4000msIndex,correctTrialIndex));
incorrect4000ms = data4000msIndex(~ismember(data4000msIndex,correctTrialIndex));

correct5000ms = data5000msIndex(ismember(data5000msIndex,correctTrialIndex));
incorrect5000ms = data5000msIndex(~ismember(data5000msIndex,correctTrialIndex));


%will calculate the number of trials of each type 
numTrials3000ms = length(data3000msIndex);
numTrials4000ms = length(data4000msIndex);
numTrials5000ms = length(data5000msIndex);

%will calculate the percent correct to each trial type 
percentCorrect3000ms = 100 * (length(correct3000ms)/(length(correct3000ms) + length(incorrect3000ms))); 
percentCorrect4000ms = 100 * (length(correct4000ms)/(length(correct4000ms) + length(incorrect4000ms))); 
percentCorrect5000ms = 100 * (length(correct5000ms)/(length(correct5000ms) + length(incorrect5000ms))); 

%place accuracy values into an array
accuracy = [percentCorrect3000ms percentCorrect4000ms percentCorrect5000ms]; 

if optoStim
    if length(allTrialIndex) > length(dataOpto)
        dataOpto{length(allTrialIndex)} = 'false';
    end
end
%update trial structure with cue information 
for i = 1:length(allTrialIndex) 
    
   %will find the time between the trial initiation and the choice
   %once cue length is determined will account for the cue time
   trials(i).initiationTime = seconds(smTime(centerChoices(i))); 
   responseTime = smTime(allTrialIndex(i)); %time after trial inititiation that animal makes a response
   s = seconds(responseTime) - trials(i).initiationTime; %convert from duration to double
   
   if ismember(allTrialIndex(i),data3000msIndex)
        trials(i).delayLength = 3000;
        trials(i).responseTime = s - 3;
   elseif ismember(allTrialIndex(i),data4000msIndex)
        trials(i).delayLength = 4000;
        trials(i).responseTime = s - 4;
   elseif ismember(allTrialIndex(i),data5000msIndex)
        trials(i).delayLength = 5000;   
        trials(i).responseTime = s - 5;
   end    
   
   if optoStim
       
   if contains(char(dataOpto(i)),'true')
      trials(i).opto =  'true';
   else
       trials(i).opto = 'false';
   end
   end
     
end

for i = 1: length(accuracy) 
    currentDataPoint = num2str(accuracy(i)); 
    dataPointsA(i) = {currentDataPoint};
end

if plotDelayLengthData == true
    figure; 
    %subplot(3,1,2);
    plot(accuracy, '-o');

    xAxis = 1:3;
    xticks(xAxis);
    xticklabels({'3s', '4s', '5s'});  

    yticks([25 50 75 100]);
    yAxis = accuracy + 3;
    text(xAxis, yAxis, dataPointsA); 
    yline(50, '--k');
    title([ 'Accuracy,  ', animalID ]);
    ylabel('Percent correct');
    set(gca,'ylim',[20 120]);
end

summary.accuracy = accuracy; 

end


%% %% will only execute if there is optogenetic stim (optoStim is
% defined as true above). 
 if optoStim
 if optoStimBlocks
     %create an index for the number of trials of 5 alternating opto off
     %opto on trials
    blockLength = 5; 
    i = 1;
    optoIndex = [];
    while i < length(trials)   
        if (i+5) < length(trials)    
            optoIndex(i:(i+blockLength)) = 0;    
            i = i+blockLength;
        else
            numToAdd = length(trials) - i;
            optoIndex(i:(i+numToAdd)) = 0;    
            i = i+numToAdd;
            break
        end
        if (i+5) < length(trials)    
            optoIndex(i:(i+blockLength)) = 1;    
            i = i+blockLength;
        else
            numToAdd = length(trials) - i;
            optoIndex(i:(i+numToAdd)) = 1;    
            i = i+numToAdd;
            break
        end
          
    end
    
    
    for i = 1:length(optoIndex)
        if optoIndex(i) == 0
            optoOff(i) = trials(i);
        end
        if optoIndex(i) == 1 
            optoOn(i) = trials(i);
        end     
    end
    
 end
 
 %find index of optoOn vs optoOff trials if optoStimRandom = true. 
 if optoStimRandom
     for i = 1:length(trials)
        if contains(trials(i).opto, 'true')
            optoOn(i) = trials(i);
        elseif contains(trials(i).opto, 'false')
            optoOff(i) = trials(i);
        end
     end
     
     
 end
  
 %delete empty elements in the array   
 optoOff = optoOff(find(arrayfun(@(optoOff) ~isempty(optoOff.number),optoOff)));
 optoOn = optoOn(find(arrayfun(@(optoOn) ~isempty(optoOn.number),optoOn)));
   
 %find percent correct at each cue length for light on vs light off trials
 if cueLengthVariable
 optoOffAccuracy = sepCueLengthandAccuracy(optoOff);
 optoOnAccuracy = sepCueLengthandAccuracy(optoOn);
 end
 
 if delayLengthVariable
 optoOffAccuracy = sepDelayLengthandAccuracy(optoOff);
 optoOnAccuracy = sepDelayLengthandAccuracy(optoOn);    
 end
 
 
 %store opto data in trial structure only for block opto stim, if random, this
 %was done already
 if optoStimBlocks
 for i = 1:length(allTrialIndex) 
   trials(i).opto = optoIndex(i);   
 end
 end
 
 %plotting opto data 
 if plotOptoData == true
  
  %create character arrays of the accuracy data to be plotted over the data
  %points in the figure
  for i = 1: length(accuracy) 
    currentDataPoint = num2str(optoOffAccuracy.Correct(i)); 
    dataPointsA(i) = {currentDataPoint};
  end
 
  for i = 1: length(accuracy) 
    currentDataPoint = num2str(optoOnAccuracy.Correct(i)); 
    dataPointsB(i) = {currentDataPoint};
  end
         
     
    figure; hold on 
    
    xAxis = 1:length(accuracy);
    
    plot(optoOffAccuracy.Correct, '-o');
    yAxis = optoOffAccuracy.Correct + 3;
    text(xAxis, yAxis, dataPointsA); 
    
    plot(optoOnAccuracy.Correct, '-o');
    yAxis = optoOnAccuracy.Correct + 3;
    text(xAxis, yAxis, dataPointsB); 
    
    xAxis = 1:5;
    xticks(xAxis);
    
    if cueLengthVariable
    if numCues == 5
        xticklabels({'5s', '2s', '1s', '500ms', '100ms'}); 
    elseif numCues == 3
        xticklabels({'5s', '2s','500ms'}); 
    end
    end
    
    if delayLengthVariable
       xticklabels({'3s', '4s','5s'});  
    end
    yticks([25 50 75 100]);
    yline(50, '--k');
    title([ 'Optogenetic Stimulation  ', animalID ]);
    ylabel('Percent correct');
    set(gca,'ylim',[20 120]);
    
    legend('Light Off','Light On');
    legend('boxoff'); 
 end

 %add opto accuracy to data structure
summary.optoOffAccuracy = optoOffAccuracy.Correct;
summary.optoOnAccuracy = optoOnAccuracy.Correct;

 end

if cueLengthVariable || delayLengthVariable
if calculateOmissions

    numOmissions = 0;
    for i = 1:length(allTrialIndex) 
        
    if trials(i).responseTime > omissionThreshold
        numOmissions = numOmissions +1;
        trials(i).accuracy = 'Omission';    
    end
        
    end
    
    if numOmissions > 0 
    if delayLengthVariable
        omissionSummary = sepDelayLengthandAccuracy(trials);
    end
    if cueLengthVariable
    omissionSummary = sepCueLengthandAccuracy(trials);
    end
    summary.percentOmissions = omissionSummary.Omissions; 
    else
        summary.percentOmissions = zeros(1,numTrialTypes);
    end
    
end
end

%% finish summary structure with descriptive information about session 

summary.sessionMinutes = sessionMinutes;
summary.numTrials = numTrials;
summary.speed = speed; 
summary.percentCorrect = percentCorrect; 
summary.leftChoices = numLeftChoices;
summary.rightChoices = numRightChoices; 


end
