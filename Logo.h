#pragma once
#ifndef _LOGO_h
#define _LOGO_h
#include <TimerOne.h>
//#include <SPI.h>
#include "GsmModule.h"
//#include "LogoBoard.h"
//#include "DateTime.h"
//#include <AT24CX.h>
#include "EEPROM.h"
#include <DallasTemperature.h> 
//#include <OneWire.h>
//#define DEBUG
//in1   A2
//in2   A3
//in3   A4
//in4   A5
//SW TX   11
//SW rX   10
// 485 SEL  12
//TX  20
//RX  21
//OUT 1   31
//OUT 2   30
//OUT 3   29
//OUT 4   28
//OUT 5   27
//OUT 6   26



#pragma region makra
//protokol udalosti
#define EE_EVENT_POINTER 558
#define START_POINT 560

#define RST 0x0000
#define ON_I 0x10
#define OFF_I 0x20
#define ON_TEMP1 0x30
#define OFF_TEMP1 0x40
#define ON_TEMP2 0x50
#define OFF_TEMP2 0x60
#define ON_SWATCH 0x70
#define OFF_SWATCH 0x80
#define ON_SWCLOCK 0x90
#define OFF_SWCLOCK 0xa0
#define ON_SMS 0xb0
#define OFF_SMS 0xc0
#define ON_RNG 0xd0
#define OFF_RNG 0xe0
#define ON_MAN 0xf0
#define OFF_MAN 0x100
#define SEND_SMS 0x200
#define RING 0x300
#define RING_SMS 0x400
#define GSM_QEST 0x500

#define CONTROLCOMBLUE
#ifdef CONTROLCOMBLUE// opro komunikaci s programem pouzit bluetooth
#define COMCONTROL swSerial
#define COMDEBUG Serial

#else // opro komunikaci s programem pouzit usb konektor
#define COMCONTROL Serial//
#define COMDEBUG swSerial
#define COMGSM Serial1

#endif // CONTROLCOMBLUE
#define INPUTS_CONTROL outputs[0].IsInputControl||outputs[1].IsInputControl||outputs[2].IsInputControl||outputs[3].IsInputControl||outputs[4].IsInputControl||outputs[5].IsInputControl

#pragma endregion

#pragma region typedefs
typedef struct
{
	char yy;
	char mnt;
	char day;
	char hr;
	char min;
	char ss;
	int evnt;
}Event;

Event event;


typedef struct In
{
	char func_index;
	char div1;
	char outs[6];
	char div2;
	char tel[10];
	char div3;
	char sms[21];
	char div4;
	char nmb;
	char div5;
	char state;
	//neposilat ven
	boolean blockSendOn;
	boolean blockSendOff;
	boolean isCallingGsm;//priznak ze prozvani
	boolean isSendingSms;//priznak ze poslal sms
	boolean isWaitingCall;//priznak ze ceka sms na odeslani po prozvoneni
	signed char counter;
}In;

typedef struct
{
	int startTime;
	int stopTime;
}MinuteSpan;

typedef struct
{
	long timeOfDelay;
	long timeOfPulse;
}ControlTimes;

typedef struct Out
{
	boolean IsTimeControl =0;
	boolean IsInputControl=0;
	boolean IsExtControl=0;
	boolean IsUseSwitchClk=0;
	boolean IsUseProgTmr=0;
	boolean IsUseThermostat=0;
	int Temperature=0;
	char TempHysteresis=0;
	int TempAlarmHi=0;
	int TempAlarmLo=0;
	boolean IsAlarmHi=0;
	boolean IsAlarmLo=0;
	char ktere_cidlo=0;
	boolean IsTrvale=0;
	boolean IsNastCas=0;
	boolean IsSwitchOn=0;
	boolean IsSwitchOff=0;
	boolean IsAnyChange=0;
	MinuteSpan minutespans[4];
	ControlTimes controlTimes;
	//neposilat ven
	char state=0;
	char nmb=0;
	boolean blockSendOn=0;
	boolean blockSendOff=0;
	char next_time_msg[6] = { 0,0,0,0,0,0 };
}Out;

typedef struct
{
    boolean isEnabled;
	boolean isResponse;
	char telNumber[10];
	char outNmb;//cislo vystupu ovladaneho externe prozvonenim
	boolean isRinging;
	boolean isActivated;
	boolean isFound;//bylo nalezeno spravne cislo
}GsmData;

GsmData gsmData;
bool isGsmDn; //je gsm modul vypnuty
typedef enum { empty, first, second }enumFlag;
#pragma endregion

#pragma region promenne
char rxBuffer[105];
char rozdelenyString[7][7];
char divider[10];
const unsigned char inpNmbs[4] = { A2,A3,A4,A5 };
const unsigned char outNmbs[6] = { 4,5,6,7,8,9 };
int nmbOfSubstr;
In inputs[4];
Out outputs[6];
int outTimers[6];
enumFlag whichTime[6];
boolean outTimersFlg[6];
boolean recMsg = false;

String gsmSignal ="0";
int rxBufferIndex;
int recChar;
boolean sendDateTimeFlg;
boolean sendOutsFlg;
volatile int minutes;
int eepromPtr;
//pro fci setdatetime
ts pomTs;
//pro fci decode data
//char pom_strgs[6][12];
//char procenta[8] = { 0,0,0,0,0,0,0,0 };
//char pomlcky[6] = { 0,0,0,0,0,0 };
char maskIn;
//pro gsm
char casProzvaneni;//jak dlouho bude prozvanet
int casBlokovaniSms;
char casZpozdeniSms;//pri prozvoneni+ sms nutno pockat
char currCallingInput;//ktery vstup zrovna prozvani
char currSendingSmsInput;//ktery vstup zrovna poslal sms
char callingNumber;//index volajiciho cisla ze seznamu
///dallas
volatile int teploty_new[2];
int teploty_old[2];
#define ONE_WIRE_BUS A0
#define TEMPERATURE_PRECISION 9 // Lower resolution



int numberOfFDallasDevices; // Number of temperature devices found

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address
DeviceAddress addr[2];
#pragma endregion

#endif

