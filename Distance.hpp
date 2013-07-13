#ifndef DISTANCE_HPP_
#define DISTANCE_HPP_

#include <math.h>
#include <iostream>

#define _pi 3.14159265358979323846

namespace lemur
{
		namespace extra
		{
			class Distance
			{
				public:

					static double deg2rad(double deg)
					{
					  return (deg * _pi / 180);
					}

					static double rad2deg(double rad)
					{
					  return (rad * 180 / _pi);
					}

					/**
					 * distance between 2 points (haverseine)
					 */
					static double getKM(double lat1, double lon1, double lat2, double lon2)
					{
						if(lat1>90||lat1<-90||lon1>180||lon1<-180||lat2>90||lat2<-90||lon2>180||lon2<-180)
						{
							std::cerr<<"getKM() expects valid latitude/longitude pairs!"<<std::endl;
							std::cerr<<"-90<=latitude<=90 -180<=longitude<=180"<<std::endl;

							std::cerr<<"lat1: "<<lat1<<", lat2: "<<lat2<<std::endl;
							std::cerr<<"lon1: "<<lon1<<", lon2: "<<lon2<<std::endl;
							exit(1);
						}

						//source: http://blog.julien.cayzac.name/2008/10/arc-and-distance-between-two-points-on.html

						double DEG_TO_RAD = 0.017453292519943295769236907684886;
						double EARTH_RADIUS_IN_METERS = 6372797.560856;

						double latitudeArc  = (lat1 - lat2) * DEG_TO_RAD;
						double longitudeArc = (lon1 - lon2) * DEG_TO_RAD;
						double latitudeH = sin(latitudeArc * 0.5);
						latitudeH *= latitudeH;
						double lontitudeH = sin(longitudeArc * 0.5);
						lontitudeH *= lontitudeH;
						double tmp = cos(lat1*DEG_TO_RAD) * cos(lat2*DEG_TO_RAD);
						return (EARTH_RADIUS_IN_METERS * (2.0 * asin(sqrt(latitudeH + tmp*lontitudeH)))/1000.0);
					}
			};
		}
}

#endif
