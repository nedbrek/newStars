//
// C++ Implementation: placeableObject
//
// Copyright: See COPYING file that comes with this distribution
//
#include "newStars.h"
#include "utils.h"
#include "classDef.h"
#include <iostream>
#include <sstream>

static const char *const CargoDescriptions[_numCargoTypes+1] =
{
   "Colonists",
   "Ironium",
   "Germanium",
   "Boranium",
   "Fuel",
   "ERROR"
};

// action handler for a placeable object to handle unload actions
bool PlaceableObject::unloadAction(GameData &gd, bool guiBased)
{
    // if no orders, just sit here
    if( !currentOrders.size() )
        return true;

    // if no unload order, just sit
    Order *firstOrderInQueue = currentOrders[0];
    if( !guiBased && firstOrderInQueue->myOrderID != unload_cargo_order )
        return true;
    if(guiBased && firstOrderInQueue->myOrderID != gui_unload_order )
        return true;

    // transfer to ourselves... what?
    if( firstOrderInQueue->targetID == firstOrderInQueue->objectId )
        return true;

    PlaceableObject *targetObject = gd.getTargetObject(firstOrderInQueue->targetID);

    if( !(coordinates == targetObject->coordinates) )
    {
        // attempted to transfer cargo to an onject not at our location
        cerr << "Error, attempting to unload cargo to a non-local object. Object #"
        << objectId << ", " << name << endl;
        return false;
    }

    if( firstOrderInQueue->cargoTransferDetails.anyGreaterThan(cargoList) )
    {
        // attempting to unload more than we have...
        cerr << "Error, attempting to unload more cargo than available from object #"
        << objectId << ", " << name << endl;
        return false;
    }

    cargoList = cargoList - firstOrderInQueue->cargoTransferDetails;
    targetObject->cargoList = targetObject->cargoList + firstOrderInQueue->cargoTransferDetails;

    currentOrders.erase(currentOrders.begin()); // done, delete the order
    return true;
}

// action handler for a placeable object to handle load actions
bool PlaceableObject::loadAction(GameData &gd, bool guiBased)
{
    // no orders, just sit here
    if( !currentOrders.size() )
        return true;

    // if no load order, just sit
    Order *firstOrderInQueue = currentOrders[0];
    if( !guiBased && firstOrderInQueue->myOrderID != load_cargo_order )
        return true;
    if( guiBased && firstOrderInQueue->myOrderID != gui_load_order )
        return true;

    // transfer to ourselves... what?
    if( firstOrderInQueue->targetID == firstOrderInQueue->objectId )
        return true;

    PlaceableObject *targetObject = gd.getTargetObject(firstOrderInQueue->targetID);

    if( !(coordinates == targetObject->coordinates) )
    {
        // attempt to transfer cargo to an onject not at our location
        cerr << "Error, attempting to load cargo to a non-local object.  Object #"
        << objectId << ", " << name << endl;
        cerr << "Transfer attempt from " << targetObject->name << endl;
        cerr << name << " location is " << coordinates.getX() << ","
        << coordinates.getY()   << endl;
        cerr << targetObject->name << " location is "
        << targetObject->coordinates.getX() << ","
        << targetObject->coordinates.getY() << endl;
        return false;
    }

    if( firstOrderInQueue->cargoTransferDetails < cargoList )
    {
        // attempt to unload more than we have...
        cerr << "Error, attempting to load more cargo than available from source.  Object #"
        << objectId << ", " << name << endl;
        return false;
    }

    cargoList = cargoList + firstOrderInQueue->cargoTransferDetails;
    targetObject->cargoList = targetObject->cargoList - firstOrderInQueue->cargoTransferDetails;

    currentOrders.erase(currentOrders.begin()); // done, delete the order
    return true;
}

bool GameObject::parseXML(mxml_node_t* tree)
{
    mxml_node_t *child = mxmlFindElement(tree, tree, "NAME", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr,"Error, could not locate NAME keyword\n");
        return false;
    }
    child = child->child;

    assembleNodeArray(child,name);

    child = mxmlFindElement(tree, tree, "OBJECTID", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        cerr << "Error, could not parse OBJECTID for:" << name << "\n";
        return false;
    }
    child = child->child;
    objectId = atoll(child->value.text.string);

    if (objectId >= nextObjectId)
        nextObjectId = objectId +1;

    child = mxmlFindElement(tree, tree, "OWNERID", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        cerr << "Error, could not parse OWNERID for:" << name << "\n";
        return false;
    }
    child = child->child;
    ownerID = atoi(child->value.text.string);

    unsigned temp;
    if (parseXML_noFail(tree,"GAMEOBJECTTYPE",temp))
        myType = (gameObjectType)temp;
    return true;
}

void GameObject::toXMLStringGuts(string &theString) const
{
	std::ostringstream os;
	os << "<NAME>"     << name     << "</NAME>"     << std::endl;
	os << "<OBJECTID>" << objectId << "</OBJECTID>" << std::endl;
	os << "<OWNERID>"  << ownerID  << "</OWNERID>"  << std::endl;
	os << "<GAMEOBJECTTYPE>" << myType << "</GAMEOBJECTTYPE>" << std::endl;

	theString = os.str();
}

void PlaceableObject::toXMLStringGuts(string &theString) const
{
    theString = "";
    GameObject::toXMLStringGuts(theString);

    std::string workString = "";
    char tempString[2048];

    sprintf(tempString, "<X_COORD>%i</X_COORD>\n<Y_COORD>%i</Y_COORD>\n",
            coordinates.getX(), coordinates.getY());
    theString += tempString;

	sprintf(tempString, "<PENSCANRADIUS>%i</PENSCANRADIUS>\n<NONPENSCANRADIUS>%i</NONPENSCANRADIUS>\n",
		    penScanRadius,nonPenScanRadius);

    cargoList.toXMLString(workString, "CARGOMANIFEST", "SCALE=\"KiloTons\"");
    theString += workString;

    theString += "<ORDERLIST>\n";
    string oString;
    for(unsigned i = 0; i < currentOrders.size(); i++)
    {
        Order *tempOrder = currentOrders[i];
        tempOrder->toXMLString(oString);
        theString += oString;
    }
    theString += "</ORDERLIST>\n";
}

bool PlaceableObject::parseXML(mxml_node_t *tree)
{
    if( !GameObject::parseXML(tree) )
        return false;

    /*   cerr<<"\n\n***\nTree for object: "<<name<<endl;
       char cTemp[4096];
       mxmlSaveString(tree,cTemp,4096,NULL);
       cerr<<cTemp;*/

	penScanRadius = 0;
	nonPenScanRadius = 0;
	parseXML_noFail(tree,"PENSCANRADIUS",penScanRadius);
	parseXML_noFail(tree,"NONPENSCANRADIUS",nonPenScanRadius);


    mxml_node_t *child = mxmlFindElement(tree, tree, "X_COORD", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr, "Error, could not locate X_COORD keyword for %s\n",name.c_str());
        return false;
    }
    child = child->child; // descend one level

    unsigned xLoc = atoi(child->value.text.string);

    child = mxmlFindElement(tree, tree, "Y_COORD", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr, "Error, could not locate Y_COORD keyword\n");
        return false;
    }
    child = child->child; // descend one level

    coordinates.set(xLoc, atoi(child->value.text.string));


    if( !cargoList.parseXML(tree, "CARGOMANIFEST") )
    {
        cerr << "Error parsing CARGOMANIFEST for object named: "
        << name.c_str() << ".\n";
        return false;
    }

    // orders are optional, so don't fail if not present...

    child = mxmlFindElement(tree, tree, "ORDERLIST", NULL, NULL, MXML_DESCEND);

    child = mxmlFindElement(child, child, "ORDER", NULL, NULL, MXML_DESCEND);
    while( child )
    {
        Order *tempOrder = new Order();
        tempOrder->parseXML(child);

        currentOrders.push_back(tempOrder);
        child = mxmlFindElement(child, child, "ORDER", NULL, NULL, MXML_NO_DESCEND);
    }

    return true;
}

bool CargoManifest::parseXML(mxml_node_t* tree, const string &nodeName)
{
    mxml_node_t *child = mxmlFindElement(tree, tree, nodeName.c_str(),
                                         NULL, NULL, MXML_DESCEND);

    if( !child )
    {
        cerr << "Error parsing input file while parsing for " << nodeName
        << ".  Node not found." << endl;
        return false;
    }
    child = child->child;

    unsigned valArray[5];
    int valCounter = assembleNodeArray(child, valArray, 5);

    if( valCounter != 5 )
    {
        fprintf(stderr, "Error, expected 5 %s data points , found %i\n",
                nodeName.c_str(), valCounter);
        return false;
    }

    cargoDetail[_ironium  ].init(valArray[0]);
    cargoDetail[_boranium ].init(valArray[1]);
    cargoDetail[_germanium].init(valArray[2]);
    cargoDetail[_fuel     ].init(valArray[3]);
    cargoDetail[_colonists].init(valArray[4]);

    return true;
}

void CargoManifest::toCPPString(string &theString)
{
    theString = "Cargo Type \tAmount\n";
    char ctemp[4096];
    for(unsigned i = 0; i < (unsigned)_numCargoTypes; i++)
    {
        sprintf(ctemp, "%10s \t%i\n", CargoDescriptions[i], cargoDetail[i].amount);
        theString += ctemp;
    }
}

void CargoManifest::toXMLString(string &theString, const string &nodeName,
                                const string &scale) const
{
    theString = "";

    char tempString[2048];
    sprintf(tempString, "<%s TYPE=\"Ironium Boranium Germanium Fuel Colonists\" %s>%i %i %i %i %i</%s>\n",
            //   sprintf(tempString, "<%s>%i %i %i %i %i</%s>\n",
            nodeName.c_str(),
            scale.c_str(),
            cargoDetail[_ironium].amount,
            cargoDetail[_boranium].amount,
            cargoDetail[_germanium].amount,
            cargoDetail[_fuel].amount,
            cargoDetail[_colonists].amount,nodeName.c_str());

    theString += tempString;
}

unsigned CargoManifest::getMass(void) const
{
	unsigned massSum = 0;
	for(unsigned i = 0; i < (unsigned)_massLessTypes; i++)
		massSum += cargoDetail[i].amount;
	return massSum;
}

void CargoManifest::clear()
{
    for(unsigned i = 0; i < (unsigned)_massLessTypes; i++)
        cargoDetail[i].amount = 0;
}


