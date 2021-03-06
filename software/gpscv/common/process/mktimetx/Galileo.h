//
//
// The MIT License (MIT)
//
// Copyright (c) 2017  Michael J. Wouters
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef __GALILEO_H_
#define __GALILEO_H_

#include <time.h>
#include <vector>

#include "GNSSSystem.h"

class Antenna;
class ReceiverMeasurement;
class SVMeasurement;

typedef double DOUBLE;
typedef float  SINGLE;

class Galileo: public GNSSSystem
{
	public:
	
	class IonosphereData
	{
		public:
			SINGLE ai0,ai1,ai2;
	};

	class UTCData
	{
		public:
			
	};

	class EphemerisData
	{
		public:
			
	};
	
	Galileo();
	~Galileo();
	
	virtual int nsats(){return NSATS;}
	virtual void deleteEphemeris();
	
	IonosphereData ionoData;
	UTCData UTCdata;
	std::vector<EphemerisData *> ephemeris;
			
	bool currentLeapSeconds(int mjd,int *leapsecs);
	
	private:
		
		static const int NSATS=32;
		
};

#endif



