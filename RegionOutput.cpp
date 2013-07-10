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
#include "NaiveBayes.hpp"
#include "Distance.hpp"
#include "Evaluation.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include "ParameterSingleton.hpp"
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <iostream>
#include <sstream>
#include <string>

/*
 * This code computes the region split for the training data and outputs one file with the format
 *
 *	[line 1] total number of terms in the corpus
 *	[line 2] corpus language model in the form [term] [tf in corpus] [term] [tf in corpus] ...
 *	[line 3] language model of region 1 [center_lat] [center_lng] [term] [tf] [term] [tf] ....
 *	[line 4] language model of region 2 [center_lat] [center_lng] [term] [tf] [term] [tf] ....
 * the output is written to <resultFile>
 */

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::extra;

namespace LocalParameter
{
	std::string resultFile;//write out the test results
	int splitLimit;//after how many documents accumulate in a node, should the node be split ...
	double minLongitudeStep;//splitting of nodes stops, once the longitude is this fine-graind (longitude interval of cell is 2*minLongitudeStep)
	double minLatitudeStep;//same for latitude
	int minAccuracy;
	double geoTermThreshold;//only terms with geoterm score <= threshold are kept; <0 means no geoterm filtering
	int generalUseTermFilter;//number of users that should have used the term ....

	void get()
	{
		resultFile = ParamGetString("resultFile");
		splitLimit = ParamGetInt("splitLimit", 5000);
		minLongitudeStep = ParamGetDouble("minLongitudeStep", 0.01);
		minLatitudeStep = ParamGetDouble("minLatitudeStep", 0.01);
		minAccuracy = ParamGetInt("minAccuracy", 11);
		geoTermThreshold = ParamGetDouble("geoTermThreshold",-1.0);
		generalUseTermFilter = ParamGetInt("generalUseTermFilter",-1);
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
		exit(1);
	}

	//initialize the parameter object
	lemur::extra::ParameterSingleton *ps = lemur::extra::ParameterSingleton::getInstance(&repository,ind,&env,NULL);//no image environment!
	ps->GN_SPLIT_NUM = LocalParameter::splitLimit;
	ps->GN_MIN_LATITUDE_STEP = LocalParameter::minLatitudeStep;
	ps->GN_MIN_LONGITUDE_STEP = LocalParameter::minLongitudeStep;

	std::cerr<<"parameter initialized"<<std::endl;

	//we need a collection LM
	//if we exclude the test users, the colLM needs to be calculated separately
	lemur::langmod::DocUnigramCounter *colDC;
	lemur::langmod::MLUnigramLM *colLM;

	std::vector<int> trainingDocids;
	for(int docid=1; docid<=ind->docCount(); docid++)
		trainingDocids.push_back(docid);

	colDC = new lemur::langmod::DocUnigramCounter(trainingDocids,*ind);
	colLM = new lemur::langmod::MLUnigramLM(*colDC,ind->termLexiconID());

	std::cerr<<"collection LM created"<<std::endl;

	//create the tree consisting of GeoNode elements
	GeoNode *rootNode = new GeoNode();

	std::vector<std::string> svecAcc = env.documentMetadata(trainingDocids,"accuracy");
	std::vector<std::string> svecMed = env.documentMetadata(trainingDocids,"mediatype");
	std::vector<std::string> svecLat = env.documentMetadata(trainingDocids,"latitude");
	std::vector<std::string> svecLng = env.documentMetadata(trainingDocids,"longitude");
	std::vector<std::string> svecUserid = env.documentMetadata(trainingDocids,"userid");

	int numAddedDocids = 0;
	for (int i=0; i<trainingDocids.size(); i++)
	{
		int docid = trainingDocids.at(i);
		if (docid % 100000 == 0)
			std::cerr << " At document " << docid << std::endl;

		int accuracy = atoi(svecAcc[i].c_str());
		std::string mediaType = svecMed[i];

		//if the training datum does not reach minimum accuracy, ignore it
		//for the videos, we have to assume they are of high enough quality location wise ...
		if (accuracy < LocalParameter::minAccuracy && mediaType.compare("img")==0)
			continue;

		rootNode->addDocument(docid);
		numAddedDocids++;
	}
	std::cerr << "number of documents added to GeoTree: "<< numAddedDocids << std::endl;

	//the TermDistributionFilter is implemented as a singleton; first call initializes the necessary parameters
	lemur::extra::TermDistributionFilter* filter = lemur::extra::TermDistributionFilter::getInstance(rootNode);

	//create a hashmap with all leaf nodes (for easy iteration)
	std::set<GeoNode*> *leaves = new std::set<GeoNode*>();
	rootNode->fillSetWithLeaves(leaves);



	for(int i=0; i<5; i++)
	{

	double latitude = 48.8567;
	double longitude = 2.3508;
	std::string name = "paris";

	if(i==1)
	{
		latitude = 40.664167;
		longitude =  -73.938611;
		name = "nyc";
	}
	if(i==2)
	{
		latitude = -33.859972;
		longitude = 151.211111;
		name = "sydney";
	}
	if(i==3)
	{
		latitude = 49.444722;
		longitude = 7.768889;
		name = "kaiserslautern";
	}
	if(i==4)
	{
		latitude = -22.908333;
		longitude = -43.196389;
		name = "rio";
	}


	std::stringstream filename;
	filename<<LocalParameter::resultFile<<"_"<<name;
	ofstream out(filename.str().c_str());
	if(out.is_open()==false)
	{
		std::cerr<<"Unable to open "<<LocalParameter::resultFile<<" for writing"<<std::endl;
		exit(1);
	}

	int numNonEmpty=0;
	for(std::set<GeoNode*>::iterator it=leaves->begin(); it!=leaves->end(); it++)
	{
		GeoNode *n = *it;

		if(n->isGeoDocFitting(latitude,longitude)==false)
		{
			continue;
		}
	
		if(n->isLeafWithContent()==true)
		{
			numNonEmpty++;		
			out<<n->latitudeCenter<<" "<<n->longitudeCenter;

			for(int termid=1; termid<=ind->termCountUnique(); termid++)
			{
				if(termid%500==0 && numNonEmpty==1)
					std::cerr<<"At term "<<termid<<" out of "<<ind->termCountUnique()<<std::endl;

				if(filter->isTermInGeneralUse(termid, LocalParameter::generalUseTermFilter)==false)
					continue;

				if(filter->isTermGeographic(termid, LocalParameter::geoTermThreshold)==false)
					continue;

				int tf = n->getMLTermCount(termid);
				if(tf<5)
					continue;

				out<<" "<<ind->term(termid)<<" "<<tf<<std::flush;
			}
			out<<endl;
		}
	}
	out.close();

	std::cerr<<"Results written for "<<numNonEmpty<<" regions to "<<LocalParameter::resultFile<<std::endl;
	}
	delete ind;
}



