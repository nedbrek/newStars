#ifndef ENVIRON_H
#define ENVIRON_H

class Environment
{
public: // data
   int temp;
   int rad;
   int grav;

public: // methods
   Environment(void)
   {
      temp = 0;
      rad  = 0;
      grav = 0;
   }

   void clear(void)
   {
      temp = 0;
      rad  = 0;
      grav = 0;
   }

   Environment operator+(const Environment &param);
   Environment operator-(const Environment &param);

   bool operator==(const Environment &param);
};

#endif
