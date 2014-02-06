#include "Random.hpp"

lemur::extra::Random* lemur::extra::Random::random;

lemur::extra::Random::Random(double mean, double stdev){

	ps = lemur::extra::ParameterSingleton::getInstance();
	generator = new std::default_random_engine( rd() );
	nd = new std::normal_distribution<double>(mean, stdev);
}

double lemur::extra::Random::getRandomNumber() {
	double r = (*nd)(*generator);
	return r;
}

lemur::extra::Random* lemur::extra::Random::getInstance(double mean, double stdev)
{
	if(random==NULL)
		random = new lemur::extra::Random(mean, stdev);
	return random;
}

lemur::extra::Random* lemur::extra::Random::getInstance()
{
	if(random==NULL) {
		std::cerr<<"Error: cannot instantiate Random object without mean/stdev"<<std::endl;
		exit;
	}
	return random;
}

void lemur::extra::Random::addKMToError(double km) {
	numErroneousItems += 1.0;
	sumErrors += km;
}

void lemur::extra::Random::printAverageError() {
	double av = sumErrors / numErroneousItems;
	std::cerr<<"Average error introduced: "<<av<<std::endl;
}	
