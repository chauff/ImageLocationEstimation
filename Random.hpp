#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <iostream>
#include <random>
#include "ParameterSingleton.hpp"

namespace lemur
{
	namespace extra
	{
		class Random
		{
			public:
				double getRandomNumber();
				void addKMToError(double km);
				void printAverageError();
				static Random* getInstance();
				static Random* getInstance(double mean, double stdev);
			private:
				std::random_device rd;
				std::default_random_engine* generator;
				std::normal_distribution<double>* nd;
				static Random* random;
				Random(double m, double s);
				lemur::extra::ParameterSingleton *ps;
	
				double numErroneousItems;
				double sumErrors;
		};
	}
}

#endif
