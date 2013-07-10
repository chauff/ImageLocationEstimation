Image Location Estimation
=========================
This source code has been used for various experiments in image location estimation (also known as image placing, geo-location estimation).

The code has been implemented on top of the [Lemur Toolkit for Information Retrieval](http://sourceforge.net/projects/lemur/)
and requires it to be installed.


Getting Started
---------------

To compile the code, clone the repository

```
$ git clone git://github.com/charlotteHase/ImageLocationEstimation.git
``` 

and then adapt your local version of Makefile.app (which was generated during the compilation of the Lemur Toolkit) by adapting
the OBJS and PROG lines:

```
## specify your object files here
OBJS = BaselinePrediction.o Evaluation.o GeoDoc.o GeoNode.o TermDistributionFilter.o Metadata.o ParameterSingleton.o
## specify your program here
PROG = bin/BaselinePrediction
```

Running:

```
make -f Makefile.app
```

should then generate a binary BaselinePrediction in the bin folder.
