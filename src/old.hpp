class Region
{
public:
   bool pointIn(Location where);
};

class MineralObject
{
public:
    cargoIDType cargoID;
    unsigned    concentration;
};

class WaypointOrder
{
public:
   Location             where;
   unsigned             speed;
   std::vector<Action*> what;
};

class WaypointList
{
public:
   std::vector<WaypointOrder*> orderList;
};

enum EnvironmentalParameters
{
   TEMPERATURE = 0,
   RADIATION,
   GRAVITY,
   NUM_ENVIRONMENTAL_PARAMS
};

static const char     *PRT_ABBREVIATIONS[] =      
{
   "JoaT",
   "AR",
   "IT",
   "PP",
   "SD",
   "IS",
   "CA",
   "WM",
   "SS",
   "HE",
   "ERROR"
};

static const char *LRT_ABBREVIATIONS[] = 
{
   "IFE",
   "TT",
   "ARM",
   "Imp.S",
   "GR",
   "UR",
   "MA",
   "NRSE",
   "OBRM",
   "NAS",
   "LSP",
   "BET",
   "RS",
   "ERROR"
};

bool operator<=(const CargoManifest &lhs, const CargoManifest &rhs)
{
   for(int i = 0; i < _numCargoTypes; i++)
   {
      if( lhs.cargoDetail[i].amount > rhs.cargoDetail[i].amount &&
          !rhs.cargoDetail[i].isInfinite )
         return false;
   }
   return true;
}

bool operator>(const CargoManifest &lhs, const CargoManifest &rhs)
{
   for(int i = 0; i < _numCargoTypes; i++)
   {
      if( lhs.cargoDetail[i].amount <= rhs.cargoDetail[i].amount &&
          !lhs.cargoDetail[i].isInfinite )
         return false;
   }
   return true;
}

bool operator>=(const CargoManifest &lhs, const CargoManifest &rhs)
{
   for(int i = 0; i < _numCargoTypes; i++)
   {
      if( lhs.cargoDetail[i].amount < rhs.cargoDetail[i].amount &&
          !rhs.cargoDetail[i].isInfinite )
         return false;
   }
   return true;
}

bool operator==(const CargoManifest &lhs, const CargoManifest &rhs)
{
   for(int i = 0; i < _numCargoTypes; i++)
   {
      if( lhs.cargoDetail[i].amount != rhs.cargoDetail[i].amount &&
          !(rhs.cargoDetail[i].isInfinite && lhs.cargoDetail[i].isInfinite) )
        return false;
   }
   return true;
}
