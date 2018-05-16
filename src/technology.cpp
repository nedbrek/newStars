//
// C++ Implementation: technology
//
// Copyright: See COPYING file that comes with this distribution
//
#include "classDef.h"
#include "utils.h"
#include <iostream>

bool StarsTechnology::parseXML(mxml_node_t *node, string nodeName)
{
    mxml_node_t *child = NULL;
    child = mxmlFindElement(node, node, nodeName.c_str(), NULL,
                            NULL, MXML_DESCEND);
    if( !child )
    {
        cerr << "Error parsing input file while parsing for " << nodeName
        << ".  Node not found.\n";
        return false;
    }
    child = child->child;

    unsigned valArray[6];
    int valCounter = assembleNodeArray(child, valArray, 6);

    if( valCounter != 6 )
    {
        fprintf(stderr, "Error, expected 6 %s data points , found %i\n",
                nodeName.c_str(), valCounter);
        return false;
    }

    for(int i = 0; i < numberOf_TECH; i++)
        techLevels[i] = valArray[i];

    return true;
}

// returns true if the object meets or exceeds whatlevel
bool StarsTechnology::meetsTechLevel(const StarsTechnology &whatLevel) const
{
	for(int i = 0; i < numberOf_TECH; i++)
		if( techLevels[i] < whatLevel.techLevels[i] )
			return false;

	return true;
}

void StarsTechnology::toXMLString(string &theString, const string &nodeName) const
{
    char temp[1024];
    sprintf(temp, "<%s>%i %i %i %i %i %i</%s>\n", nodeName.c_str(),
            techLevels[energy_TECH],techLevels[weapons_TECH],techLevels[propulsion_TECH],
            techLevels[construction_TECH],techLevels[electronics_TECH],techLevels[biotech_TECH],nodeName.c_str());
    theString = temp;
}

void StarsTechnology::toCPPString(string &theString)
{
    char temp[1024];
    sprintf(temp, "EN %i WE %i PR %i CO %i EL %i BI %i\n",
            techLevels[energy_TECH],techLevels[weapons_TECH],techLevels[propulsion_TECH],
            techLevels[construction_TECH],techLevels[electronics_TECH],techLevels[biotech_TECH]);
    theString = temp;
}

bool Technology::meetsTechLevel(Technology whatLevel)
{
    return false;
}

void Technology::toCPPString(string &theString)
{
    theString = "Base technology string.\n";
}
void Technology::toXMLString(string &theString)
{
    theString = "Error, Technology toXML() should never be called\n";
}
bool Technology::parseXML(mxml_node_t *tree,string nodeName)
{
    cerr<< "Error, Technology parseXML() should never be called\n";
    return false;
}
