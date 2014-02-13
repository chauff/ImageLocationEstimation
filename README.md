Image Location Estimation
=========================
This source code has been used for various experiments in image location estimation (also known as image placing, geo-location estimation).

The code has been implemented on top of the [Lemur Toolkit for Information Retrieval](http://sourceforge.net/projects/lemur/)
and requires it to be installed.

Since the code was first used for the Placing Task @ [MediaEval 2011](http://www.multimediaeval.org/mediaeval2011/), different runs on that dataset are also included here in the folder [MediaEval2011](https://github.com/chauff/ImageLocationEstimation/tree/master/MediaEval2011). 

For an easy start, the provided [toy example](https://github.com/chauff/ImageLocationEstimation/tree/master/ToyExample/) contains a step-by-step guid through the process with example input and parameter files.

Please note that due to the size of the test data (up to hundreds of thousands of elements in current experiments), this code requires a large amount of memory to run efficiently. The memory requirements can be (somewhat) controlled through the `higherLevel` parameter.


Publications
------------
Publications that made use of this code can be found on my [homepage](http://www.st.ewi.tudelft.nl/~hauff/). They include a [SIGIR'12 paper](http://www.st.ewi.tudelft.nl/~hauff/papers/SIGIR2012-hauff.pdf), an [ECIR'12 paper](http://www.st.ewi.tudelft.nl/~hauff/papers/ECIR2012-hauff.pdf) and [MediaEval Working Notes](http://ceur-ws.org/Vol-807/Hauff_WISTUD_Placing_me11wn.pdf).


Getting Started
---------------

To compile the code, clone the repository

```
$ git clone git://github.com/chauff/ImageLocationEstimation.git
``` 

and then adapt your local version of Lemur's `Makefile.app` (which was generated during the compilation of the Lemur Toolkit) by adapting
the OBJS and PROG lines:

```
## specify your object files here
OBJS = PredictionRun.o TestItem.o Util.o Evaluation.o GeoDoc.o GeoNode.o TermDistributionFilter.o Metadata.o ParameterSingleton.o
## specify your program here
PROG = bin/PredictionRun
```

Running:

```
make -f Makefile.app
```

will generate the executable `PredictionRun`. It's only input is a parameter file which is explained in detail below.


Creating an Index
-----------------

First, we need to create an inverted index, which creates a document for each region in the world. The "training data"
for this task are images which contain geo-information (latitude/longitude) as well as textual metadata.
The location of the "test data" is to be estimated.

The Lemur Toolkit requires the documents to be indexed to be provided in a particular XML-like format such as:

```
<DOC>
<DOCNO>2537024282</DOCNO>
<LONGITUDE>-138.51164</LONGITUDE>
<LATITUDE>-89.99999</LATITUDE>
<USERID>10403985@N04</USERID>
<TAGS>de azulooo desfiguraciones</TAGS>
<TITLE>my holidays</TITLE>
<TIMETAKEN>1212154760</TIMETAKEN>
<USERLOCATION>Los Angeles, US, earth</USERLOCATION>
<ACCURACY>6</ACCURACY>
</DOC>
```

Each `<DOC>..</DOC>` encodes the information of one training image and a single text file can contain an unlimited number of `<DOC>...</DOC>` elements (though indexing will become slow if too many `<DOC>` elements exist per file).

Once the data is in this format, indexing with the Lemur Toolkit is simply creating a parameter file:

```
<parameters>  
<corpus>
<path>/path/to/imageCorpus</path>
<class>trectext</class>
</corpus>
<memory>5G</memory>
<index>/path/to/imageIndex</index>
<metadata>
<forward>DOCNO</forward>
<backward>DOCNO</backward>
<forward>USERID</forward>
<forward>TIMETAKEN</forward>
<forward>LONGITUDE</forward>
<forward>LATITUDE</forward>
<forward>ACCURACY</forward>
<forward>USERLOCATION</forward>
</metadata>
<field><name>TAGS</name></field>
<field><name>TITLE</name></field>
</parameters>
 ```
and then calling `IndriBuildIndex` (one of the apps of the Lemur Toolkit) with this file as only parameter. This creates an inverted index.

Note that for the purposes of indexing, no difference is made between training and test data, both should be in the corpus and be indexed.

Also note the difference between fields and metadata in the indexing process.

Running PredictionRun
---------------------

Once the index has been created, `PredictionRun` can be run.
It requires as only input a parameter file:

```
<parameters>
<index>/path/to/flickrIndex</index>
<resultFile>/path/to/resultFile</resultFile>
<smoothingParam>10000</smoothingParam>
<dirParamNN>10</dirParamNN>
<splitLimit>5000</splitLimit>
<excludeTestUsers>true</excludeTestUsers>
<minLongitudeStep>0.01</minLongitudeStep>
<minLatitudeStep>0.01</minLatitudeStep>
<testFile>/path/to/test_file</testFile>
<geoTermThreshold>-1</geoTermThreshold>
<generalUseTermFilter>-1</generalUseTermFilter>
<defaultLatitude>0</defaultLatitude>
<defaultLongitude>0</defaultLongitude>
<priorFile>null</priorFile>
<fields>tags</fields>
<alphas>1</alphas>
<higherLevel>7</higherLevel>
<skippingModulos>25</skippingModulos>
</parameters>
```

Meaning of the different parameters:

* `index`: path to the index folder created in the previous step
* `resultFile`: path to the file where the results (i.e. some meta-data and the estimated locations for the test data including the error distance with respect to the true location); if the result file is not empty, the file is read, and the test items are skipped when running the program again (i.e. if the program crashes, you don't have to restart from scratch)
* `smoothingParam`: Dirichlet-smoothed language modeling is used as retrieval approach; this is the `mu` parameter
* `dirParamNN`: location estimation is a two step process: (1) the correct world region is identified and then, (2) the nearest neighbour to the test image in the found region is identified; this is the smoothing parameter of step (2)
* `splitLimit`: world regions are created dynamically, based on the training data. Once a region contains `splitLimit` images, it is split into 4 equally sized child regions
* `excludeTestUsers`: if set to `true`, all images contributed by users that occur in the test data are not used in the training data; if it is set to `false`, test and training users are not split up
* `minLongitudeStep`/`minLatitudeStep`: once a region has a longitude/latitude size less than these minima, no further splitting is performed (ensures that regions do not become too small)
* `testFile`: the test file contains the test data in the form of one `docno` per line - all specified ids need to exist in the index
* `geoTermThreshold`: if 0 or negative no geo-filtering is conducted, otherwise for each term the geo-score is computed and terms are filtered out if their score is above the threshold
* `generalUseTermFilter`: if 0 or negative this filter has no effect, otherwise terms are filtered out if they have not been used by at least the provided number of unique users
* `defaultLatitude`/`defaultLongitude`: if no estimate of the location can be made (because no textual meta-data is available for a test image), the here defined location is the default location used instead (a common choice is either 0/0 or the location that occurs most often in the training data)
* `priorFile`: language modeling has a principled way of including prior information - each region can be assigned a prior probability of relevance; the priorFile format is as follows (one per line): `score latitude longitude` where the `score` can be any positive value. One example is population data - regions of higher population density are more likely to have images taken in them. Each line in the prior file is then `population latitude-city-center longitude-city-center`. The program takes care to determine the correct region for each of these scores. All scores of a region are added up and finally all scores are turned into probabilities.
Instead of a valid filename, `<priorFile>POPULARITY</priorFile>` can be used, which creates a prior based on the popularity of the region in the training data.
* `fields`: the data can contain more than one type of textual meta-data, next to `tags` also `titles` or `descriptions` and `userHomeLocation` (these are then also specified in the parameter file for indexing). Which fields should be considered is defined here. If we have tags as well as titles the line would like as follows: `<fields>tags titles</fields>`
* `alphas`: this parameter entry has the same number of values as `fields`; it defines how the scores from the different fields should be interpolated. For the tags+titles example a valid entry would be `<alphas>0.7 0.3</alphas>` which gives a weight of 0.7 to the tags score and a weight of 0.3 to the titles score.
* `higherLevel`: a parameter to speed up experiments; needs to be used with care, can lead to worse results; instead of directly evaluating all identified regions, a test image is first matched against higher-level nodes in the region tree and then only the sub-tree achieving the highest score is used further on
* `skippingModulos`: we can index 100 million images or more; but if for experimental purposes less documents should be used in training, this parameter indicates which documents should be skipped, e.g. `<skippingModulos>10</skippingModulos>` means that only 1/10th of all available training data should be used
* `minAccuracy`: it is also possible to specify the accuracy level of the training data (as meta-data field during Lemur's indexing with `<ACCURACY>`) and only allow training data to enter the region models with at least the specified accuracy level
* `useHomeLocation`: if set to `true`, the user's home location string will be used for location estimation if no text data is available for a test item. This of course only works if the home location is set for the test items (as metadata in the Lemur index with XML element `<USERLOCATION>`).  


[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/chauff/imagelocationestimation/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

