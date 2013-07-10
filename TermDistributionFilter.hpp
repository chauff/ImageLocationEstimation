#ifndef TERMDISTRIBUTIONFILTER_HPP_
#define TERMDISTRIBUTIONFILTER_HPP_

#include <set>
#include "GeoDoc.hpp"
#include "GeoNode.hpp"
#include "Evaluation.hpp"
#include "Metadata.hpp"
#include "ParameterSingleton.hpp"

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
