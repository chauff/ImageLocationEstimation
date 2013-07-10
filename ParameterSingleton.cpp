#include "ParameterSingleton.hpp"

lemur::extra::ParameterSingleton* lemur::extra::ParameterSingleton::paramSingleton;

lemur::extra::ParameterSingleton::ParameterSingleton(){ }

lemur::extra::ParameterSingleton* lemur::extra::ParameterSingleton::getInstance()
{
	if(paramSingleton==NULL)
		paramSingleton = new lemur::extra::ParameterSingleton();
	return paramSingleton;
}

/*
 * this method parses the field names
 */
void lemur::extra::ParameterSingleton::parseFields(std::string fields)
{
	char *p = strtok((char*)fields.c_str(), " ");
	while (p)
	{
		std::string s = p;
	    GN_FIELDS.push_back(s);
	    p = strtok(NULL, " ");
	}
	std::cerr<<"Number of fields recognized: "<<GN_FIELDS.size()<<std::endl;
}

/*
 * this method parses the fields' corresponding mixing values
 */
void lemur::extra::ParameterSingleton::parseAlphas(std::string alphas)
{
	char *p = strtok((char*)alphas.c_str(), " ");
	double sum = 0.0;
	while (p)
	{
		double m = atof(p);
	    GN_ALPHAS.push_back(m);
	    sum += m;
	    p = strtok(NULL, " ");
	}

	//sanity check - should add up to 1
	if(sum!=1.0)
		std::cerr<<"Warning: ALPHA values should add up to 1, currently sum="<<sum<<std::endl;
}


