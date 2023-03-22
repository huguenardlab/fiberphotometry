%--------------------------------------------------------------------------
%sepCueLengthandAccuracy
%By: Brielle Ferguson
%Date: 10/22/19
%Function that will take trial structure generated from the 
%readAnalayzePlotBehData function and then calculate accuracy at each cue length
%Assumptions:
% - data is a trial structure generated by readAnalyzePlotBehData
% - cue lengths are only those listed in the variable list below
%inputs: 
% - data: "trials" structure generated by function above
%outputs: 
% - accuracy: accuracy.correct is an array of percentage of correct choices at each
%   cue length and accuracy.omissions is the percentage of omissions
%--------------------------------------------------------------------------


function [accuracy] = sepCueLengthandAccuracy(data)

%variables to count amount of correct, incorrect, and omission trials
num5000Correct = 0;
num2000Correct = 0;
num1000Correct = 0;
num500Correct = 0;
num100Correct = 0;
num5000Incorrect = 0;
num2000Incorrect = 0;
num1000Incorrect = 0;
num500Incorrect = 0;
num100Incorrect = 0;
num5000Omission = 0;
num2000Omission = 0;
num1000Omission = 0;
num500Omission = 0;
num100Omission = 0;
                 
%loop through each trial and determine cue length and accuracy  
for i = 1:length(data)
  if data(i).cuelength == 5000
      if strcmp(data(i).accuracy ,'Correct')
          num5000Correct = num5000Correct + 1;
      elseif strcmp(data(i).accuracy ,'Incorrect')
          num5000Incorrect = num5000Incorrect + 1;
      else
          num5000Omission = num5000Omission + 1;
      end
  end
  
  if data(i).cuelength == 2000
      if strcmp(data(i).accuracy ,'Correct')
          num2000Correct = num2000Correct + 1;
      elseif strcmp(data(i).accuracy ,'Incorrect')
          num2000Incorrect = num2000Incorrect + 1;
      else
          num2000Omission = num2000Omission + 1;
      end
  end

  if data(i).cuelength == 1000
      if strcmp(data(i).accuracy ,'Correct')
          num1000Correct = num1000Correct + 1;
      elseif strcmp(data(i).accuracy ,'Incorrect')
          num1000Incorrect = num1000Incorrect + 1;
      else
          num1000Omission = num1000Omission + 1;
      end
  end
  
  if data(i).cuelength == 500
      if strcmp(data(i).accuracy ,'Correct')
          num500Correct = num500Correct + 1;
      elseif strcmp(data(i).accuracy ,'Incorrect')
          num500Incorrect = num500Incorrect + 1;
      else
          num500Omission = num500Omission + 1;
      end
  end
  
  if data(i).cuelength == 100
      if strcmp(data(i).accuracy ,'Correct')
          num100Correct = num100Correct + 1;
      elseif strcmp(data(i).accuracy ,'Incorrect')
          num100Incorrect = num100Incorrect + 1;
      else
          num100Omission = num100Omission + 1;
      end
  end          
  
end

%calculate total omissions      
numOmissions = num5000Omission + num2000Omission + num1000Omission + num500Omission + num100Omission;     

%calculate percentage with just correct and incorrect if there are no omissions
if numOmissions == 0      
percentCorrect100ms = 100 * (num100Correct/(num100Correct+ num100Incorrect)); 
percentCorrect500ms = 100 * (num500Correct/(num500Correct+ num500Incorrect));
percentCorrect1000ms = 100 * (num1000Correct/(num1000Correct+ num1000Incorrect)); 
percentCorrect2000ms = 100 * (num2000Correct/(num2000Correct+ num2000Incorrect)); 
percentCorrect5000ms = 100 * (num5000Correct/(num5000Correct+ num5000Incorrect)); 

%create table with percent correct at each cue length and a separate table for
%omissions
accuracy.Correct = [percentCorrect5000ms percentCorrect2000ms percentCorrect1000ms percentCorrect500ms percentCorrect100ms]; 
accuracy.Omissions = zeros(1,length(accuracy.Correct));
end

%calculate percentage correct and percent omissions 
if numOmissions > 0 
percentCorrect100ms = 100 * (num100Correct/(num100Correct+ num100Incorrect + num100Omission)); 
percentCorrect500ms = 100 * (num500Correct/(num500Correct+ num500Incorrect + num500Omission));
percentCorrect1000ms = 100 * (num1000Correct/(num1000Correct+ num1000Incorrect + num1000Omission)); 
percentCorrect2000ms = 100 * (num2000Correct/(num2000Correct+ num2000Incorrect + num2000Omission)); 
percentCorrect5000ms = 100 * (num5000Correct/(num5000Correct+ num5000Incorrect + num5000Omission));   

percentOmission100ms = 100 * (num100Omission/(num100Correct+ num100Incorrect + num100Omission)); 
percentOmission500ms = 100 * (num500Omission/(num500Correct+ num500Incorrect + num500Omission));
percentOmission1000ms = 100 * (num1000Omission/(num1000Correct+ num1000Incorrect + num1000Omission)); 
percentOmission2000ms = 100 * (num2000Omission/(num2000Correct+ num2000Incorrect + num2000Omission)); 
percentOmission5000ms = 100 * (num5000Omission/(num5000Correct+ num5000Incorrect + num5000Omission));  

accuracy.Correct = [percentCorrect5000ms percentCorrect2000ms percentCorrect1000ms percentCorrect500ms percentCorrect100ms]; 
accuracy.Omissions = [percentOmission5000ms percentOmission2000ms percentOmission1000ms percentOmission500ms percentOmission100ms]; 
end

     
end