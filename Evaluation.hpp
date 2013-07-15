#ifndef EVALUATION_HPP_
#define EVALUATION_HPP_

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include "TestItem.hpp"

namespace lemur
{
	namespace extra
	{
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
				void readResultFile();
		};
	}
}

#endif
