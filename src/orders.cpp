//
// C++ Implementation: orders
//
// Copyright: See COPYING file that comes with this distribution
//
#include "newStars.h"
#include "classDef.h"
#include "utils.h"
#include <iostream>

bool Order::parseXML(mxml_node_t *tree)
{
    mxml_node_t *child = mxmlFindElement(tree, tree, "PLAYERNUMBER", NULL,
                                         NULL, MXML_DESCEND);

    if( !child )
    {
        fprintf(stderr,"Error, could not locate PLAYERNUMBER keyword for ORDER parse\n");
        return false;
    }
    child = child->child;
    playerNumber = atoi(child->value.text.string);

    child = mxmlFindElement(tree, tree, "ORDERNUMBER", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr, "Error, could not locate ORDERNUMBER keyword for ORDER parse on player %i\n", playerNumber);
        return false;
    }
    child = child->child;
    orderNumber = atoi(child->value.text.string);

    child = mxmlFindElement(tree,tree,"ORDERSTRING",NULL,NULL,MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr,"Error, could not locate ORDERSTRING keyword while parsing player# %i, order #%i\n",playerNumber,orderNumber);
        return false;
    }
    child = child->child;
    assembleNodeArray(child, orderDescriptorString);

    child = mxmlFindElement(tree, tree, "ORDERID", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr,"Error, could not locate ORDERID keyword while parsing player# %i, order #%i\n",playerNumber,orderNumber);
        return false;
    }
    child = child->child;
    myOrderID = (orderID)atoi(child->value.text.string);

    child = mxmlFindElement(tree, tree, "OBJECTID", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr,"Error, could not locate OBJECTID keyword while parsing player# %i, order #%i\n",playerNumber,orderNumber);
        return false;
    }
    child = child->child;
    objectId = atoll(child->value.text.string);
    if (objectId >= GameObject::nextObjectId)
        GameObject::nextObjectId = objectId +1;

    child = mxmlFindElement(tree, tree, "X_COORD", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr, "Error, could not locate X_COORD keyword while parsing player# %i, order #%i\n",playerNumber,orderNumber);
        return false;
    }
    child = child->child;
    unsigned xLoc = atoi(child->value.text.string);

    unsigned valArray[6];
    int valCounter=0;
    child = mxmlFindElement(tree, tree, "Y_COORD", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr, "Error, could not locate Y_COORD keyword while parsing player# %i, order #%i\n",playerNumber,orderNumber);
        return false;
    }
    child = child->child;
    destination.set(xLoc, atoi(child->value.text.string));

    child = mxmlFindElement(tree,tree,"WARPFACTOR",NULL,NULL,MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr, "Error, could not locate WARPFACTOR keyword while parsing player# %i, order #%i\n",playerNumber,orderNumber);
        return false;
    }
    child = child->child;
    warpFactor = atoi(child->value.text.string);

    child = mxmlFindElement(tree,tree,"EXTRADATA",NULL,NULL,MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr, "Error, could not locate EXTRADATA keyword while parsing player# %i, order #%i\n",playerNumber,orderNumber);
        return false;
    }
    child = child->child;
	extraData.interceptRadius = atoi(child->value.text.string);

	child = mxmlFindElement(tree,tree,"TARGETID",NULL,NULL,MXML_DESCEND);
    if( !child )
    {
        fprintf(stderr, "Error, could not locate TARGETID keyword while parsing player# %i, order #%i\n",playerNumber,orderNumber);
        return false;
    }
    child = child->child;
    targetID = atoll(child->value.text.string);

    child = mxmlFindElement(tree, tree, "CARGOMANIFEST", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        cerr << "Error parsing file, expected CARGOMANIFEST data for player "
        << playerNumber << ", order " << orderNumber << endl;
        return false;
    }
    child = child->child;
    valCounter = assembleNodeArray(child,valArray,5);
    if( valCounter != 5 )
    {
        fprintf(stderr, "Error, expected 5 CARGOMANIFEST data points while parsing player# %i, order #%i\n",playerNumber,orderNumber);
        return false;
    }

    useJumpGate = false;
    whichJumpGate = 0;

    parseXML_noFail(tree,"USEJUMPGATE",useJumpGate);
    parseXML_noFail(tree,"WHICHJUMPGATE",whichJumpGate);

    cargoTransferDetails.cargoDetail[_ironium  ].init(valArray[0]);
    cargoTransferDetails.cargoDetail[_boranium ].init(valArray[1]);
    cargoTransferDetails.cargoDetail[_germanium].init(valArray[2]);
    cargoTransferDetails.cargoDetail[_fuel     ].init(valArray[3]);
    cargoTransferDetails.cargoDetail[_colonists].init(valArray[4]);

    return true;
}

void Order::toXMLString(string &theString) const

{
    theString = "";
    char tempString[2048];

    theString = "<ORDER>\n";

    sprintf(tempString,"\
            <PLAYERNUMBER>%i</PLAYERNUMBER>\n\
            <ORDERNUMBER>%i</ORDERNUMBER>\n\
            <ORDERSTRING>%s</ORDERSTRING>\n\
            <ORDERID>%i</ORDERID>\n\
            <OBJECTID>%lu</OBJECTID>\n\
            <TARGETID>%lu</TARGETID>\n\
            <X_COORD>%i</X_COORD>\n<Y_COORD>%i</Y_COORD>\n\
			<EXTRADATA>%i</EXTRADATA>\n",
            playerNumber,orderNumber,orderDescriptorString.c_str(),myOrderID,objectId,
			targetID, destination.getX(), destination.getY(),extraData.interceptRadius);

    theString += tempString;

    sprintf(tempString,"<WARPFACTOR>%i</WARPFACTOR>\n", warpFactor);
    theString += tempString;
    sprintf(tempString,"<USEJUMPGATE>%i</USEJUMPGATE>\n", useJumpGate);
    theString += tempString;
    sprintf(tempString,"<WHICHJUMPGATE>%i</WHICHJUMPGATE>\n", whichJumpGate);
    theString += tempString;

    sprintf(tempString,
            "<CARGOMANIFEST TYPE=\"Ironium Boranium Germanium Fuel Colonists\" SCALE=\"KiloTons\">%i %i %i %i %i</CARGOMANIFEST>\n",
            cargoTransferDetails.cargoDetail[_ironium].amount,
            cargoTransferDetails.cargoDetail[_boranium].amount,
            cargoTransferDetails.cargoDetail[_germanium].amount,
            cargoTransferDetails.cargoDetail[_fuel].amount,
            cargoTransferDetails.cargoDetail[_colonists].amount);
    theString += tempString;

    theString += "</ORDER>\n";
}

Order::Order(void)
{
	orderDescriptorString = "BADFOOD";
	useJumpGate = false;
	whichJumpGate = 0;
	extraData.timespan = 0;
	targetID = 0;
}

bool Order::processOrder(GameData &theGame)
{
    return false;
}

