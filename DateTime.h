// Clock.h

#ifndef _CLOCK_h
#define _CLOCK_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>
#include "ds3231.h"
//#include "rtc_ds3231.h"



class DateTime
{
public:

	struct ts dateTimeStr;

	DateTime ();
	~DateTime ();
	void Init ();
	void SetDateTime (ts ts/*char* dtstring[7][7]*/);
	void SetDate (uint8_t dd, uint8_t mm, uint16_t yyyy);
	void SetTime (uint8_t hh, uint8_t mm, uint8_t ss);
	void GetDateTime ();
	void GetDate ();
	void GetTime ();
	int GetMinutes ();
	String ToString ();

private:

};





#endif
