#include "TestItem.hpp"

lemur::extra::TestItem::TestItem(int did) : lemur::extra::GeoDoc(did)
{
	distToGroundTruth=-1.0;
	estimatedLatitude=-1.0;
	estimatedLongitude=-1.0;
}
