#ifndef CARGO_H
#define CARGO_H

#include <string>

struct mxml_node_s;

// all the things that can be stored and moved about
enum cargoIDType
{
	_colonists=0,

	// mineral wealth
	_ironium,
	_mineralBegin = _ironium,
	_germanium,
	_boranium,
	_mineralEnd = _boranium,

	// start enumeration of massless cargo
	_massLessTypes,
	_fuel = _massLessTypes,

	_numCargoTypes
};

// one bin of cargo
class CargoItem
{
public: // data
	cargoIDType cargoID;
	unsigned    amount;
	bool        isInfinite; // e.g. fuel on planets

public:
	CargoItem(void)
	{
		init(0);
	}

	void init(unsigned amt)
	{
		cargoID     = _numCargoTypes;
		isInfinite  = false;
		amount      = amt;
	}
};

// a collection of all types of bins
class CargoManifest
{
public:
	CargoItem cargoDetail[_numCargoTypes];

public:
	// needed for STL containers
	bool operator<(const CargoManifest &rhs) const;

	bool operator==(const CargoManifest &rhs) const;
	bool operator!=(const CargoManifest &rhs) const
	{ return !(*this == rhs); }

	// is any bin in 'this' greater than 'rhs'
	// (good for checking for conservation)
	bool anyGreaterThan(const CargoManifest &rhs) const;

	CargoManifest operator+(const CargoManifest &param);
	CargoManifest operator-(const CargoManifest &param);

	// used for salvage, 'mineralValue * numShipsLost / 3'
	CargoManifest operator*(unsigned param);
	CargoManifest operator/(unsigned param);

	// reset all bins to zero
	void clear(void);

	// sum the massful bins
	unsigned getMass(void) const;

	// serialization
	bool parseXML(mxml_node_s *tree, const std::string &nodeName);
	void toXMLString(std::string &theString, const std::string &nodeName,
	                 const std::string &scale) const;
	void toCPPString(std::string &theString);
};

#endif
