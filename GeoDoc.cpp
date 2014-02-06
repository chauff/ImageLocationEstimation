#include "GeoDoc.hpp"

lemur::extra::GeoDoc::GeoDoc(int did, bool addNoise)
{
	ps = lemur::extra::ParameterSingleton::getInstance();
	docid = did;

	termsExtracted = new std::map<std::string, bool>;
	fieldTerms = new std::map<std::string, std::map<int,int>*>;

	std::vector<std::string>& fields = ps->GN_FIELDS;
	for(int i=0; i<ps->GN_FIELDS.size(); i++)
	{
		termsExtracted->insert(std::make_pair(ps->GN_FIELDS[i],false));
		std::map<int,int> *termMap = new std::map<int,int>;
		fieldTerms->insert(std::make_pair(ps->GN_FIELDS[i], termMap));
	}

	latitude = atof(Metadata::getLatitude(docid).c_str());
	longitude = atof(Metadata::getLongitude(docid).c_str());

	//add some noise here
	//but only if this is a training item (addNoise is set to true)
	//and only if the randomly generated numbers falls in the interval 0-(percentage-1)
	if(ps->locNoise == true && addNoise == true && ( rand()%100 < ps->locNoisePercentage ) ) {

		double d1, d2;
		int attempts = 0;
		do {
			//the parameters mean/stdev only need to be set once across all calls,
			//but for simplicity just added whenever the first call for an object is made
			d1 = lemur::extra::Random::getInstance(ps->locMean,ps->locStdev)->getRandomNumber();
		} while( fabs(latitude+d1) > 90 && attempts++ < 5);

		do {
			d2 = lemur::extra::Random::getInstance()->getRandomNumber();
		} while( fabs(longitude+d2) > 180 && attempts++ < 10);

		
		double errorInKM = lemur::extra::Distance::getKM(latitude, longitude, latitude+d1, longitude+d2);
		lemur::extra::Random::getInstance()->addKMToError(errorInKM);

		latitude += d1;
		longitude += d2;
	}

	if ( abs(latitude)>90 || abs(longitude)>180)
	{
		std::cerr << "Invalid latitude (max. +-90) or longitude (max. +-180): "<<latitude<<" / "<<longitude<<std::endl;
		std::cerr << "Exiting program.";
		exit(1);
	}
}


/**
 * this method can be used to reset the stored information about terms
 */
void lemur::extra::GeoDoc::clearStoredTerms(std::string field)
{
	if(termsExtracted->find(field)!=termsExtracted->end())
	{
		termsExtracted->insert(std::make_pair(field, false));
		fieldTerms->find(field)->second->clear();
	}
}

//get the content of a field in a hashmap: key is termID, value is term frequency in the field
std::map<int, int>* lemur::extra::GeoDoc::getTerms(std::string field)
{
	if(termsExtracted->find(field)->second == true)
		return fieldTerms->find(field)->second;

	termsExtracted->insert(std::make_pair(field, true));

	std::map<int,int> *termMap = fieldTerms->find(field)->second;

	indri::collection::Repository::index_state repIndexState = ps->repos->indexes();
	indri::index::Index *index = (*repIndexState)[0];

	lemur::extra::TermDistributionFilter *filter =
			lemur::extra::TermDistributionFilter::getInstance();

	int fieldID = index->field(field);
	if (fieldID < 1)
	{
		std::cerr << "Invalid field name ["<<field<<"]; it yields ID " << fieldID
				<< std::endl;
		return termMap;
	}

	if(docid<1)
		return termMap;

	const indri::index::TermList *tlist = index->termList(docid);
	if (!tlist)
	{
		std::cerr << "Invalid term list for document with internal ID "
				<< docid << std::endl;
		return termMap;
	}

	indri::utility::greedy_vector < indri::index::FieldExtent > fieldVec
			= tlist->fields();
	indri::utility::greedy_vector<indri::index::FieldExtent>::iterator fIt =
			fieldVec.begin();
	while (fIt != fieldVec.end())
	{
		if ((*fIt).id == fieldID)
		{
			int beginTerm = (*fIt).begin;
			int endTerm = (*fIt).end;

			for (int t = beginTerm; t < endTerm; t++)
			{
				int termID = tlist->terms()[t];
				if (termID <= 0)
					continue;

				bool validTerm1 = filter->isTermGeographic(termID);
				bool validTerm2 = filter->isTermInGeneralUse(termID);
				bool validTerm3 = (ps->colLM->prob(termID)>0) ? true : false;

				if (validTerm1 == false || validTerm2 == false || validTerm3 == false)
					continue;

				std::map<int, int>::iterator tIt = termMap->find(termID);
				//we assume each term appears at most once per field
				termMap->insert(std::make_pair(termID, 1));
			}
		}
		fIt++;
	}
	delete tlist;

	return termMap;
}

/**
 * clear out the current terms, copy the ones in the given term set (for the oracle experiments) or
 * for instance if the home location should be used instead of the empty tags/title
 */
void lemur::extra::GeoDoc::setTerms(std::string field, std::map<int, int> *manuallySetTerms)
{
	std::map<int,int> *termMap = fieldTerms->find(field)->second;
	termMap->clear();
	termsExtracted->insert(std::make_pair(field, true));

	for (std::map<int, int>::iterator it = manuallySetTerms->begin(); it
			!= manuallySetTerms->end(); it++)
		termMap->insert(std::make_pair(it->first, it->second));
}


int lemur::extra::GeoDoc::getNumTerms()
{
	int numTerms=0;
	for(int i=0; i< ps->GN_FIELDS.size(); i++)
	{
		int n =getTerms(ps->GN_FIELDS[i])->size();
		numTerms+=n;
	}
	return numTerms;
}

void lemur::extra::GeoDoc::printTerms()
{
	std::cerr<<"Overview of terms in document "<<ps->ind->document(this->docid)<<std::endl;
	int numTerms=0;
	for(int i=0; i< ps->GN_FIELDS.size(); i++)
	{
		std::map<int,int> *termMap = getTerms(ps->GN_FIELDS[i]);
		std::cerr<<"field="<<ps->GN_FIELDS[i]<<" ";
		for(std::map<int,int>::iterator termIt=termMap->begin(); termIt!=termMap->end(); termIt++)
		{
			std::cerr<<"["<<ps->ind->term(termIt->first)<<"] (tf "<<ps->ind->termCount(termIt->first)<<") ";
		}
		std::cerr<<std::endl;
	}
}
