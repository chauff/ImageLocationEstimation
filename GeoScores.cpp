#include "IndexManager.hpp"
#include "common_headers.hpp"
#include "BasicDocStream.hpp"
#include "RetMethodManager.hpp"
#include "ResultFile.hpp"
#include "RelDocUnigramCounter.hpp"
#include "indri/Repository.hpp"
#include "indri/QueryEnvironment.hpp"
#include "GeoNode.hpp"
#include "GeoDoc.hpp"
#include "Distance.hpp"
#include "Evaluation.hpp"
#include "ParameterSingleton.hpp"
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream


//print out a parameter
#define SHOW(a) std::cerr << #a << ": " << (a) << std::endl

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::extra;

namespace LocalParameter
{
	std::string resultFile;//write out the test results
	std::string testFile;//this is where the test items are kept, format: testid latitude longitude (for quick eval)
	std::string priorFile;//a prior can be used (e.g. population prior); format: <latitude> <longitude> <num> (num can be population); each (lat/long) is assigned to the respective GeoNode and <num> are added up
	double smoothingParam;//amount of smoothing; depends on the language modeling type ...
	double dirParamNN;
	int splitLimit;//after how many documents accumulate in a node, should the node be split ...
	double minLongitudeStep;//splitting of nodes stops, once the longitude is this fine-graind (longitude interval of cell is 2*minLongitudeStep)
	double minLatitudeStep;//same for latitude

	bool excludeTestUsers;//exclude the users who have images in the test set from training
	double geoTermThreshold;//only terms with geoterm score <= threshold are kept; <0 means no geoterm filtering
	int generalUseTermFilter;//number of users that should have used the term ....
	double defaultLatitude;//latitude to assign when no guess can be made (no tags, no home location)
	double defaultLongitude;//longitude to assign when no guess can be made
	std::string fields;//list of fields (e.g. TITLE, TWEETS, TAGS)
	std::string alphas;//list of alpha values (determines how to combine the fields)

	std::string geoTermThresholdFile;//keep track of <termid> <geoScore> (reads/writes)

	int takenInMonth;//use for training/testing only images of a particular month

	int modulosOp;

	std::string termFile;//1 term per line for which to compute the geo scores

	void get()
	{
		resultFile = ParamGetString("resultFile");
		testFile = ParamGetString("testFile");
		priorFile = ParamGetString("priorFile");
		smoothingParam = ParamGetDouble("smoothingParam", 0.5);
		dirParamNN = ParamGetDouble("dirParamNN", 100);
		splitLimit = ParamGetInt("splitLimit", 5000);
		minLongitudeStep = ParamGetDouble("minLongitudeStep", 0.01);
		minLatitudeStep = ParamGetDouble("minLatitudeStep", 0.01);
		std::string s = ParamGetString("excludeTestUsers","false");
		excludeTestUsers = (s.compare("false")==0) ? false : true;

		geoTermThreshold = ParamGetDouble("geoTermThreshold",-1.0);
		generalUseTermFilter = ParamGetInt("generalUseTermFilter",-1);
		defaultLatitude = ParamGetDouble("defaultLatitude", 0.0);
		defaultLongitude = ParamGetDouble("defaultLongitude", 0.0);

		fields = ParamGetString("fields");
		alphas = ParamGetString("alphas");

		geoTermThresholdFile = ParamGetString("geoTermThresholdFile");

		takenInMonth = ParamGetInt("takenInMonth",-1);

		modulosOp = ParamGetInt("modulosOp",1);//if fewer training docs should be used, modulosOp>1

		termFile = ParamGetString("termFile","");
	}
}
;

void GetAppParam()
{
	RetrievalParameter::get();
	SimpleKLParameter::get();
	LocalParameter::get();
}


int AppMain(int argc, char* argv[])
{
    std::cerr<<"Reading from parameter file "<<argv[1]<<std::endl;

	indri::collection::Repository repository;//indri: access to fields & co
	lemur::api::Index *ind;//conversion of external/internal docids, unidgramdoccounter
	indri::api::QueryEnvironment env;//Indri again

	try
	{
		repository.openRead(RetrievalParameter::databaseIndex);
		ind = IndexManager::openIndex(RetrievalParameter::databaseIndex);
		env.addIndex(RetrievalParameter::databaseIndex);
	}
	catch (Exception &e)
	{
		std::cerr << "Error when opening index ..." << std::endl;
		std::cerr << e.what() << std::endl;
		exit(1);
	}

	//print all LocalParameter entries
	SHOW(LocalParameter::resultFile);
	SHOW(LocalParameter::testFile);
	SHOW(LocalParameter::priorFile);
	SHOW(LocalParameter::smoothingParam);
	SHOW(LocalParameter::dirParamNN);
	SHOW(LocalParameter::splitLimit);
	SHOW(LocalParameter::minLatitudeStep);
	SHOW(LocalParameter::minLongitudeStep);
	SHOW(LocalParameter::excludeTestUsers);
	SHOW(LocalParameter::geoTermThreshold);
	SHOW(LocalParameter::generalUseTermFilter);
	SHOW(LocalParameter::defaultLatitude);
	SHOW(LocalParameter::defaultLongitude);
	SHOW(LocalParameter::alphas);
	SHOW(LocalParameter::fields);
	SHOW(LocalParameter::excludeTestUsers);
	SHOW(LocalParameter::geoTermThresholdFile);
	SHOW(LocalParameter::modulosOp);
	SHOW(LocalParameter::takenInMonth);
	SHOW(LocalParameter::termFile);

	//initialize the parameter object
	lemur::extra::ParameterSingleton *ps = lemur::extra::ParameterSingleton::getInstance(&repository,ind,&env);//no image environment!
	ps->GN_SPLIT_NUM = LocalParameter::splitLimit;
	ps->GN_MIN_LATITUDE_STEP = LocalParameter::minLatitudeStep;
	ps->GN_MIN_LONGITUDE_STEP = LocalParameter::minLongitudeStep;
	ps->geoTermThreshold = LocalParameter::geoTermThreshold;
	ps->generalUseTermFilter = LocalParameter::generalUseTermFilter;
	ps->parseAlphas(LocalParameter::alphas);
	ps->parseFields(LocalParameter::fields);
	ps->geoTermThresholdFile=LocalParameter::geoTermThresholdFile;
	ps->modulosOp = LocalParameter::modulosOp;
	ps->takenInMonth = LocalParameter::takenInMonth;

	//sanity check
	if(ps->GN_ALPHAS.size()!=ps->GN_FIELDS.size())
	{
		std::cerr<<"Error: number of fields ("<<ps->GN_FIELDS.size()<<") needs to match alpha values ("<<ps->GN_ALPHAS.size()<<")"<<std::endl;
		exit(1);
	}

	//initialize the evaluation
	lemur::extra::Evaluation eval(LocalParameter::excludeTestUsers, LocalParameter::testFile);
	ps->eval=&eval;

	//we need a collection LM
	//if we exclude the test users, the colLM needs to be calculated separately
	lemur::langmod::DocUnigramCounter *colDC;
	lemur::langmod::MLUnigramLM *colLM;

	//test documents are not included in the collection LM
	std::vector<int> trainingDocids;

	int stepSize=500000;
	for(int boundary=1; boundary<=ind->docCount(); boundary+=stepSize)
	{
	    std::cerr<<"boundary: "<<boundary<<std::endl;
	    	std::vector<int> docidVec;
		for(int docid=boundary;docid<(boundary+stepSize)&&docid<=ind->docCount(); docid++)
			docidVec.push_back(docid);

		std::vector<std::string> timetaken = env.documentMetadata(docidVec,"timetaken");

		for(int i=0; i<docidVec.size(); i++)
		{
		    int docid = docidVec[i];

			if(eval.isTrainingItem(docid)==true)
				trainingDocids.push_back(docid);
		}
	}

	std::cerr<<"Training documents determined"<<std::endl;
	std::cerr<<"in total: "<<trainingDocids.size()<<std::endl;

	colDC = new lemur::langmod::DocUnigramCounter(trainingDocids,*ind);
	colLM = new lemur::langmod::MLUnigramLM(*colDC,ind->termLexiconID());

	ps->colLM = colLM;

	std::cerr<<"collection LM created"<<std::endl;

	//create the tree consisting of GeoNode elements
	GeoNode *rootNode = new GeoNode();
	int numAddedDocids = 0;
	for (int i=0; i<trainingDocids.size(); i++)
	{
		int docid = trainingDocids.at(i);
		if (docid % 50000 == 0)
			std::cerr << " At document " << docid << std::endl;

		rootNode->addDocument(docid);
		numAddedDocids++;
	}
	std::cerr << "number of documents used as training data: "<< numAddedDocids << std::endl;

	//the TermDistributionFilter is implemented as a singleton; first call initializes the necessary parameters
	lemur::extra::TermDistributionFilter* tdf = lemur::extra::TermDistributionFilter::getInstance(rootNode);


	ifstream in(LocalParameter::termFile.c_str());
	if(in.is_open()==false)
	{
		std::cerr<<"Unable to read from term file "<<LocalParameter::termFile<<std::endl;
		exit(1);
	}

	std::string term;

	while(in>>term)
	{
		int termid = ind->term(term);
		if(ind->termCount(termid)<20)
			continue;
		tdf->isTermGeographic(termid);
	}
	in.close();

	delete tdf;
	delete ind;
}



