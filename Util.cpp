#include "Util.hpp"

std::vector<std::string>* lemur::extra::Util::tokenizeString(std::string s, char delimiter, bool tolower)
{
    std::vector<std::string> *results = new std::vector<std::string>;

    size_t prev = 0;
    size_t next = 0;

    while ((next = s.find_first_of(delimiter, prev)) != std::string::npos)
    {
        if(next - prev != 0)
        {
        	std::string sub = s.substr(prev, next - prev);
        	if(tolower)
        		toLower(sub);
        	results->push_back(sub);
        }
        prev = next + 1;
    }

    if (prev < s.size())
    {
    	std::string sub = s.substr(prev);
    	if(tolower)
    		toLower(sub);
        results->push_back(sub);
    }
    return results;
}

std::string lemur::extra::Util::eraseChar(std::string s, char c)
{
	std::stringstream ss;
	for(int i=0; i<s.length(); i++)
		if(s[i]!=c)
			ss<<s[i];
	return ss.str();
}

void lemur::extra::Util::toLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}
