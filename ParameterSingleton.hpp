#ifndef PARAMETERSINGLETON_HPP_
#define PARAMETERSINGLETON_HPP_

#include "IndexManager.hpp"
#include "common_headers.hpp"
#include "BasicDocStream.hpp"
#include "RetMethodManager.hpp"
#include "RelDocUnigramCounter.hpp"
#include "indri/Repository.hpp"
#include "indri/QueryEnvironment.hpp"
#include "GeoNode.hpp"


namespace lemur
{
	namespace extra
	{
		class Evaluation;
		class GeoNode;

		class ParameterSingleton
		{
			public:
				//parameters for many objects
				indri::collection::Repository *repos;
				lemur::api::Index *ind;
				indri::api::QueryEnvironment *env;
				lemur::extra::Evaluation *eval;
				lemur::extra::GeoNode *rootNode;
				lemur::langmod::MLUnigramLM *colLM;
				std::set<int> *trainingSet;

				//parameters for GeoNode objects
				int GN_SPLIT_NUM;
				double GN_MIN_LATITUDE_STEP;
				double GN_MIN_LONGITUDE_STEP;
				double geoTermThreshold;
				int generalUseTermFilter;

				int testDataInMonth;
				int testDataInYear;
				int geoFilterByMonth;
				int geoFilterByYear;
				bool exludeTestUsers;
				std::string testFile;
				std::string priorFile;
				std::string resultFile;
				std::string paramFile;
				double smoothingParam;
				double dirParamNN;
				double defaultLatitude;
				double defaultLongitude;
				int higherLevel;
				int skippingModulos;

				std::vector<double> GN_ALPHAS;
				std::vector<std::string> GN_FIELDS;

				static ParameterSingleton* getInstance();
				void parseFields(std::string fields);
				void parseAlphas(std::string alphas);

			private:
				static ParameterSingleton* paramSingleton;
				ParameterSingleton();


		};
	}
}

#endif
