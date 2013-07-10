#ifndef EVALUATION_HPP_
#define EVALUATION_HPP_

#include "common_headers.hpp"
#include "indri/Repository.hpp"
#include "indri/QueryEnvironment.hpp"
#include "TermDistributionFilter.hpp"
#include "TestItem.hpp"
#include "Metadata.hpp"
#include "ParameterSingleton.hpp"

namespace lemur
{
	namespace extra
	{
		class ParameterSingleton;

		class Evaluation
		{
			public:
				Evaluation();
				bool isTestItem(int docid);
				std::vector<int>* getTestDocids();
				TestItem* getTestItem(int docid);
				double evaluateTestItem(int docid, double latitude, double longitude);
				double getLatitude(int docid);
				double getLongitude(int docid);
				void writeResultsToFile(std::string additionalInfo);
				std::string printStatistics();
				std::set<std::string>* getTestUsers();

			private:
				std::map<int,TestItem*> *testMap;
				lemur::extra::ParameterSingleton *ps;
				std::stringstream resultStream;
				double getCoordinates(int docid, int type);
		};
	}
}

#endif
