#ifndef METADATA_HPP_
#define METADATA_HPP_

#include "common_headers.hpp"
#include "indri/Repository.hpp"
#include "indri/QueryEnvironment.hpp"
#include "TermDistributionFilter.hpp"
#include "TestItem.hpp"

namespace lemur
{
	namespace extra
	{
		class Metadata
		{
			public:
				static std::string getUser(int docid);
				static std::string getTimeTaken(int docid);
				static std::string getLatitude(int docid);
				static std::string getLongitude(int docid);
				static int getMonth(std::string s);
				static int getYear(std::string s);

				static std::vector<std::string>* getUsers(std::vector<int>* docids);
				static std::vector<std::string>* getTimeTakens(std::vector<int>* docids);
				static std::set<std::string>* getUniqueUsers(std::vector<int>* docids);
				static std::vector<std::string>* getLatitudes(std::vector<int>* docids);
				static std::vector<std::string>* getLongitudes(std::vector<int>* docids);

				static std::string get(int docid, int type);
				static std::vector<std::string>* get(std::vector<int>* docids, int type);
				static std::set<std::string>* getSet(std::vector<int> *docids, int type);
		};
	}
}

#endif
