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

	docids = new std::vector<GeoDoc*>();//keep track of the docids stored in the current node
	childrenDocids = new std::vector<GeoDoc*>();//keep track of the docids that are stored in the children
	children = new std::vector<GeoNode*>();//keep track of the children
	neighbours = new std::set<GeoNode*>;
	parent = NULL;
	geoDUC = NULL;
	geoMLLM = NULL;
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

	docids = new std::vector<GeoDoc*>();//keep track of the docids stored in the current node
	childrenDocids = new std::vector<GeoDoc*>();//keep track of the docids that are stored in the children
	children = new std::vector<GeoNode*>();//keep track of the children
	neighbours = new std::set<GeoNode*>;
	parent = NULL;
	geoDUC = NULL;
	geoMLLM = NULL;
}


void lemur::extra::GeoNode::addDocument(int docid)
{
	//create the object and push it through the tree
	GeoDoc *gd = new GeoDoc(docid);
	addDocument(gd);
}

/**
 * a document is added to the node;
 * the latitude/longitude of the document is determined and if the node is not a leaf node, the document
 * is automatically propagated to the correct node (this is usually called from the root node, i.e. the document traverses the entire tree)
 */
void lemur::extra::GeoNode::addDocument(GeoDoc *gd)
{
	//if the current node has children, this document needs to be pushed down the tree
	if (children->size() > 0)
	{
		//store it in the special children aggregate vector
		childrenDocids->push_back(gd);

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
		//error checking
		std::cerr << "Could not find correct child for document with lat="<< gd->latitude << ", lng=" << gd->longitude << std::endl;
		return;
	}
	//no children, must mean that the document should be added here ....
	else
	{
		docids->push_back(gd);
		splitNode();//check if a split is necessary
		return;
	}
}


/**
 * method checks if the GeoDoc object fits geographically to the node (i.e. is it within the correct latitude/longitude boundaries)
 */
bool lemur::extra::GeoNode::isGeoDocFitting(GeoDoc *gd)
{
	if (gd->latitude <= (latitudeCenter - latDegreesFromCenter))
		return false;
	if (gd->latitude >(latitudeCenter + latDegreesFromCenter))
		return false;

	if (gd->longitude <= (longitudeCenter - lngDegreesFromCenter))
		return false;
	if (gd->longitude > (longitudeCenter + lngDegreesFromCenter))
		return false;

	return true;
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
 * returns the number of DIRECT children of a GeoNode;
 * should be either 0 or 4 (as the split is always into 4 sub-rectangles)
 */
int lemur::extra::GeoNode::getNumChildren()
{
	return children->size();
}

/**
 * returns the total number of GeoDocs attached to the current node;
 * total here means that if the node is not a leaf, the leaf counts of all the nodes
 * direct and indirect children are accumulated;
 * note: this number should be the same as get docidsChildren->size()
 * (this method is kept as a control ...)
 */
int lemur::extra::GeoNode::getTotalNumGeoDocs()
{
	if (docids->size() > 0)//leaf node
		return docids->size();
	else if (children->size() > 0)
	{
		//iterate over all children
		int sum = 0;
		for (int i = 0; i < children->size(); i++)
			sum += children->at(i)->getTotalNumGeoDocs();
		return sum;
	}
	else
		return 0;
}

/**
 * returns the total number of leaf nodes of the current node;
 * total here means that if the node is not a leaf (in which case the count is 1),
 * the leaves of all direct and indirect children are accumulated
 */
int lemur::extra::GeoNode::getTotalNumLeaves()
{
	if (docids->size() > 0)//leaf node
		return 1;
	else if (children->size() > 0)
	{
		//iterate over all children
		int sum = 0;
		for (int i = 0; i < children->size(); i++)
			sum += children->at(i)->getTotalNumLeaves();
		return sum;
	}
	else
		return 0;
}

/**
 * in order to not traverse the tree each time, we check the leaves, this method
 * fills the set given as input with all the leaves that are under the current node
 * (usually called from the root node)
 */
void lemur::extra::GeoNode::fillSetWithLeaves(std::set<GeoNode*> *nodeSet)
{
	if (docids->size() > 0)//non-empty leaf node
	{
		nodeSet->insert(this);
		return;
	}
	else if (children->size() > 0)
	{
		//iterate over all children
		for (int i = 0; i < children->size(); i++)
			children->at(i)->fillSetWithLeaves(nodeSet);
	}
	else
		return;
}


/**
 * given a level and a set of nodes, this method fills the node set with all nodes
 * at the correct level of the tree; root is level 0, first split is level 1 and so on
 */
void lemur::extra::GeoNode::fillSetWithNodesAtLevel(std::set<GeoNode*> *nodeSet, int level)
{
	//either achieved the wanted level or this is the final node (leave)
	if( level == 0 || docids->size()>0 )
	{
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
 * when a document passes "through" a node to its children, it is still kept as
 * a record at the node; thus, this method returns all documents in the form of
 * GeoDoc pointers that have passed through the current node
 */
std::vector<lemur::extra::GeoDoc*>* lemur::extra::GeoNode::getDocidsInChildren()
{
	return childrenDocids;
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
	if (docids->size() <= ps->GN_SPLIT_NUM || latDegreesFromCenter
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
	for (int i = 0; i < docids->size(); i++)
	{
		GeoDoc *gd = docids->at(i);

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

		//the docids are now in the childrenDocids vector of the new internal node
		childrenDocids->push_back(gd);
	}

	//do splitting until no child needs further splitting ....
	northWest->splitNode();
	northEast->splitNode();
	southEast->splitNode();
	southWest->splitNode();

	//don't forget to clear the docids from the now parent node
	docids->clear();
}

/**
 * takes a termid as input and returns the maximum likelihood probability
 * of the term given the LM made from the current node;
 * if the node is a leaf node, all its documents are used; if it is an internal
 * node, all documents of its children are used;
 *
 * it can also happen, that none of the documents in the current GeoNode have any tags
 * associated with them; in this case, the returned probability is MINUS ONE!!
 */
double lemur::extra::GeoNode::getMLTermProb(int termid)
{
	if(termid<=0)
		return -1;

	if(geoDUC==0)
	{
		std::vector<int> docidVec;
		std::vector<GeoDoc*> *pointer;
		if(docids->size()>0)
			pointer = docids;
		else
			pointer = childrenDocids;

		for(int i=0; i<pointer->size(); i++)
			docidVec.push_back(pointer->at(i)->docid);

		geoDUC = new lemur::langmod::DocUnigramCounter(docidVec,*(ps->ind));
	}
	if(geoMLLM==0)
	{
		geoMLLM = new lemur::langmod::MLUnigramLM(*geoDUC,ps->ind->termLexiconID());
	}

	if( geoDUC->sum()<=0.0 )
		return -1.0;

	double p = geoMLLM->prob(termid);
	if(isnan(p))
	{
		std::cerr<<"Error: p=NaN within getMLTermProb"<<std::endl;
		exit(1);
	}
	return p;
}


/**
 * takes a termid as input and returns the termcount in the ML model made from all documents
 * below this node
 */
int lemur::extra::GeoNode::getMLTermCount(int termid)
{
	if(termid<=0)
		return 0;

	if(geoDUC==0)
	{
		std::vector<int> docidVec;
		std::vector<GeoDoc*> *pointer;
		if(docids->size()>0)
			pointer = docids;
		else
			pointer = childrenDocids;

		for(int i=0; i<pointer->size(); i++)
			docidVec.push_back(pointer->at(i)->docid);

		geoDUC = new lemur::langmod::DocUnigramCounter(docidVec,*(ps->ind));
	}
	return geoDUC->count(termid);
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
double lemur::extra::GeoNode::getGenerationLikelihood(lemur::extra::GeoDoc *gd,bool storeInClassVar)
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
		std::vector<GeoDoc*> *pointer = (docids->size()>0) ? docids : childrenDocids;
		for(int i=0; i<pointer->size(); i++)
			docidVec.push_back(pointer->at(i)->docid);

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

	for (int i = 0; i < docids->size(); i++)
		dSet.insert(std::make_pair(docids->at(i)->docid, docids->at(i)));

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
		return docids->at(0);
	}
	return dSet.find(maxDocid)->second;
}
