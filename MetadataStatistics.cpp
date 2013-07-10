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
/*
 * this computes some statistics on the image metadata
 */

//print out a parameter
#define SHOW(a) std::cerr << #a << ": " << (a) << std::endl

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::extra;


void GetAppParam()
{
	RetrievalParameter::get();
	SimpleKLParameter::get();
}


int AppMain(int argc, char* argv[])
{
	std::cerr << "Reading from parameter file " << argv[1] << std::endl;

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

	long minDate = LONG_MAX;
	long maxDate = LONG_MIN;

	std::map<std::string,int> dateMap;
	int stepSize=500000;
	for(int boundary=1; boundary<=ind->docCount(); boundary+=stepSize)
	{
		std::cerr<<"at step "<<boundary<<std::endl;
	    std::vector<int> docidVec;
		for(int docid=boundary;docid<(boundary+stepSize)&&docid<=ind->docCount(); docid++)
			docidVec.push_back(docid);

		std::vector<std::string> timetaken = env.documentMetadata(docidVec,"timetaken");
		for(int i=0; i<timetaken.size(); i++)
		{
			long l = atol(timetaken[i].c_str());

			time_t tt = l;
			struct tm *date = localtime(&tt);
			int year = date->tm_year + 1900;
			int month = date->tm_mon + 1;

			std::stringstream ss;
			ss<<month<<"/"<<year;

			std::string key = ss.str();
			std::map<std::string,int>::iterator it = dateMap.find(key);
			if(it == dateMap.end())
				dateMap.insert(std::make_pair(key, 1));
			else
				it->second += 1;
		}
	}

	for(std::map<std::string,int>::iterator it = dateMap.begin(); it!=dateMap.end(); it++)
	{
		std::cerr<<it->first<<" "<<it->second<<std::endl;
	}


	delete ind;
}

