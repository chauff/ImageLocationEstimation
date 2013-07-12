MediaEval 2011 Runs
===================
This folder contains the result files when running version v0.1 (GitHub tag) of the software on the MediaEval 2011 data set.

The results are approximately the same as reported in http://ceur-ws.org/Vol-807/Hauff_WISTUD_Placing_me11wn.pdf (the original working notes paper where parts of the code were first used). To make the comparison easier, the results of that paper are shown below:
 
![alt text](table2.png "Location estimation results as presented in the working notes") 

The parameter file settings are contained in the result files, as well as the accuracy overview and the estimated locations for each test item. This is the output the BaselinePrediction program generates.

* `res.baseline` corresponds to the Basic run in the table
* `res.geogen` corresponds to the GeoGen run in the table
* `res.gen` corresponds to the Gen run in the table

To show the influence of the overlap between training and test users, `res.excludeTestUsers` is a baseline run (no geo-filtering) where the training data does not contain a single element from the test uers.

