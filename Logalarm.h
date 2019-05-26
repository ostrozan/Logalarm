#pragma once
#ifndef _LOGO_h
#define _LOGO_h
#include <TimerOne.h>
//#include <SPI.h>
//#include "GsmModule.h"
#include "DateTime.h"
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
#define ALARM 0x0010
#define ALARM_ACT 0x0020
#define ALARM_DEACT 0x0030
#define ON_TEMP 0x0040
#define OFF_TEMP 0x0050
#define ON_SWCLOCK 0x0060
#define OFF_SWCLOCK 0x0070
#define ON_SMS 0x0080
#define OFF_SMS 0x0090
#define ON_RNG 0x00a0
#define OFF_RNG 0x00b0
#define SEND_SMS 0x00c0
#define GSM_QEST 0x00d0
#define SYS_ZAP_GSM 0x00e0
#define SYS_VYP_GSM 0x00f0

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
//typedef struct _TS {
//	uint8_t sec;         /* seconds */
//	uint8_t min;         /* minutes */
//	uint8_t hour;        /* hours */
//	uint8_t mday;        /* day of the month */
//	uint8_t mon;         /* month */
//	int16_t year;        /* year */
//	uint8_t wday;        /* day of the week */
//	uint8_t yday;        /* day in the year */
//	uint8_t isdst;       /* daylight saving time */
//	uint8_t year_s;      /* year in short notation*/
//#ifdef CONFIG_UNIXTIME
//	uint32_t unixtime;   /* seconds since 01.01.1970 00:00:00 UTC*/
//#endif
//}TS;
//TS ts;
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
	boolean isMonitorig;//odposlech pres mikrofon
}GsmData;

GsmData gsmData;
bool isGsmDn; //je gsm modul vypnuty
typedef enum { empty, first, second }enumFlag;

#pragma endregion

#pragma region promenne
char rxBuffer[105];
char txBuffer[80];
char rozdelenyString[7][7];
char divider[10];
const unsigned char inpNmbs[4] = { A2,A3,A4,A5 };
const unsigned char outNmbs[6] = { 4,5,6,7,8,9 };
int nmbOfSubstr;
In inputs[4];
Out outputs[6];
Out gsmOut;
int outTimers[6];
enumFlag whichTime[6];
boolean outTimersFlg[6];
boolean recMsg = false;

String gsmSignal ="0";
char rxBufferIndex;
char txBufferIndex;
int recChar;
boolean sendDateTimeFlg;
boolean sendOutsFlg;

boolean getGsmDt;//priznak pro nacteni datum cas z gsm
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
	char isMonitorig;//odposlech pres mikrofon
}TelNum;

char pocet_prozvoneni = 0;
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

boolean isDebug = false;
#define ONE_WIRE_BUS A1
#define TEMPERATURE_PRECISION 9 // Lower resolution



int numberOfFDallasDevices; // Number of temperature devices found

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address
DeviceAddress addr[2];

//alarm
char alarm_counter = 0;
char alarm_counter_zona = 0;//special pro typ zona
char loop_in_alarm;
boolean is_alarm_activated = false;
boolean system_active;
boolean is_active_entry_delay, is_active_exit_delay;
char entry_loop_active;

int entry_timer, exit_timer,alarm_timer,zone_out_timer,zone_block_timer;
boolean alarmT1HiBlocked =false, alarmT2HiBlocked=false, alarmT1LoBlocked = false, alarmT2LoBlocked = false;
boolean is_zone_out_blocked = false;
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

boolean tick;
#pragma endregion

#endif

