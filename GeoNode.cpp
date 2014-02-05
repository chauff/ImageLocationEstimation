#include "GeoNode.hpp"


/**
 * a GeoNode is defined by its latitude/longitude center and the extension of the rectangle in the horizontal/vertical direction;
 * a node with center (0,0) and latDegreesFromCenter=90 and lngDegreesFromCenter=180 spans the entire earth
 */
lemur::extra::GeoNode::GeoNode(double lat, double lng, double latFromCenter,
		double lngFromCenter)
{
	ps = lemur::extra::ParameterSingleton::getInstance();

	latitudeCenter = lat;
	longitudeCenter = lng;
	latDegreesFromCenter = latFromCenter;
	lngDegreesFromCenter = lngFromCenter;

	childrenDocids = new std::vector<GeoDoc*>();//keep track of the docids that are stored in the children
	children = new std::vector<GeoNode*>();//keep track of the children
	parent = NULL;
	geoDUC = NULL;
	geoSmoothedLM = NULL;
}

/**
 * the root node ... spans the entire earth
 */
lemur::extra::GeoNode::GeoNode()
{
	ps = lemur::extra::ParameterSingleton::getInstance();

	latitudeCenter = 0.0;
	longitudeCenter = 0.0;
	latDegreesFromCenter = 90.0;
	lngDegreesFromCenter = 180.0;

	childrenDocids = new std::vector<GeoDoc*>();//keep track of the docids that are stored in the children
	children = new std::vector<GeoNode*>();//keep track of the children
	parent = NULL;
	geoDUC = NULL;
}


void lemur::extra::GeoNode::addDocument(int docid)
{
	//create the object and push it through the tree
	//by definition we have a training item here, so add noise if set in the parameter settings
	GeoDoc *gd = new GeoDoc(docid,true);
	addDocument(gd);
}

/**
 * a document is added to the node;
 * the latitude/longitude of the document is determined and if the node is not a leaf node, the document
 * is automatically propagated to the correct node (this is usually called from the root node, i.e. the document traverses the entire tree)
 */
void lemur::extra::GeoNode::addDocument(GeoDoc *gd)
{
	childrenDocids->push_back(gd);

	//if the current node has children, this document needs to be pushed down the tree
	if (children->size() > 0)
	{
		//push it to the correct child ....
		for (int i = 0; i < children->size(); i++)
		{
			GeoNode *c = children->at(i);
			if (c->isGeoDocFitting(gd) == true)
			{
				c->addDocument(gd);
				return;
			}
		}
		std::cerr << "Could not find correct child for document with lat="<< gd->latitude << ", lng=" << gd->longitude << std::endl;
		return;
	}
	//no children, must mean that the document should be added here ....
	else
	{
		splitNode();//check if a split is necessary
		return;
	}
}


/**
 * method checks if the GeoDoc object fits geographically to the node (i.e. is it within the correct latitude/longitude boundaries)
 */
bool lemur::extra::GeoNode::isGeoDocFitting(GeoDoc *gd)
{
	return isGeoDocFitting(gd->latitude, gd->longitude);
}

/**
 * method checks if the given lat/lng fits geographically to the node (i.e. is it within the correct latitude/longitude boundaries)
 */
bool lemur::extra::GeoNode::isGeoDocFitting(double lat, double lng)
{
	if (lat <= (latitudeCenter - latDegreesFromCenter))
		return false;
	if (lat > (latitudeCenter + latDegreesFromCenter))
		return false;

	if (lng <= (longitudeCenter - lngDegreesFromCenter))
		return false;
	if (lng > (longitudeCenter + lngDegreesFromCenter))
		return false;

	return true;
}

/**
 * in a tree, each node has a parent (apart from the root)
 */
void lemur::extra::GeoNode::setParent(GeoNode *p)
{
	parent = p;
}

lemur::extra::GeoNode* lemur::extra::GeoNode::getParent()
{
	return parent;
}

/**
 * returns the total number of leaf nodes of the current node;
 * total here means that if the node is not a leaf (in which case the count is 1),
 * the leaves of all direct and indirect children are accumulated
 */
int lemur::extra::GeoNode::getTotalNumLeaves()
{
	int sum = 0;
	_getTotalNumLeaves(sum);
	return sum;
}

void lemur::extra::GeoNode::_getTotalNumLeaves(int& sum)
{
	if (children->size() == 0)//leaf node
		sum +=1;
	else
	{
		for (int i = 0; i < children->size(); i++)
			children->at(i)->_getTotalNumLeaves(sum);
	}
}

/**
 * in order to not traverse the tree each time, we check the leaves, this method
 * fills the set given as input with all the leaves that are under the current node
 */
void lemur::extra::GeoNode::fillSetWithLeaves(std::set<GeoNode*> *nodeSet)
{
	if (children->size() == 0)
	{
		if(childrenDocids->size()>0)
			nodeSet->insert(this);
		return;
	}
	else
	{
		//iterate over all children
		for (int i = 0; i < children->size(); i++)
			children->at(i)->fillSetWithLeaves(nodeSet);
	}
}


/**
 * given a level and a set of nodes, this method fills the node set with all nodes
 * at the correct level of the tree; root is level 0, first split is level 1 and so on
 */
void lemur::extra::GeoNode::fillSetWithNodesAtLevel(std::set<GeoNode*> *nodeSet, int level)
{
	//either achieved the wanted level or this is a leave or it has less than X documents
	if( level == 0 || children->size()==0 || childrenDocids->size()<ps->GN_SPLIT_NUM)
	{
		if(childrenDocids->size()>0)
			nodeSet->insert(this);
		return;
	}
	else
	{
		//iterate over all children
		for (int i = 0; i < children->size(); i++)
			children->at(i)->fillSetWithNodesAtLevel(nodeSet, level-1);
	}
}


/**
 * if a document is added to a leaf node, a split of the leaf node (leaf node becomes internal
 * node with 4 leaf children) might be necessary;
 * a split happens if too many documents accumulate at the node and the minimum step size for the
 * grid is not reached yet;
 * in a split 4 nodes are created and the documents are distributed among them, depending on their
 * location
 */
void lemur::extra::GeoNode::splitNode()//split the node if necessary
{
	//only split if the number #docid is reached and the degrees to split are more than the minimum
	if ( childrenDocids->size() <= ps->GN_SPLIT_NUM || latDegreesFromCenter
			<= ps->GN_MIN_LATITUDE_STEP || lngDegreesFromCenter <= ps->GN_MIN_LONGITUDE_STEP)
	{
		return;
	}

	//create 4 children
	GeoNode *northWest = new GeoNode(
			latitudeCenter + 0.5 * latDegreesFromCenter,
			longitudeCenter - 0.5 * lngDegreesFromCenter,
			0.5 * latDegreesFromCenter, 0.5 * lngDegreesFromCenter);
	GeoNode *southWest = new GeoNode(
			latitudeCenter - 0.5 * latDegreesFromCenter,
			longitudeCenter - 0.5 * lngDegreesFromCenter,
			0.5 * latDegreesFromCenter, 0.5 * lngDegreesFromCenter);
	GeoNode *northEast = new GeoNode(
			latitudeCenter + 0.5 * latDegreesFromCenter,
			longitudeCenter + 0.5 * lngDegreesFromCenter,
			0.5 * latDegreesFromCenter, 0.5 * lngDegreesFromCenter);
	GeoNode *southEast = new GeoNode(
			latitudeCenter - 0.5 * latDegreesFromCenter,
			longitudeCenter + 0.5 * lngDegreesFromCenter,
			0.5 * latDegreesFromCenter, 0.5 * lngDegreesFromCenter);

	//couple parent and children
	northWest->setParent(this);
	northEast->setParent(this);
	southWest->setParent(this);
	southEast->setParent(this);

	children->push_back(northWest);
	children->push_back(northEast);
	children->push_back(southEast);
	children->push_back(southWest);

	//distribute the docids to the children
	for (int i = 0; i < childrenDocids->size(); i++)
	{
		GeoDoc *gd = childrenDocids->at(i);

		if (northWest->isGeoDocFitting(gd) == true)
			northWest->addDocument(gd);
		else if (northEast->isGeoDocFitting(gd) == true)
			northEast->addDocument(gd);
		else if (southEast->isGeoDocFitting(gd) == true)
			southEast->addDocument(gd);
		else if (southWest->isGeoDocFitting(gd) == true)
			southWest->addDocument(gd);
		else
		{
			std::cerr << "Cannot assign GeoDoc to any child!";
			return;
		}
	}

	//do splitting until no child needs further splitting ....
	northWest->splitNode();
	northEast->splitNode();
	southEast->splitNode();
	southWest->splitNode();
}


/**
 * the GeoDoc given as input is evaluated; returned is the probability with
 * which the language model created the GeoDoc
 *
 * if the node is an internal node, the language model is created from all its children;
 * if it is a leaf node, the language model is created from all the leaf's documents
 *
 * the smoothing parameter determines which smoothing method is chosen: JM if smoothingParam<1, Dirichlet smoothing otherwise
 */
double lemur::extra::GeoNode::getGenerationLikelihood(lemur::extra::GeoDoc *gd, bool storeInClassVar)
{
	lemur::langmod::DocUnigramCounter *counter = NULL;
	lemur::langmod::UnigramLM *lm = NULL;

	if(storeInClassVar==true && geoDUC)
	{
		counter = geoDUC;
		lm = geoSmoothedLM;
	}
	else
	{
		std::vector<int> docidVec;

		for(int i=0; i<childrenDocids->size(); i++)
			docidVec.push_back(childrenDocids->at(i)->docid);

		counter = new lemur::langmod::DocUnigramCounter(docidVec,*(ps->ind));
		lm = new lemur::langmod::DirichletUnigramLM(*counter, ps->ind->termLexiconID(),*(ps->colLM), ps->smoothingParam);

		if(storeInClassVar==true)
		{
			geoDUC = counter;
			geoSmoothedLM = lm;
		}
	}


	if(!counter || !lm)
	{
		std::cerr<<"Invalid pointer for either counter or lm: "<<counter<<" "<<lm<<std::endl;
		return -1 * DBL_MAX;
	}

	std::vector<double> probSumVec;

	for(int i=0; i<ps->GN_FIELDS.size(); i++)
	{
		std::map<int,int> *termMap = gd->fieldTerms->find(ps->GN_FIELDS[i])->second;

		double probSum = 0.0;
		for (std::map<int, int>::iterator termIt = termMap->begin(); termIt
				!= termMap->end(); termIt++)
		{
			int termid = (*termIt).first;
			int count = (*termIt).second;

			//this is a necessary check; e.g. if a test document contains a unique term, it will not appear in the collection LM
			if(ps->colLM->prob(termid)<=0)
				continue;

			double pLM  = pow(lm->prob(termid) , (double)count);

			probSum += log(pLM);
		}
		probSumVec.push_back(probSum);
	}
	double interpResult = 0.0;

	for(int i=0; i<ps->GN_ALPHAS.size(); i++)
	{
		double alpha = ps->GN_ALPHAS[i];
		interpResult += alpha * probSumVec[i];
	}

	if(storeInClassVar==false)
	{
		delete counter;
		delete lm;
	}
	return interpResult;
}


//lets assume we have found the right GeoNode ...
//return the location of the image that is closest to it
lemur::extra::GeoDoc* lemur::extra::GeoNode::getNearestNeighbour(GeoDoc *gd)
{
	std::map<int,double> acc;

	double maxScore=-1 * DBL_MAX;
	int maxDocid = -1;

	std::map<int,GeoDoc*> dSet;

	for (int i = 0; i < childrenDocids->size(); i++)
		dSet.insert(std::make_pair(childrenDocids->at(i)->docid, childrenDocids->at(i)));

	lemur::api::TermInfoList *tlist = ps->ind->termInfoList(gd->docid);
	tlist->startIteration();

	while(tlist->hasMore())
	{
		lemur::api::TermInfo *info = tlist->nextEntry();

		int termid = info->termID();
		int count = info->count();
		if(ps->colLM->prob(termid)<=0.0)
			continue;

		std::set<int> foundDocids;

		lemur::api::DocInfoList *dlist = ps->ind->docInfoList(termid);
		dlist->startIteration();
		while(dlist->hasMore())
		{
			lemur::api::DocInfo *dinfo = dlist->nextEntry();
			if(dSet.find(dinfo->docID())!=dSet.end())
			{
			    lemur::api::DOCID_T docID = dinfo->docID();
			    double prob = ((count + ps->smoothingParam * ps->colLM->prob(termid)) / (ps->ind->docLength(docID)+ps->smoothingParam));

			    std::map<int,double>::iterator accIt = acc.find(docID);
			    if(accIt==acc.end())
			    	acc.insert(std::make_pair(docID, log(prob)));
			    else
			    	accIt->second+=log(prob);
				foundDocids.insert(docID);
			}
		}

		for(std::map<int,GeoDoc*>::iterator it=dSet.begin(); it!=dSet.end(); it++)
		{
			if( foundDocids.find(it->first)!=foundDocids.end() )
				continue;

			double prob = ((0.0+ps->smoothingParam*ps->colLM->prob(termid))/((double)ps->ind->docLength(it->first)+ps->smoothingParam));

		    std::map<int,double>::iterator accIt = acc.find(it->first);
		    if(accIt==acc.end())
		    	acc.insert(std::make_pair(it->first, log(prob)));
		    else
		    	accIt->second+=log(prob);
		}
		delete dlist;
	}
	delete tlist;


	for(std::map<int,double>::iterator accIt=acc.begin(); accIt!=acc.end(); accIt++)
	{
		if(accIt->second > maxScore && accIt->second!=0.0)
		{
			maxScore=accIt->second;
			maxDocid = accIt->first;
		}
	}

	if(maxDocid<0 || dSet.find(maxDocid)==dSet.end())
	{
		std::cerr<<"Error: maxIndex="<<maxDocid<<std::endl;
		std::cerr<<"returning first document ...."<<std::endl;
		return childrenDocids->at(0);
	}
	return dSet.find(maxDocid)->second;
}
