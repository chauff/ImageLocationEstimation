#ifndef GEONODE_HPP_
#define GEONODE_HPP_

#include "common_headers.hpp"
#include "indri/Repository.hpp"
#include "IndexManager.hpp"
#include "UnigramLM.hpp"
#include "DocUnigramCounter.hpp"
#include "GeoDoc.hpp"
#include "TermDistributionFilter.hpp"
#include "Distance.hpp"
#include <math.h>
#include <set>
#include <vector>
#include "ParameterSingleton.hpp"


namespace lemur{
	namespace extra{

class GeoDoc;

class GeoNode
{

	private:
		void addDocument(GeoDoc *gd);

		std::vector<GeoNode*> *children;//store in here the children nodes
		GeoNode *parent;//store hte parent node

		//DocUnigramCounter of all children nodes or if leaf, all docids of the leaf
		lemur::langmod::DocUnigramCounter *geoDUC;
		//maximum likelihood language model based on geoDUC
		lemur::langmod::MLUnigramLM *geoMLLM;
		lemur::langmod::UnigramLM *geoSmoothedLM;
		lemur::extra::ParameterSingleton *ps;

	public:
		double latitudeCenter;//store the center of the rectangle on the earth grid
		double longitudeCenter;
		double latDegreesFromCenter;
		double lngDegreesFromCenter;

		std::vector<GeoDoc*> *docids;//store in here the documents (if leaf)
		std::vector<GeoDoc*> *childrenDocids;//aggregate here the documents of the children

		GeoNode(double lat, double lng, double latFromCenter, double lngFromCenter);
		GeoNode();

		void addDocument(int docid);
		bool isGeoDocFitting(GeoDoc *gd);
		bool isGeoDocFitting(double lat, double lng);
		void setParent(GeoNode *p);
		GeoNode* getParent();
		std::vector<GeoDoc*>* getDocidsInChildren();
		int getNumChildren();
		int getTotalNumGeoDocs();
		int getTotalNumLeaves();
		void fillSetWithLeaves(std::set<GeoNode*> *nodeSet);
		void fillSetWithNodesAtLevel(std::set<GeoNode*> *nodeSet, int level);
		void splitNode();
		double getSizeOfArea();
		double getGenerationLikelihood(lemur::extra::GeoDoc *gd, bool higherLevelType);
		GeoDoc* getNearestNeighbour(GeoDoc *gd);

		double getMLTermProb(int termid);
		int getMLTermCount(int termid);

		std::set<GeoNode*> *neighbours;
};

	}}

#endif
