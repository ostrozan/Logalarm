// 
// 
// 

#include "DateTime.h"

DateTime::DateTime ()
{
}

DateTime::~DateTime ()
{
}

void DateTime::Init ()
{
	Wire.begin ();
	DS3231_init (DS3231_INTCN);
}

void DateTime::SetDateTime (ts ts/*char* dtstring[7][7]*/)
{

	SetDate (ts.mday, ts.mon, ts.year);
	SetTime (ts.hour, ts.min, ts.sec);
}

void DateTime::SetDate (uint8_t dd, uint8_t mm, uint16_t yyyy)
{
	dateTimeStr.mday = dd;
	dateTimeStr.mon = mm;
	dateTimeStr.year = yyyy;
	DS3231_set (dateTimeStr);

}

void DateTime::SetTime (uint8_t hh, uint8_t mm, uint8_t ss)
{
	dateTimeStr.hour = hh;
	dateTimeStr.min = mm;
	dateTimeStr.sec = ss;
	DS3231_set (dateTimeStr);
}

void DateTime::GetDateTime ()
{
	DS3231_get (&dateTimeStr);
}

void DateTime::GetDate ()
{
}

void DateTime::GetTime ()
{
}

int DateTime::GetMinutes ()
{
	return (dateTimeStr.hour * 60) + dateTimeStr.min;
}



String DateTime::ToString ()
{
	char str[22];
	sprintf (str, "%u.%u.%u  %02u:%02u:%02u", dateTimeStr.mday, dateTimeStr.mon, dateTimeStr.year, dateTimeStr.hour, dateTimeStr.min, dateTimeStr.sec);
	return String (str);
}



