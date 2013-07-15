#ifndef TESTITEM_HPP_
#define TESTITEM_HPP_

#include "GeoDoc.hpp"

namespace lemur
{
	namespace extra
	{
		class TestItem : public GeoDoc
		{
		public:
			TestItem(int did);
			double distToGroundTruth;
			std::string user;
			double estimatedLatitude;
			double estimatedLongitude;
		};
	}
}

#endif
