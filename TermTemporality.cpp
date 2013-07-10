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
 * this program reads terms from termFile and for each term,
 * it determines the geographic spread when the corpus is restricted to a particular time (month AND/OR year AND/OR weekend) AND/OR a particular continent.
 * Note that testFile is also required so that all documents coming from test users can be excluded
 * from the training items.
 *
 * The output is a file where for each valid time the geographic spread score of the terms is listed
 */

//print out a parameter
#define SHOW(a) std::cerr << #a << ": " << (a) << std::endl

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::extra;

namespace LocalParameter
{
	std::string resultFile;//write out the results
	std::string termFile;//read a list of terms (one per line) to evaluate
	std::string testFile;//this is where the test items are kept, format: testid latitude longitude (for quick eval)
	std::string exampleOutFile;//write example documents (their assigned tags) for each time period to this file

	bool excludeTestUsers;//exclude the users who have images in the test set from training

	int takenInMonth;//use for training/testing only images of a particular month (<0 no month based partition, >0 month based partition)
	int takenInYear;//images taken in a particular year
	int isWeekend;//images taken in the weekend
	int takenOnContinent;//images taken on a particular continent

	void get()
	{
		resultFile = ParamGetString("resultFile");
		exampleOutFile = ParamGetString("exampleOutFile","-1");
		termFile = ParamGetString("termFile");
		testFile = ParamGetString("testFile");
		std::string s = ParamGetString("excludeTestUsers", "false");
		excludeTestUsers = (s.compare("false") == 0) ? false : true;
		takenInMonth = ParamGetInt("takenInMonth", -1);
		takenInYear = ParamGetInt("takenInYear", -1);
		isWeekend = ParamGetInt("isWeekend",-1);
		takenOnContinent = ParamGetInt("takenOnContinent",-1);
	}
}
;

void GetAppParam()
{
	RetrievalParameter::get();
	SimpleKLParameter::get();
	LocalParameter::get();
}

/*
 * method computes the geo-score for the wanted terms
 */
std::map<int,double>* computeScores(lemur::extra::ParameterSingleton *ps, lemur::api::Index *ind, std::vector<int>& termids, std::string outfile="", std::string outColumn0="")
{
	static int numCalls = 0;

	//note that in the eval object, it is determined which documents in the index fulfill the temporal constraints
	lemur::extra::Evaluation eval(LocalParameter::excludeTestUsers,LocalParameter::testFile);
	ps->eval = &eval;

	//the TermDistributionFilter is implemented as a singleton; first call initializes the necessary parameters
	lemur::extra::TermDistributionFilter *filter = lemur::extra::TermDistributionFilter::getInstance(NULL);

	ifstream termIn(LocalParameter::termFile.c_str());
	if (termIn.is_open() == false)
	{
		std::cerr << "Unable to read from " << LocalParameter::termFile
				<< std::endl;
		exit;
	}

	std::string term;
	while (termIn >> term)
	{
		int termid = ind->term(term);
		if (termid <= 0)
		{
			std::cerr << "Term [" << term << "] not found in index"
					<< std::endl;
			continue;
		}
		std::cerr << "\t Evaluating " << term << std::endl;
		if(numCalls==0)
			termids.push_back(termid);
		filter->isTermGeographic(termid);

		//if an outfile name is provided, assume that examples of documents with the tag should be written
		if(outfile.compare("") != 0)
		{
			std::ofstream out(outfile.c_str(),ios::out | ios::app  );
			if(out.is_open()==false)
				std::cerr<<"Unable to open "<<outfile<<" for writing."<<std::endl;
			else
			{
				DocInfoList *dlist = ind->docInfoList(termid);
				dlist->startIteration();
				std::vector<int> tmp;
				tmp.push_back(0);
				while(dlist->hasMore())
				{
					DocInfo *info = dlist->nextEntry();
					int docid = info->docID();
					if(!eval.isInCorrectInterval(docid))
						continue;

					out<<outColumn0<<" docid:"<<ind->document(docid);

					tmp[0]=docid;
					std::vector<std::string> timetaken = eval.env->documentMetadata(tmp,"timetaken");
					out<<" timetaken:"<<timetaken[0]<<" target:"<<term<<" ";

					TermInfoList *tlist = ind->termInfoList(docid);
					tlist->startIteration();
					while(tlist->hasMore())
					{
						TermInfo *tinfo = tlist->nextEntry();
						out<<ind->term(tinfo->termID())<<" ";
					}
					delete tlist;
					out<<std::endl;
				}
				delete dlist;

				out.close();
			}
		}
		else
			std::cerr<<"Alert: No example file provided"<<std::endl;
	}
	termIn.close();
	numCalls++;

	std::map<int, double> *scores = filter->evaluatedGeoTerms;

	std::map<int,double> *m = new std::map<int,double>();
	for(std::map<int,double>::iterator it = scores->begin(); it!=scores->end(); it++)
		m->insert(std::make_pair(it->first,it->second));

	scores->clear();
	return m;
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

	//print all LocalParameter entries
	SHOW(LocalParameter::resultFile);
	SHOW(LocalParameter::termFile);
	SHOW(LocalParameter::testFile);
	SHOW(LocalParameter::excludeTestUsers);
	SHOW(LocalParameter::exampleOutFile);
	SHOW(LocalParameter::takenInMonth);
	SHOW(LocalParameter::takenInYear);
	SHOW(LocalParameter::takenOnContinent);
	SHOW(LocalParameter::isWeekend);

	//bounding boxes for different continents: [0]:north [1]:east [2]:south [3]:west
	//hardcoded values, as they are constant
	double BB_Europe[]	={81.0, 39.869, 27.635, -31.266};
	double BB_Africa[]	={37.567, 63.525, -46.9, -25.3587};
	double BB_NAmerica[]={83.162, -52.233, 5.49955, -167.2764};
	double BB_Australia[]={-6.06945, 175.292, -53.059, 105.377};
	double* BBs[] ={BB_Europe,BB_Africa,BB_NAmerica,BB_Australia};
	std::string continents[] = {"europe","africa","namerica","australia"};

	//initialize the parameter object
	lemur::extra::ParameterSingleton *ps =
			lemur::extra::ParameterSingleton::getInstance(&repository, ind,
					&env);

	ofstream out(LocalParameter::resultFile.c_str());
	out << "time ";

	std::vector<int> termids;

	//if no year restriction is provided, we need to loop through here only once
	for (int takenInYear = (LocalParameter::takenInYear<0 ? 2013 : 2005); takenInYear <= 2013; takenInYear++)
	{
		//if no month restriction, one loop is enough
		for (int takenInMonth = (LocalParameter::takenInMonth<0 ? 12 : 1); takenInMonth <= 12; takenInMonth++)
		{
			//if no weekday restriction, one loop is enough
			for(int isWeekend = (LocalParameter::isWeekend<0 ? 1 : 0); isWeekend<2; isWeekend++)
			{
				//if no continent restriction, one loop is enough
				for(int continent=(LocalParameter::takenOnContinent<0 ? 3 : 0); continent<4; continent++)
				{
					if(LocalParameter::takenOnContinent>0)
					{
						ps->BB_N=BBs[continent][0];
						ps->BB_E=BBs[continent][1];
						ps->BB_S=BBs[continent][2];
						ps->BB_W=BBs[continent][3];
					}

					static int numLoopCalls = 0;

					ps->takenInMonth = (LocalParameter::takenInMonth<0 ? -1 : takenInMonth);
					ps->takenInYear = (LocalParameter::takenInYear<0 ? -1 : takenInYear);
					ps->isWeekend = (LocalParameter::isWeekend<0 ? -1 : isWeekend);
					ps->inBoundingBox = (LocalParameter::takenOnContinent<0 ? -1 : 1);

					std::stringstream header;
					header<<"W:"<<(LocalParameter::isWeekend<0?-1:isWeekend)<<"/";
					header<<"M:"<<(LocalParameter::takenInMonth<0?-1:takenInMonth)<<"/";
					header<<"Y:"<<(LocalParameter::takenInYear<0?-1:takenInYear)<<"/";
					header<<"C:"<<(LocalParameter::takenOnContinent<0?"-1":continents[continent]);

					std::map<int,double> *scores = computeScores(ps, ind, termids, LocalParameter::exampleOutFile, header.str());

					//print out the output file "header" (terms)
					if(numLoopCalls==0)
					{
						for(int k=0; k<termids.size(); k++)
							out << ind->term(termids[k])<<" ";
						out<<std::endl;
					}
					numLoopCalls++;

					std::cerr << "Writing " << termids.size() << " entries to file"
							<< std::endl;

					out<<header.str()<<" ";

					for (int j = 0; j < termids.size(); j++)
					{
						std::map<int, double>::iterator it = scores->find(termids[j]);
						if (it == scores->end())
						{
							out << "- ";
							continue;
						}
						out << it->second << " ";
					}
					out << std::endl;
					delete scores;
				}
			}
		}
	}
	delete ind;
	out.close();
}

