#include "IndexManager.hpp"
#include "common_headers.hpp"
#include "BasicDocStream.hpp"
#include "RetMethodManager.hpp"
#include "ResultFile.hpp"
#include "RelDocUnigramCounter.hpp"
#include "indri/Repository.hpp"
#include "indri/QueryEnvironment.hpp"
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string>
#include <iostream>
#include <sstream>
#include "ParameterSingleton.hpp"
#include "Evaluation.hpp"
#include "Metadata.hpp"
#include "TermDistributionFilter.hpp"
#include "GeoNode.hpp"
#include "TestItem.hpp"
#include "Util.hpp"

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::extra;

namespace LocalParameter {
std::string resultFile;//write out the test results
std::string testFile;//this is where the test items are kept, format: testid latitude longitude (for quick eval)
std::string priorFile;//a prior can be used (e.g. population prior); format: <latitude> <longitude> <num> (num can be population); each (lat/long) is assigned to the respective GeoNode and <num> are added up; instead of a file the term POPULARITY indicates to compute the prior based on the distribution of the training data
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

int higherLevel;//at what level of the hierarchy does the first iteration take place
int skippingModulos;//instead of using everything for training, use %skippingModulos==0 documents

int testDataInMonth;//only use test data of month X
int testDataInYear;//only use test data of year y

int geoFilterByMonth;//geo-filter: only employ training data of month X
int geoFilterByYear;//geo-filter: only employ training data of month Y

int minAccuracy;//for training only items with min-X accuracy are considered
bool hasUserLocation;//if no textual meta-data is available, use the user's home location

void get() {
	//initialize the parameter object
	lemur::extra::ParameterSingleton *ps =
			lemur::extra::ParameterSingleton::getInstance();

	ps->resultFile = ParamGetString("resultFile");
	ps->testFile = ParamGetString("testFile");
	ps->priorFile = ParamGetString("priorFile");
	ps->smoothingParam = ParamGetDouble("smoothingParam", 0.5);
	ps->dirParamNN = ParamGetDouble("dirParamNN", 100);
	ps->GN_SPLIT_NUM = ParamGetInt("splitLimit", 5000);
	ps->GN_MIN_LONGITUDE_STEP = ParamGetDouble("minLongitudeStep", 0.01);
	ps->GN_MIN_LATITUDE_STEP = ParamGetDouble("minLatitudeStep", 0.01);
	std::string s = ParamGetString("excludeTestUsers", "false");
	ps->exludeTestUsers = (s.compare("false") == 0) ? false : true;

	ps->geoTermThreshold = ParamGetDouble("geoTermThreshold", -1.0);
	ps->generalUseTermFilter = ParamGetInt("generalUseTermFilter", -1);
	ps->defaultLatitude = ParamGetDouble("defaultLatitude", 0.0);
	ps->defaultLongitude = ParamGetDouble("defaultLongitude", 0.0);

	ps->parseAlphas(ParamGetString("alphas"));
	ps->parseFields(ParamGetString("fields"));

	ps->higherLevel = ParamGetInt("higherLevel", 5);
	ps->skippingModulos = ParamGetInt("skippingModulos", 1);

	ps->testDataInMonth = ParamGetInt("testDataInMonth", -1);
	ps->testDataInYear = ParamGetInt("testDataInYear", -1);

	ps->geoFilterByMonth = ParamGetInt("geoFilterByMonth", -1);
	ps->geoFilterByYear = ParamGetInt("geoFilterByYear", -1);

	minAccuracy = ParamGetInt("minAccuracy",-1);
	s = ParamGetString("useHomeLocation","false");
	hasUserLocation = (s.compare("false") == 0) ? false : true;
}
}
;

void GetAppParam() {
	RetrievalParameter::get();
	SimpleKLParameter::get();
	LocalParameter::get();
}

int AppMain(int argc, char* argv[]) {
	std::cerr << "Reading from parameter file " << argv[1] << std::endl;

	indri::collection::Repository repository;//indri: access to fields & co
	lemur::api::Index *ind;//conversion of external/internal docids, unidgramdoccounter
	indri::api::QueryEnvironment env;//Indri again

	std::cerr << "Opening index structures" << std::endl;
	try {
		repository.openRead(RetrievalParameter::databaseIndex);
		ind = IndexManager::openIndex(RetrievalParameter::databaseIndex);
		env.addIndex(RetrievalParameter::databaseIndex);
	} catch (Exception &e) {
		std::cerr << "Error when opening index ..." << std::endl;
		std::cerr << e.what() << std::endl;
		exit(1);
	}

	//initialize the evaluation and parameters
	lemur::extra::ParameterSingleton *ps =
			lemur::extra::ParameterSingleton::getInstance();
	ps->ind = ind;
	ps->repos = &repository;
	ps->env = &env;
	ps->paramFile = argv[1];

	lemur::extra::Evaluation eval;
	ps->eval = &eval;

	//we need a collection LM
	//if we exclude the test users, the colLM needs to be calculated separately
	lemur::langmod::DocUnigramCounter *colDC;
	lemur::langmod::MLUnigramLM *colLM;

	std::set < std::string > *testUsers = eval.getTestUsers();

	//test documents are not included in the collection LM
	std::vector<int> trainingDocids;
	std::set<int>* trainingSet = new std::set<int>;
	std::cerr << "determining training documents" << std::endl;
	int stepSize = 500000;
	for (int boundary = 1; boundary <= ind->docCount(); boundary += stepSize) {
		std::vector<int> tmp;
		for (int docid = boundary; docid < (boundary + stepSize) && docid
				<= ind->docCount(); docid++)
			tmp.push_back(docid);

		std::vector < std::string > *users = lemur::extra::Metadata::getUsers(
				&tmp);

		std::vector<std::string> *accuracies;
		if(LocalParameter::minAccuracy>0)
			accuracies = lemur::extra::Metadata::getAccuracies(&tmp);

		for (int i = 0; i < tmp.size(); i++) {
			int docid = tmp[i];

			//several conditions to check: skippingModulos, test item?, item by test user?
			if (docid % ps->skippingModulos != 0)
				continue;

			if (eval.isTestItem(docid))
				continue;

			if (testUsers->find(users->at(i)) != testUsers->end()
					&& ps->exludeTestUsers == true)
				continue;

			if(LocalParameter::minAccuracy>0)
			{
				int acc = atoi(accuracies->at(i).c_str());
				if(acc<LocalParameter::minAccuracy)
					continue;
			}
			trainingDocids.push_back(docid);
			trainingSet->insert(docid);
		}

		delete users;
		if(LocalParameter::minAccuracy>0)
			delete accuracies;
	}

	std::cerr << "Training documents in total: " << trainingDocids.size()
			<< std::endl;

	std::cerr << "Creating collection LM" << std::endl;
	colDC = new lemur::langmod::DocUnigramCounter(trainingDocids, *ind);
	colLM = new lemur::langmod::MLUnigramLM(*colDC, ind->termLexiconID());

	ps->colLM = colLM;
	ps->trainingSet = trainingSet;

	//the TermDistributionFilter is implemented as a singleton; first call initializes the necessary parameters
	lemur::extra::TermDistributionFilter *tdf =
			lemur::extra::TermDistributionFilter::getInstance();

	//create the tree consisting of GeoNode elements
	std::cerr << "Generating hierarchical region splits" << std::endl;
	GeoNode *rootNode = new GeoNode();
	int numAddedDocids = 0;
	for (int i = 0; i < trainingDocids.size(); i++) {
		int docid = trainingDocids.at(i);
		rootNode->addDocument(docid);
		numAddedDocids++;
	}

	ps->rootNode = rootNode;

	//create a hashmap with all leaf nodes (for easy iteration)
	std::set<GeoNode*> *leaves = new std::set<GeoNode*>();
	rootNode->fillSetWithLeaves(leaves);
	std::cerr << "Number of non-empty leaf nodes: " << leaves->size()
			<< std::endl;

	//create a hashmap with higher level nodes (X steps from root)
	std::set<GeoNode*> *higherLevelNodes = new std::set<GeoNode*>();
	rootNode->fillSetWithNodesAtLevel(higherLevelNodes, ps->higherLevel);
	std::cerr << "Number of higher level nodes: " << higherLevelNodes->size()
			<< std::endl;

	/*
	 * read the prior file and convert it to prior probabilities (or uniform prob. if no prior file)
	 * expected prior file format: <latitude> <longitude> <score>
	 * all scores for a region are added up, normalized;
	 *
	 * if no file is found, if the "filename" is POPULARITY, a prior based on the number of
	 * training documents in the region is formed, otherwise the prior is uniform
	 */

	std::map<GeoNode*, double> *priors;
	std::map<GeoNode*, double> leavePrior;
	for (std::set<GeoNode*>::iterator it = leaves->begin(); it != leaves->end(); it++)
		leavePrior.insert(std::make_pair(*it, 1));

	std::map<GeoNode*, double> higherLevelPrior;
	for (std::set<GeoNode*>::iterator it = higherLevelNodes->begin(); it
			!= higherLevelNodes->end(); it++)
		higherLevelPrior.insert(std::make_pair(*it, 1));

	double priorLat, priorLng, priorNum;
	ifstream priorIn(ps->priorFile.c_str());
	if (priorIn.is_open() == false) {
		std::cerr << "Unable to read prior file " << ps->priorFile << std::endl;

		if (ps->priorFile.compare("POPULARITY") == 0) {

			std::cerr<<"Using the popularity of the training data as prior!"<<std::endl;

			leavePrior.clear();
			higherLevelPrior.clear();
			for (std::set<GeoNode*>::iterator it = leaves->begin(); it
					!= leaves->end(); it++)
				leavePrior.insert(std::make_pair(*it,
						(*it)->childrenDocids->size()));

			for (std::set<GeoNode*>::iterator it = higherLevelNodes->begin(); it
					!= higherLevelNodes->end(); it++)
				higherLevelPrior.insert(std::make_pair(*it,
						(*it)->childrenDocids->size()));
		}
		else
			std::cerr<<"Using uniform prior probabilities"<<std::endl;

	} else {
		std::cerr << "Computing prior probabilities from " << ps->priorFile
				<< std::endl;

		double priorSum[2] = { 0.0, 0.0 };
		while (priorIn >> priorLat >> priorLng >> priorNum) {
			//add prior to both types of regions (higherLevel and leave)
			for (int i = 0; i < 2; i++) {
				priors = (i == 0) ? &higherLevelPrior : &leavePrior;

				//find the right GeoNode
				for (std::map<GeoNode*, double>::iterator priorIt =
						priors->begin(); priorIt != priors->end(); priorIt++) {
					GeoNode *n = priorIt->first;
					if (n->isGeoDocFitting(priorLat, priorLng) == true) {
						priorIt->second += priorNum;
						priorSum[i] += priorNum;
						break;
					}
				}
			}
		}
		priorIn.close();

		for (int i = 0; i < 2; i++) {
			priors = (i == 0) ? &higherLevelPrior : &leavePrior;
			for (std::map<GeoNode*, double>::iterator priorIt = priors->begin(); priorIt
					!= priors->end(); priorIt++)
				priorIt->second /= priorSum[i];
		}
	}

	//evaluating the test data
	std::vector<int> *testDocids = eval.getTestDocids();

	std::set<GeoNode*> *localLeaves = new std::set<GeoNode*>();

	int numDefaultLocations = 0;
	int numDocsTested = 0;

	for (int i = 0; i < testDocids->size(); i++) {
		int docid = testDocids->at(i);
		std::cerr << (i + 1) << ") evaluating docid=" << ind->document(docid)
				<< std::endl;

		if (ps->testDataInMonth > 0 || ps->testDataInYear) {
			TestItem *ti = eval.getTestItem(docid);
			if (ps->testDataInMonth > 0 && ti->takenInMonth
					!= ps->testDataInMonth)
				continue;

			if (ps->testDataInYear > 0 && ti->takenInYear != ps->testDataInYear)
				continue;
		}

		numDocsTested++;
		GeoDoc gd(docid);

		//first check, if there are actually terms in the test document
		int numTerms = gd.getNumTerms();

		if(numTerms==0 && LocalParameter::hasUserLocation==true)
		{
			//original user location, assumed format: "Los Angeles, USA, Earth"
			//preprocessing: removal of whitespaces and tokenization by commata + lowercasing
			std::string normalized = lemur::extra::Util::eraseChar(lemur::extra::Metadata::getUserLocation(docid),' ');
			std::vector<std::string>* homeTokens = lemur::extra::Util::tokenizeString(normalized,',',true);

			//check which terms are actually in the index
			std::map<int,int> termMap;
			for(int s=0; s<homeTokens->size(); s++)
			{
				int termid = ind->term(homeTokens->at(s));
				if(termid<=0)
					continue;

				int tf = 1;
				if(termMap.find(termid)!=termMap.end())
					tf += termMap.find(termid)->second;
				termMap.insert(std::make_pair(termid,1));
			}

			//if indexed terms are found, replace the (empty) "tags" term map
			if(termMap.size()>0)
			{
				gd.setTerms("tags",&termMap);
				numTerms = gd.getNumTerms();
			}
			delete homeTokens;
		}

		//if we are at zero terms, return the default location
		if (numTerms == 0) {
			eval.evaluateTestItem(docid, ps->defaultLatitude,
					ps->defaultLongitude);
			std::cerr << "Using default location, no meta-data found!"
					<< std::endl;
			numDefaultLocations++;
			continue;
		} else {
			std::cerr << "++++++ TAGS ++++++" << std::endl;
			std::cerr << "number of terms: " << numTerms << std::endl;
			std::map<int, int> *terms = gd.getTerms("tags");
			for (std::map<int, int>::iterator it = terms->begin(); it
					!= terms->end(); it++)
				std::cerr << "\t" << ind->term(it->first) << std::endl;
			std::cerr << "++++++" << std::endl;
		}

		localLeaves->clear();

		//2 iterations
		//	1. high level nodes
		//	2. leaf nodes
		for (int j = 0; j < 2; j++) {
			double maxLikelihood = -1 * DBL_MAX;
			GeoNode *maxNode = NULL;//store the best fitting node here
			priors = (j == 0) ? &higherLevelPrior : &leavePrior;

			std::set<GeoNode*> *nodes = ((j == 0) ? higherLevelNodes
					: localLeaves);

			if (nodes->size() <= 0) {
				std::cerr
						<< "Error: nodes within the loop should not be empty!"
						<< std::endl;
				std::cerr << "j=" << j << std::endl;
				std::cerr << "size higherLevelNodes: "
						<< higherLevelNodes->size() << std::endl;
				std::cerr << "size localLeaves: " << localLeaves->size()
						<< std::endl;
				continue;
			}

			for (std::set<GeoNode*>::iterator nodeIt = nodes->begin(); nodeIt
					!= nodes->end(); nodeIt++) {
				GeoNode *node = (*nodeIt);
				if (node->childrenDocids->size() == 0)
					continue;

				double p = node->getGenerationLikelihood(&gd, (j == 0) ? true
						: false);

				//prior information
				p = p + log(priors->find(node)->second);

				if (p > maxLikelihood) {
					maxLikelihood = p;
					maxNode = node;
				}
			}

			if (j == 0)
				maxNode->fillSetWithLeaves(localLeaves);
			//find the most similar item in the node with the largest probability
			else {
				GeoDoc *maxDoc = maxNode->getNearestNeighbour(&gd);
				double errorDist = eval.evaluateTestItem(docid,
						maxDoc->latitude, maxDoc->longitude);
				std::cerr << "estimated lat/lng: " << maxDoc->latitude << "/"
						<< maxDoc->longitude << " error (km): " << errorDist
						<< std::endl;
				if (i % 10 == 0 && i>0) {
					eval.writeResultsToFile("");
					eval.printStatistics();
				}
			}
		}
	}

	delete localLeaves;

	std::cerr << std::endl;
	std::cerr << "Evaluation finished!" << std::endl;

	std::stringstream ssInfo;
	ssInfo << "number of training documents: " << trainingDocids.size()
			<< std::endl;
	ssInfo << "number of test users: " << eval.getTestUsers()->size()
			<< std::endl;
	ssInfo << "number of higher-level nodes: " << higherLevelNodes->size()
			<< std::endl;
	ssInfo << "number of leaf nodes: " << leaves->size() << std::endl;
	ssInfo << "number of test documents: " << numDocsTested << std::endl;
	ssInfo << "number of test documents with default location: "
			<< numDefaultLocations << std::endl;

	//how many terms were filtered out?
	std::map<int, double> *geoTerms = tdf->evaluatedGeoTerms;
	int numFiltered = 0;
	for (std::map<int, double>::iterator it = geoTerms->begin(); it
			!= geoTerms->end(); it++) {
		if ((*it).second > ps->geoTermThreshold)
			numFiltered++;
	}
	int s = geoTerms->size();
	ssInfo << "[GEO-FILTER] number of unique terms in test documents: "
			<< ( (LocalParameter::geoTermThreshold<=0)?-1:s)
			<< std::endl;
	ssInfo
			<< "[GEO-FILTER] number of unique terms above geo threshold (i.e. filtered out): "
			<< ( (LocalParameter::geoTermThreshold<=0)?-1:numFiltered) << std::endl;

	eval.writeResultsToFile(ssInfo.str());

	delete ind;
}

