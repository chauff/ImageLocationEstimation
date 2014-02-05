#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <random>

namespace lemur
{
	namespace extra
	{
		class Random
		{
			public:
				double getRandomNumber();
				static Random* getInstance();
				static Random* getInstance(double mean, double stdev);
			private:
				std::random_device rd;
				std::default_random_engine* generator;
				std::normal_distribution<double>* nd;
				static Random* random;
				Random();
		};
	}
}

#endif
