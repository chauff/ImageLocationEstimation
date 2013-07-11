#ifndef TERMDISTRIBUTIONFILTER_HPP_
#define TERMDISTRIBUTIONFILTER_HPP_

#include <map>
#include "ParameterSingleton.hpp"
#include "Metadata.hpp"
#include "math.h"

namespace lemur
{
	namespace extra
	{
		class GeoDoc;
		class GeoNode;
		class Evaluation;
		class ParameterSingleton;

		class TermDistributionFilter
		{
			public:
				static TermDistributionFilter* getInstance();
				bool isTermGeographic(int termid);
				double getGeographicScore(int termid);
				bool isTermInGeneralUse(int termid);
				std::map<int,double> *evaluatedGeoTerms;
			private:
				std::map<int,bool> *evaluatedGeneralTerms;
				TermDistributionFilter();
				static TermDistributionFilter* filterInstance;
				lemur::extra::ParameterSingleton *ps;
		};
	}
}

#endif
