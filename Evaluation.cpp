#include "Evaluation.hpp"
#include "Distance.hpp"
#include "ParameterSingleton.hpp"

/*
 * the testfile is expected to be in the format <externalID> <latitude> <longitude>;
 * if excludeTestUsers=true in the training data all items are ignored that are by users that contribute to the test set
 */

lemur::extra::Evaluation::Evaluation()
{
	ps = lemur::extra::ParameterSingleton::getInstance();

	testMap = new std::map<int,TestItem*>();

	//read test data
	ifstream in(ps->testFile.c_str());
	std::cerr<<"Reading test data from "<<ps->testFile<<std::endl;
	if(in.is_open()==false)
	{
		std::cerr<<"Cannot read test file!"<<std::endl;
		exit(1);
	}

	std::string sDocid = "";

	while(in>>sDocid)
	{
		int docid = ps->ind->document(sDocid);
		if(docid<=0)
			continue;

		TestItem *ti = new TestItem(docid);
		ti->latitude=atof(lemur::extra::Metadata::getLatitude(docid).c_str());
		ti->longitude=atof(lemur::extra::Metadata::getLongitude(docid).c_str());;
		ti->user = lemur::extra::Metadata::getUser(docid);
		std::string stime = lemur::extra::Metadata::getTimeTaken(docid);
		ti->takenInMonth = lemur::extra::Metadata::getMonth(stime);
		ti->takenInYear = lemur::extra::Metadata::getYear(stime);
		testMap->insert(std::make_pair(docid,ti));
	}
}



std::set<std::string>* lemur::extra::Evaluation::getTestUsers()
{
	std::set<std::string> *users = new std::set<std::string>;
	for(std::map<int,TestItem*>::iterator it=testMap->begin(); it!=testMap->end(); it++)
		users->insert( (*it).second->user );
	return users;
}

lemur::extra::TestItem* lemur::extra::Evaluation::getTestItem(int docid)
{
	return testMap->find(docid)->second;
}


bool lemur::extra::Evaluation::isTestItem(int docid)
{
	return (testMap->find(docid)==testMap->end() ? false : true);
}

std::vector<int>* lemur::extra::Evaluation::getTestDocids()
{
	std::vector<int> *vec = new std::vector<int>;
	for(std::map<int,TestItem*>::iterator it=testMap->begin(); it!=testMap->end(); it++)
		vec->push_back( (*it).first );
	return vec;
}


double lemur::extra::Evaluation::evaluateTestItem(int docid, double latitude, double longitude)
{
	std::map<int,TestItem*>::iterator it = testMap->find(docid);
	if(it==testMap->end())
	{
		std::cerr<<"evaluateItem() called for a docid that is not in the test set"<<std::endl;
		exit(1);
	}

	TestItem *ti = (*it).second;

	//ground truth latitude/longitude
	double gtLatitude = ti->latitude;
	double gtLongitude = ti->longitude;

	double km = -1.0;
	if(latitude>500 && longitude>500)
		;
	else
		km = Distance::getKM(gtLatitude, gtLongitude, latitude, longitude);

	ti->distToGroundTruth=km;
	
	resultStream<<docid<<" gtLat:"<<gtLatitude<<" gtLng:"<<gtLongitude<<" lat:"<<latitude<<" lng:"<<longitude<<" dist:"<<km<<" sDocid:"<<ps->ind->document(docid)<<std::endl;
	return km;
}


double lemur::extra::Evaluation::getCoordinates(int docid, int type)
{
	std::map<int,TestItem*>::iterator it = testMap->find(docid);
	//check if the item is in the test set
	if( it == testMap->end() )
	{
		std::cerr<<"getLatitude() called for a docid that is not in the test set"<<std::endl;
		exit(1);
	}

	if(type==1)
		return (*it).second->latitude;
	else
		return (*it).second->longitude;
}


double lemur::extra::Evaluation::getLatitude(int docid)
{
	return getCoordinates(docid,1);
}

double lemur::extra::Evaluation::getLongitude(int docid)
{
	return getCoordinates(docid,0);
}


std::string lemur::extra::Evaluation::printStatistics()
{
	std::stringstream ss;

	ss<<"Total number of test items: "<<testMap->size()<<std::endl;
	ss<<"Number of evaluated test items: "<<testMap->size()<<std::endl;

	//from 10m, 50m, 100m, 1km ... 1000k,
	double evalMeasures[] = {0.01, 0.05, 0.1, 1, 10, 50, 100, 1000};
	int arrayLen = 8;
	double evalCounter[arrayLen];
	for(int i=0; i<arrayLen; i++)
		evalCounter[i]=0;

	double counter = 0.0;

	std::vector<double> distVec;

	for(std::map<int,TestItem*>::iterator it = testMap->begin(); it!=testMap->end(); it++)
	{
		TestItem *ti = (*it).second;
		if(ti->distToGroundTruth<0)
			continue;
		
		counter += 1.0;
		distVec.push_back(ti->distToGroundTruth);
		for(int i=0; i<arrayLen; i++)
		{	
			if(ti->distToGroundTruth<=evalMeasures[i])
				evalCounter[i]+=1.0;
		}
	}

	ss<<"++++ evaluation +++ "<<std::endl;
	for(int i=0; i<arrayLen; i++)
		ss<<"Accuracy "<<evalMeasures[i]<<"KM: "<<( 100.0 * evalCounter[i]/counter)<<"% "<<evalCounter[i]<<"/"<<counter<<std::endl;

	//sort the error distances to to get to the median error
	sort(distVec.begin(),distVec.end());
	double medianError = -1.0;

	if(distVec.size()%2==1)
	{
		int medianIndex = (distVec.size()-1)/2;
		if(medianIndex>0)
			medianError = distVec[medianIndex];
	}
	else
	{
		int medianIndex1 = (distVec.size())/2;
		int medianIndex2 = medianIndex1-1;
		if(medianIndex2>=0)
			medianError = 0.5 * (distVec[medianIndex1]+distVec[medianIndex2]);
	}

	ss<<"Median error in KM: "<<medianError<<std::endl;
	ss<<"(size of distVec: "<<distVec.size()<<")"<<std::endl;

	std::cerr<<ss.str()<<std::endl;
	return ss.str();

}

void lemur::extra::Evaluation::writeResultsToFile(std::string additionalInfo="")
{
	ofstream out(ps->resultFile.c_str());

	if(out.is_open()==false)
	{
		std::cerr<<"Unable to open "<<ps->resultFile<<" for writing"<<std::endl;
		exit(1);
	}

	ifstream in(ps->paramFile.c_str());
	if(in.is_open()==false)
	    std::cerr<<"Warning: unable to add parameter entries to "<<ps->resultFile<<std::endl;
	else
	{
		std::string line;
		while (std::getline(in, line))
		    out<<line<<std::endl;
		in.close();
	}

	out<<std::endl;
	out<<additionalInfo<<std::endl;
	out<<printStatistics()<<std::endl;
	out<<resultStream.str()<<std::endl;
	out.close();
}
