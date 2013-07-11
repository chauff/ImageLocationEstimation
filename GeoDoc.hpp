#ifndef GEODOC_HPP_
#define GEODOC_HPP_

#include <map>
#include "ParameterSingleton.hpp"
#include "Metadata.hpp"
#include "TermDistributionFilter.hpp"

namespace lemur{
	namespace extra{


class GeoDoc
{
	public:
		int docid;
		double latitude;
		double longitude;
		long timetaken;
		int takenInMonth;
		int takenInYear;

		std::map<std::string, std::map<int,int>* > *fieldTerms;

		GeoDoc(int did);
		std::map<int, int>* getTerms(std::string field);

		//for the oracle experiments, we also allow a manual change of the tags
		void setTerms(std::string field,std::map<int,int> *terms);
		void clearStoredTerms(std::string field);
		int getNumTerms();
		void printTerms();

	private:
		std::map<std::string, bool> *termsExtracted;
		lemur::extra::ParameterSingleton *ps;
};
	}}

#endif
