#include "Metadata.hpp"



int lemur::extra::Metadata::getMonth(std::string stime)
{
	time_t tt = atol(stime.c_str());
	struct tm *date = localtime(&tt);
	int month = date->tm_mon + 1;
	return month;
}

int lemur::extra::Metadata::getYear(std::string stime)
{
	time_t tt = atol(stime.c_str());
	struct tm *date = localtime(&tt);
	int year = date->tm_year + 1900;
	return year;
}


std::string lemur::extra::Metadata::getUser(int docid)
{
	return get(docid, 1);
}

std::string lemur::extra::Metadata::getTimeTaken(int docid)
{
	return get(docid, 2);
}

std::string lemur::extra::Metadata::getLatitude(int docid)
{
	return get(docid, 3);
}

std::string lemur::extra::Metadata::getLongitude(int docid)
{
	return get(docid, 4);
}

std::string lemur::extra::Metadata::getAccuracy(int docid)
{
	return get(docid, 5);
}

std::string lemur::extra::Metadata::getUserLocation(int docid)
{
	return get(docid, 6);
}

std::vector<std::string>* lemur::extra::Metadata::getUsers(std::vector<int>* docids)
{
	return get(docids, 1);
}

 std::vector<std::string>* lemur::extra::Metadata::getTimeTakens(std::vector<int>* docids)
{
	return get(docids, 2);
}

std::vector<std::string>* lemur::extra::Metadata::getLatitudes(std::vector<int>* docids)
{
	return get(docids, 3);
}

std::vector<std::string>* lemur::extra::Metadata::getLongitudes(std::vector<int>* docids)
{
	return get(docids, 4);
}

std::vector<std::string>* lemur::extra::Metadata::getAccuracies(std::vector<int>* docids)
{
	return get(docids, 5);
}

std::set<std::string>* lemur::extra::Metadata::getUniqueUsers(std::vector<int>* docids)
{
	return getSet(docids, 1);
}

/*
 * given a single document, return its metadata element
 */
std::string lemur::extra::Metadata::get(int docid, int type)
{
	static vector<int> tmp;
	if (tmp.size() == 0)
		tmp.push_back(0);
	tmp[0] = docid;

	std::string s;
	if (type == 1)
		s = "userid";
	if(type==2)
		s = "timetaken";
	if(type==3)
		s = "latitude";
	if(type==4)
		s = "longitude";
	if(type==5)
		s = "acccuracy";
	if(type==6)
		s = "userlocation";

	lemur::extra::ParameterSingleton *ps = lemur::extra::ParameterSingleton::getInstance();
	std::vector<std::string> svec = ps->env->documentMetadata(tmp, s);
	return svec[0];
}


/*
 * given a vector of documents, return the vector of their metadata elements
 */
std::vector<std::string>* lemur::extra::Metadata::get(std::vector<int> *docids, int type)
{
	std::string s;
	if (type == 1)
		s = "userid";
	if(type==2)
		s = "timetaken";
	if(type==3)
		s = "latitude";
	if(type==4)
		s = "longitude";
	if(type==5)
		s = "accuracy";
	if(type==6)
		s = "userlocation";

	lemur::extra::ParameterSingleton *ps = lemur::extra::ParameterSingleton::getInstance();
	std::vector<std::string> svec = ps->env->documentMetadata(*docids, s);
	std::vector<std::string> *vec = new std::vector<std::string>;
	for(int i=0; i<svec.size(); i++)
		vec->push_back(svec[i]);
	return vec;
}


std::set<std::string>* lemur::extra::Metadata::getSet(std::vector<int> *docids, int type)
{
	std::vector<std::string> *vec = get(docids,type);
	std::set<std::string> *s = new std::set<std::string>;
	for(int i=0; i<vec->size(); i++)
		s->insert(vec->at(i));
	delete vec;
	return s;
}
