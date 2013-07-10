#include "TermDistributionFilter.hpp"


lemur::extra::TermDistributionFilter* lemur::extra::TermDistributionFilter::filterInstance;

lemur::extra::TermDistributionFilter::TermDistributionFilter()
{
	ps = lemur::extra::ParameterSingleton::getInstance();
	evaluatedGeoTerms = new std::map<int,double>();
	evaluatedGeneralTerms = new std::map<int,bool>();
}


lemur::extra::TermDistributionFilter* lemur::extra::TermDistributionFilter::getInstance()
{
	if(filterInstance==NULL)
		filterInstance = new lemur::extra::TermDistributionFilter();
	return filterInstance;
}


bool lemur::extra::TermDistributionFilter::isTermInGeneralUse(int termid)
{
	if(termid<=0)
		return false;

	if(ps->generalUseTermFilter<=0)
	{
		evaluatedGeneralTerms->insert(std::make_pair(termid,true));
		return true;
	}

	std::map<int,bool>::iterator evalIt = evaluatedGeneralTerms->find(termid);
	if(evalIt!=evaluatedGeneralTerms->end())
		return evalIt->second;

	//how many different users use the term?
	std::vector<int> docidVec;
	lemur::api::DocInfoList *dlist = ps->ind->docInfoList(termid);
	dlist->startIteration();
	while(dlist->hasMore())
	{
		lemur::api::DocInfo *info = dlist->nextEntry();
		if(ps->trainingSet->find(info->docID())!=ps->trainingSet->end())
			docidVec.push_back(info->docID());
	}
	delete dlist;

	std::set<std::string> *users = Metadata::getUniqueUsers(&docidVec);
	int numUsers = users->size();
	delete users;

	bool value = true;
	if(numUsers<ps->generalUseTermFilter)
		value = false;
	evaluatedGeneralTerms->insert(std::make_pair(termid,value));
	return value;
}

bool lemur::extra::TermDistributionFilter::isTermGeographic(int termid)
{
	if(termid<=0)
		return false;

	if(ps->geoTermThreshold<=0)
		return true;

	double d = getGeographicScore(termid);
	if(d>ps->geoTermThreshold)
		return false;
	return true;
}

double lemur::extra::TermDistributionFilter::getGeographicScore(int termid)
{
	if(termid<=0)
		return 1000.0;

	if(ps->geoTermThreshold<=0)
		return ps->geoTermThreshold-1;

	std::map<int,double>::iterator evalIt = evaluatedGeoTerms->find(termid);
	if(evalIt!=evaluatedGeoTerms->end())
		return evalIt->second;

	int grid[180][360];
	for(int i=0; i<180; i++)
		for(int j=0; j<360; j++)
			grid[i][j]=0.0;

	std::vector<int> docidVec;

	lemur::api::DocInfoList *dlist = ps->ind->docInfoList(termid);
	dlist->startIteration();
	while(dlist->hasMore())
	{
		lemur::api::DocInfo *info = dlist->nextEntry();
		if(ps->trainingSet->find(info->docID())!=ps->trainingSet->end())
			docidVec.push_back(info->docID());
	}
	delete dlist;

	if(ps->geoFilterByMonth>0 || ps->geoFilterByYear>0)
	{
		std::vector<std::string>* timetaken = Metadata::getTimeTakens(&docidVec);

		std::vector<int> tmp;
		for(int i=0; i<docidVec.size();i++)
		{
			std::string stime = timetaken->at(i);
			int month = Metadata::getMonth(stime);
			int year = Metadata::getYear(stime);

			if(ps->geoFilterByMonth>0 && ps->geoFilterByMonth!=month)
				continue;
			if(ps->geoFilterByYear>0 && ps->geoFilterByYear!=year)
				continue;

			tmp.push_back(docidVec[i]);
		}

		docidVec.clear();
		for(int i=0; i<tmp.size(); i++)
			docidVec.push_back(tmp[i]);
	}

	std::cerr<<"Number of training documents found in TermDistributionFilter::isTermGeographic() => "<<docidVec.size()<<std::endl;

	std::vector<std::string>* svecLat = Metadata::getLatitudes(&docidVec);
	std::vector<std::string>* svecLng = Metadata::getLongitudes(&docidVec);

	for(int i=0; i<svecLat->size(); i++)
	{
		double lat = atof(svecLat->at(i).c_str()) + 90.0;
		double lng = atof(svecLng->at(i).c_str()) + 180.0;

		int iLat = (int)floor(lat);
		int iLng = (int)floor(lng);
		grid[iLat][iLng]++;
	}

	int clusterID=1;
	int maxEntry=0;
	int clusterGrid[180][360];
	int cellsFilled = 0;
	int cellsContent = 0;
	for(int i=0; i<180; i++)
	{
		for(int j=0; j<360; j++)
		{
			clusterGrid[i][j]=clusterID++;
			if(grid[i][j]>maxEntry)
			{
				maxEntry = grid[i][j];
			}

			if(grid[i][j]>0)
				cellsFilled++;

			cellsContent += grid[i][j];
		}
	}

	bool changing = true;
	while(changing==true)
	{
		changing = false;

		for(int i=1; i<179; i++)
		{
			for(int j=1; j<359; j++)
			{
				if(grid[i][j]==0)
					continue;

				//left
				if(grid[i-1][j]>0 && clusterGrid[i][j]!=clusterGrid[i-1][j])
				{
					changing = true;
					if(clusterGrid[i][j]<clusterGrid[i-1][j])
						clusterGrid[i-1][j]=clusterGrid[i][j];
					else
						clusterGrid[i][j]=clusterGrid[i-1][j];
				}
				//right
				if(grid[i+1][j]>0 && clusterGrid[i][j]!=clusterGrid[i+1][j])
				{
					changing = true;
					if(clusterGrid[i][j]<clusterGrid[i+1][j])
						clusterGrid[i+1][j]=clusterGrid[i][j];
					else
						clusterGrid[i][j]=clusterGrid[i+1][j];
				}
				//up
				if(grid[i][j+1]>0 && clusterGrid[i][j]!=clusterGrid[i][j+1])
				{
					changing = true;
					if(clusterGrid[i][j]<clusterGrid[i][j+1])
						clusterGrid[i][j+1]=clusterGrid[i][j];
					else
						clusterGrid[i][j]=clusterGrid[i][j+1];
				}
				//down
				if(grid[i][j-1]>0 && clusterGrid[i][j]!=clusterGrid[i][j-1])
				{
					changing = true;
					if(clusterGrid[i][j]<clusterGrid[i][j-1])
						clusterGrid[i][j-1]=clusterGrid[i][j];
					else
						clusterGrid[i][j]=clusterGrid[i][j-1];

				}
			}
		}
	}

	//how many unique clusterIDs do we have that are not zero?
	std::set<int> clusterIDSet;
	for(int i=0; i<180; i++)
	{
		for(int j=0; j<360; j++)
		{
			if(grid[i][j]==0)
				continue;
			clusterIDSet.insert(clusterGrid[i][j]);
		}
	}

	//normalize by max. count
	double res = 10000.0;
	if(maxEntry>0)
		res = ((double)clusterIDSet.size())/((double)maxEntry);

	evaluatedGeoTerms->insert(std::make_pair(termid, res));
	std::cerr<<"\tgeo-score("<<ps->ind->term(termid)<<")="<<res<<std::endl;
	return res;
}

