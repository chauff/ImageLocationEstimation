#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

namespace lemur
{
	namespace extra
	{
		class Util
		{
			public:
				static std::vector<std::string>* tokenizeString(std::string s, char delim=' ',bool toLower=false);
				static std::string eraseChar(std::string s, char delim=' ');
				static void toLower(std::string);
		};
	}
}

#endif
