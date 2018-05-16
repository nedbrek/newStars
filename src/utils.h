#ifndef utils__h
#define utils__h

#include <string>
#include <vector>
#include "mxml.h"

#ifndef MSVC
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
#endif

bool parseXML_noFail  (mxml_node_t *tree, std::string nodeName, int         &target);
bool parseXML_noFail  (mxml_node_t *tree, std::string nodeName, unsigned    &target);
bool parseXML_noFail  (mxml_node_t *tree, std::string nodeName, uint64_t    &target);
bool parseXML_noFail  (mxml_node_t *tree, std::string nodeName, std::string &target);
bool parseXML_noFail  (mxml_node_t *tree, std::string nodeName, bool &target);
bool parseXML_noFail  (mxml_node_t* tree, std::string nodeName, double &target);

void assembleNodeArray(mxml_node_t *node, std::string  &theString); //assembles all elements into theString
int  assembleNodeArray(mxml_node_t *node, unsigned      theArray[], int size);//puts each element into the array and returns number of elements parsed
int  assembleNodeArray(mxml_node_t *node, std::string   theArray[], int size);
int  assembleNodeArray(mxml_node_t *node, int           theArray[], int size);
const char *                            /* O - Whitespace string or NULL */
whitespace_cb(mxml_node_t *node,        /* I - Element node */
              int         where);        /* I - Open or close tag? */

void vectorToString(std::string &theString, std::vector<unsigned> vec);
#endif

