#pragma once
#ifndef _LOGO_h
#define _LOGO_h
#include <TimerOne.h>
//#include <SPI.h>
//#include "GsmModule.h"
//#include "DateTime.h"
//#include <AT24CX.h>
#include "EEPROM.h"
#include <DallasTemperature.h> 
//#include <OneWire.h>
//#define DEBUG

//vstupy
#define LOCK   A0
#define SM1   A2
#define	SM2   A3
#define SM3   A4
#define SM4   A5

//SW TX   11
//SW rX   10
// 485 SEL  12
//TX  20
//RX  21

//vystupy
#define SIR  4//rele - sirena
#define GSM_OUT   5//rele ovladane gsm
#define AKTIV   6
#define SPIN_HOD   7
#define ZONA   8
#define TERMOSTAT   9



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
#define LOOP_ACT_LO u.s.loop_activate == '0'
#define LOOP_ACT_HI u.s.loop_activate == '1'
#pragma endregion
typedef struct _TS {
	uint8_t sec;         /* seconds */
	uint8_t min;         /* minutes */
	uint8_t hour;        /* hours */
	uint8_t mday;        /* day of the month */
	uint8_t mon;         /* month */
	int16_t year;        /* year */
	uint8_t wday;        /* day of the week */
	uint8_t yday;        /* day in the year */
	uint8_t isdst;       /* daylight saving time */
	uint8_t year_s;      /* year in short notation*/
#ifdef CONFIG_UNIXTIME
	uint32_t unixtime;   /* seconds since 01.01.1970 00:00:00 UTC*/
#endif
}TS;
TS ts;
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

typedef enum { no_use,inst,del,h24,zona} LoopTypes;
typedef enum { closed,open } LoopState;

typedef struct
{
	unsigned char input;
	unsigned char type;
	unsigned char state;
}AlarmLoop;
unsigned char alarm_loops[4] = { SM1,SM2,SM3,SM4 };
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
	long timeOfDelay;
	long timeOfPulse;
}ControlTimes;

typedef enum {sir,gsm,thermo,swclk} Outfunc;

typedef struct Out
{
	Outfunc func;
	char state=0;
	char nmb=0;
	boolean blockSendOn=0;
	boolean blockSendOff=0;
}Out;

struct 
{
	unsigned char out_num;

}Spinacky;

/////////GSM////////////
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
//ts pomTs;
//pro fci decode data
//char pom_strgs[6][12];
//char procenta[8] = { 0,0,0,0,0,0,0,0 };
//char pomlcky[6] = { 0,0,0,0,0,0 };
char maskIn;
//pro gsm

typedef struct
{
	char number[10];
	char is_sms_control;
	char is_ring_control;
	char send_sms;
	char ring;
	char is_waiting_to_send_sms;
	char is_waiting_to_calling;
}TelNum;


char casProzvaneni;//jak dlouho bude prozvanet
int casBlokovaniSms;
char casZpozdeniSms;//pri prozvoneni+ sms nutno pockat
char currCallingInput;//ktery vstup zrovna prozvani
char currSendingSmsInput;//ktery vstup zrovna poslal sms
char callingNumber;//index volajiciho cisla ze seznamu
///dallas
boolean measure_temp_flag;
volatile int teploty_new[2];
int teploty_old[2];
#define ONE_WIRE_BUS A1
#define TEMPERATURE_PRECISION 9 // Lower resolution



int numberOfFDallasDevices; // Number of temperature devices found

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address
DeviceAddress addr[2];

//alarm
char alarm_counter = 0;
char loop_in_alarm;
boolean is_alarm_activated = false;
boolean system_active;
boolean is_active_entry_delay, is_active_exit_delay;
char entry_loop_active;

int entry_timer, exit_timer,alarm_timer;
boolean alarmT1blocked =false, alarmT2blocked=false;
//spinacky
typedef struct
{
	int startTime;
	int stopTime;
}MinuteSpan;


typedef struct
{
	//alarm
		int entry_delay;
		int exit_delay;
		int time_alarm;
		int time_zone_activ;
		int time_zone_wait;
	    char loop_types[4];
		char loop_activate;
		//tel cisla,text sms povoleni atd...
		TelNum telNums[3];
		//spinacky
		MinuteSpan minutespans[4];
		//teploty
		int teplota;
		char hystereze;
		int alarmT1;
		int alarmT2;
		unsigned char aktivAlarmT1;//aktivace alarmu : 0 - pokles teploty pod nad nast.hodnotu, 1 - prekroceni nastavene hodoty
		unsigned char aktivAlarmT2;

}DataStruct;


union
{
	DataStruct s;
    char data[sizeof(DataStruct)];
}u;
#pragma endregion

#endif

