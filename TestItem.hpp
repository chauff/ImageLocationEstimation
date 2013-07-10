#ifndef TESTITEM_HPP_
#define TESTITEM_HPP_

namespace lemur
{
	namespace extra
	{
		class TestItem
		{
			public:
				TestItem()
				{
					distToGroundTruth=-1.0;
				};

				int docid;
				double latitude;
				double longitude;
				int monthTaken;
				int yearTaken;
				double distToGroundTruth;
				std::string user;
		};
	}
}

#endif
