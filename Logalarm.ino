//
//#include <watchdogHandler.h>
//#include <avr\wdt.h>
#include <EEPROM.h>
#include "GsmModule.h"
#include "Logalarm.h"
//#include "DateTime.h"
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire (ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors (&oneWire);
//DateTime dateTime;
GsmModule GSM;
boolean firstLoop = true;
void setup ()
{
	Serial.begin (38400); delay (2000);
	//while (!Serial);
	digitalWrite (ZONA, HIGH);
	COMGSM.begin (9600);
	while (!COMGSM);
	GSM.Init ();
	alarm_loops[0] = { SM1 };
	alarm_loops[1] = { SM2 };
	alarm_loops[2] = { SM3 };
	alarm_loops[3] = { SM4 };

	Spinacky.out_num = SPIN_HOD;
	
	//I/O setup
	pinMode (LOCK, INPUT_PULLUP);
	pinMode (SM1, INPUT_PULLUP);
	pinMode (SM2, INPUT_PULLUP);
	pinMode (SM3, INPUT_PULLUP);
	pinMode (SM4, INPUT_PULLUP);
	pinMode (SIR, OUTPUT);
	pinMode (GSM_OUT, OUTPUT);
	pinMode (AKTIV, OUTPUT);
	pinMode (SPIN_HOD, OUTPUT);
	pinMode (ZONA, OUTPUT);
	pinMode (TERMOSTAT, OUTPUT);
	pinMode (17, OUTPUT); // Set RX LED as an output
	//outputs

	//DS18B20
	// Grab a count of devices on the wire
	sensors.begin ();

	// Grab a count of devices on the wire
	numberOfFDallasDevices = sensors.getDeviceCount ();
	for (int i = 0; i < numberOfFDallasDevices; i++)
	{
		// Search the wire for address
		sensors.getAddress (tempDeviceAddress, i);
		sensors.getAddress (addr[i], i);
		// set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
		sensors.setResolution (tempDeviceAddress, TEMPERATURE_PRECISION);
	}
	//dateTime.Init ();
	Timer1.initialize ();
	Timer1.attachInterrupt (TimerTick);
	EEPROM.get (0, u.data);
	gsmData.outNmb = GSM_OUT;
	gsmOut.nmb = GSM_OUT;
	//setup_watchdog (WDTO_30MS);
	COMGSM.print ("AT+CCLK?\n\r");
	delay (2000);
}

void loop ()
{


	//wdt_reset ();
	//komunikace prijem GSM
	if (COMGSM.available ()) GsmReceive ();
	////komunikace prijem zpravy
	if (Serial.available ())GetSerialData ();
	GsmAlarm ();
	TempControl ();
	//posli cas a datum
	if (sendDateTimeFlg)
	{

		digitalWrite (17, tick ? HIGH : LOW);
		SendDateTime ();

	}
	//nacteni datum cas y gsm
	if (getGsmDt)
	{
		getGsmDt = false;
		COMGSM.print ("AT+CCLK?\n\r");

	}
	//mereni teplot
	if (measure_temp_flag)
	{
		measure_temp_flag = false;
		CtiTeploty ();
	}
    //kontroluj vstup pro aktivaci/deaktivaci
	TestActivity ();

	if (is_active_entry_delay && !is_alarm_activated)//je aktivovan prichodovy cas
	{
		if (EntryTimeout ())
		{
			is_active_entry_delay = false;
			Alarm (entry_loop_active,del);
		}
	}

	for (int i = 0; i < 4; i++)//projdi hlidaci smycky
	{
		boolean is_loop_break = digitalRead (alarm_loops[i]);//optoclen spina zem
		if (LOOP_ACT_LO)is_loop_break = ~is_loop_break;//primy vstup bez optoclenu
		if (is_loop_break)//aktivovana smycka?
		{
			if (isDebug) { Serial.print ("loop"); Serial.println (i, 10); }
			if (system_active)//aktivni alarm?
			{
				switch (u.s.loop_types[i])
				{
				case no_use: break;//nic
				case inst:if (!is_alarm_activated) Alarm (i, u.s.loop_types[i]); break;//alarm hned
				case del: entry_loop_active = i; EntryTimeout (); break;//aktivuj prichodovy cas
				case h24: if (!is_alarm_activated)Alarm (i, u.s.loop_types[i]); break;//alarm hned
				case zona:if (!zone_out_blocked) Alarm (i, u.s.loop_types[i]);  break;
				}
			}
			else //neaktivni alarm
			{
				switch (u.s.loop_types[i])
				{
				case no_use: break;//nic
				case inst: if (exit_timer)if (!is_alarm_activated) Alarm (i, u.s.loop_types[i]); break;// casuje odchodovy cas inst uz hlida - alarm hned
				case del: break;//nic
				case h24:if (!is_alarm_activated) Alarm (i, u.s.loop_types[i]); break;//alarm hned
				case zona:  break;
				}
			}
		}
	}
}


boolean pushButton = false;
unsigned long tmrButt;

void TestActivity ()
{

	int state = digitalRead (LOCK);
	

	if (!state && !pushButton)
	{
		if (tmrButt == 0)tmrButt = millis (); 

		else if (millis() > tmrButt + 1500)//stisk tlacitka 1,5 sec
		{
			tmrButt = 0;
			pushButton = true;
			if (isDebug)Serial.print ("push");
			//Serial.println ();
			//while (digitalRead (LOCK));
		}

	}
	else tmrButt = 0;

	if (pushButton)
	{
		//pushButton = false;
		tmrButt = 0;
		if (!system_active)
		{
			system_active = ExitTimeout ();
			if (system_active)pushButton = false;
		}

		else if (system_active)
		{
			//Serial.println ();
			if (isDebug)Serial.print ("disarm");
			digitalWrite (AKTIV, LOW);
			system_active = false;
			is_active_entry_delay = false;//shod priznak vstupniho zpozdeni
			entry_timer = 0;//vynuluj vstupni timer	
			is_alarm_activated = false;//shod priznak aktivovaneho alarmu
			digitalWrite (SIR, LOW);//vypni sirenu, pokud je zapla
			alarm_timer = 0;//vynuluj casovac alarmu
			alarm_counter = 0;//vynuluj pocitadlo alarmu
			pushButton = false;
			SaveEvent (ALARM_DEACT, 0);//uloz do pameti udalosti
		}
	}
}

boolean EntryTimeout ()
{
	if (!is_active_entry_delay)
	{
		if (isDebug)Serial.println ("active entry");
		is_active_entry_delay = true;
		entry_timer = u.s.entry_delay;
	}
	else if (entry_timer == 0)return true;
	return false;
}

boolean ExitTimeout ()
{
	if (!is_active_exit_delay)
	{
		is_active_exit_delay = true;//nastav priznak odpocitavani odchodu
		exit_timer = u.s.exit_delay;//napln timer nastavenou hodnotou
	}
	else if (exit_timer == 0)
	{
		if (isDebug)Serial.println ("arm");
		digitalWrite (AKTIV, HIGH);
		is_active_exit_delay = false;//shod priznak odpocitavani odchodu
		return true;//je aktivni odpocitavani odchodu a uplyunl nastaveny cas?

	}
	return false;
}

void Alarm (char loop_num ,char loop_typ)
{
	if (!is_alarm_activated)
	{
		for (int i = 0; i < 3; i++)
		{
			if (u.s.telNums[i].ring && !u.s.telNums[i].is_waiting_to_calling)u.s.telNums[i].is_waiting_to_calling = true;
			if(loop_typ != zona)if (u.s.telNums[i].send_sms && !u.s.telNums[i].is_waiting_to_send_sms)u.s.telNums[i].is_waiting_to_send_sms = true;
		}


		if (loop_typ == zona)
		{
			if (alarm_counter_zona < 10)//10x opakovani pro smycku zona
			{
				loop_in_alarm = loop_num;
				alarm_counter_zona++;
				if (isDebug){Serial.print ("alarm");
				Serial.print (alarm_counter, 10);
				Serial.println (loop_num, 10);}
				is_alarm_activated = true;
				digitalWrite (SIR, HIGH);//zapni sirenu
				digitalWrite (ZONA, HIGH);//zapni vystup zona(svetlo ?)
				alarm_timer = u.s.time_alarm;//nastav casovac sireny
				zone_out_timer = u.s.time_zone_activ;
				SaveEvent (ALARM, loop_num);
			}
		}

		else if (alarm_counter < 3)
		{
			loop_in_alarm = loop_num;
			alarm_counter++;
			if (isDebug) {
				Serial.print ("alarm");
				Serial.print (alarm_counter, 10);
				Serial.println (loop_num, 10);
			}
			is_alarm_activated = true;
			digitalWrite (SIR, HIGH);//zapni sirenu
			alarm_timer = u.s.time_alarm;//nastav casovac sireny
			SaveEvent (ALARM, loop_num);
		}
	}
}

void GsmAlarm ()
{

	if (casProzvaneni || casZpozdeniSms)return;//pokud zrovna prozvani  - nepokracuj
	//pokud neceka zadna sms na odeslani
	if (!u.s.telNums[0].is_waiting_to_send_sms && !u.s.telNums[1].is_waiting_to_send_sms && !u.s.telNums[2].is_waiting_to_send_sms)
	{
		//if (isDebug)Serial.println ("gsmalarm");
		//prozvoneni
		for (int i = 0; i < 3; i++)
		{

			if (u.s.telNums[i].is_waiting_to_calling)
			{
				u.s.telNums[i].is_waiting_to_calling = false;
				GSM.Call (u.s.telNums[i].number);
				casProzvaneni = 30;//20 sec na prozvoneni jednoho cisla
				break;
			}
		}
	}

	else //nejdriv posli smsky
	{
		for (int i = 0; i < 3; i++)
		{
			if (u.s.telNums[i].is_waiting_to_send_sms)
			{
				char buff[17];
				sprintf (buff, "poplach smycka %u", loop_in_alarm);
				u.s.telNums[i].is_waiting_to_send_sms = false;
				GSM.Sms (u.s.telNums[i].number, buff);
				casZpozdeniSms = 5;//5 sec mezi jednotlivymi sms a pripadnym prozvanenim
				break;
			}
		}

	}
	//sms

}


void TimerTick ()
{
	static  boolean blockSetOn, blockSetOff ;
	tick = !tick;
	if (++ts.sec > 59)
	{
		ts.sec = 0;
		minutes = (ts.hour * 60) + ts.min;
		if (isDebug)Serial.println (minutes, 10);
		if (++ts.min > 59)
		{
           ts.min = 0;
		   getGsmDt = true;
		   if (++ts.hour > 23)
		   {
			   ts.mday++;
		   }
		}
			
	}
	sendDateTimeFlg = true;
	//tick = !tick;
	//digitalWrite (ZONA,tick? HIGH : LOW);
	for (int i = 0; i < 4; i++)
	{
		if (firstLoop)
		{
			firstLoop = false;
			break;
		}
		if (minutes == u.s.minutespans[i].startTime && blockSetOn == false)
		{
			blockSetOn = true;
			blockSetOff = false;
			digitalWrite (SPIN_HOD, HIGH);
			SaveEvent (ON_SWCLOCK, 0);
			if (isDebug)Serial.println ("swckOn");
		}
		if (minutes == u.s.minutespans[i].stopTime && blockSetOff == false)
		{
			blockSetOff = true;
			blockSetOn = false;
			digitalWrite (SPIN_HOD, LOW);
			SaveEvent (OFF_SWCLOCK, 0);
			if (isDebug)Serial.println ("swckOff");
		}
	}
	static char temp_timer = 10;
	if (--temp_timer == 0)
	{
		temp_timer = 10;
		measure_temp_flag = true;
	}
	//sendDateTimeFlg = true;
	if (exit_timer)
	{
		digitalWrite (AKTIV, tick ? HIGH : LOW);
		if (isDebug)Serial.println (exit_timer, 10);
		if (--exit_timer == 0)
		{
			if (isDebug)Serial.println ("odchod docasovan");
			SaveEvent (ALARM_ACT, 0);
		}
	}

	if (entry_timer)
	{
		if (isDebug)Serial.println (entry_timer, 10);
		if (--entry_timer == 0)
		{
			//	is_active_entry_delay = false;
			if (isDebug)Serial.println ("prichod docasovan");

		}
	}

	if (alarm_timer)
	{
		if (--alarm_timer == 0)
		{
			is_alarm_activated = false;
			digitalWrite (SIR, LOW);//vypni sirenu
			if (isDebug)Serial.println ("sirena off");
		}
	}

	if (zone_out_timer)
	{
		if (--zone_out_timer == 0)
		{
			digitalWrite (ZONA, LOW);//vypni vystup zona
			zone_block_timer = u.s.time_zone_wait;//blokuj zonu na nastaveny cas
			zone_out_blocked = true;
		}
	}

	if (zone_block_timer)if (--zone_block_timer == 0)zone_out_blocked = false;

	if (casProzvaneni > 0)
	{
		Serial.print ("ring");
		if (isDebug)Serial.println (casProzvaneni, 10);
		casProzvaneni--;
		if (casProzvaneni == 5)
		{
			GSM.HangOut ();
			delay (100);
		}

		if (casProzvaneni <= 1)
		{
			casProzvaneni = 0;
			//COMDEBUG.println ("ath");

		}

	}

	if (casZpozdeniSms)
	{
		if (isDebug){Serial.print ("sms");
		Serial.println (casZpozdeniSms, 10);}
		casZpozdeniSms--;
		//	//COMDEBUG.println (casZpozdeniSms, 10);
			//if (--casZpozdeniSms == 0)SendLocalSms (currCallingInput);//
		//}
		//for (int i = 0; i < 4; i++)
		//{
		//	if (inputs[i].isWaitingCall)
		//	{
		//		//COMDEBUG.println ("aaa");
		//		if (inputs[i].counter == -1)inputs[i].counter = 10;
		//		if (inputs[i].counter > 0)
		//		{
		//			inputs[i].counter--;
		//			//COMDEBUG.println (inputs[i].counter,10);
		//			if (inputs[i].counter == 0)
		//			{

		//				inputs[i].isWaitingCall = false;
		//				inputs[i].counter = -1;
		//				GSM.Call (inputs[i].tel);
		//				casProzvaneni = 30;
		//				inputs[i].isCallingGsm = true;
		//				currCallingInput = i;
		//			}
		//		}
		//	}
	}
	//if (casBlokovaniSms)
	//{
	//	//COM.println (casBlokovaniSms);
	//	if (--casBlokovaniSms == 0)inputs[currSendingSmsInput].isSendingSms = false;
	//}
	//for (int i = 0; i < 6; i++)
	//{
	//	if (outTimers[i] > 0)
	//	{
	//		if (--outTimers[i] == 0)
	//		{
	//			outTimersFlg[i] = true;
	///*			COMCONTROL.print (i, 10);
	//			COMCONTROL.println ("flg");*/
	//		}
	//		else
	//		{
	//			//COMCONTROL.println (String (i) + "tmr-" + String (outTimers[i]) + '<');
	//		}

	//		delay (100);
	//	}
	//}
}

void CtiTeploty ()
{
	static char cnt;
	//unsigned long time = millis ();
	sensors.requestTemperatures (); // Send the command to get temperatures

	for (int i = 0; i < 2; i++)//numberOfDevices
	{

		// Search the wire for address
		if (sensors.getAddress (tempDeviceAddress, i))
		{
			teploty_new[i] = (int)(sensors.getTempC (tempDeviceAddress) * 10); //sensors.getTempCByIndex(i);
																			   //COM.println (teploty_new[i]);
		}

		if (teploty_old[i] != teploty_new[i])
		{
			teploty_old[i] = teploty_new[i];

		}

	}
	char pomstr[10];
	//sprintf (pomstr, "%2u,%1u %2u,%1u", teploty_new[0] / 10, teploty_new[0] % 10, teploty_new[1] / 10, teploty_new[1] % 10);
	//Serial.println (pomstr);
	//unsigned long cas = millis () - time;
	//COMDEBUG.println (cas, 10);
}

void GetSerialData ()
{

	while (Serial.available ())
	{
		recChar = Serial.read ();
		rxBuffer[rxBufferIndex] = (char)recChar;
		if (rxBuffer[rxBufferIndex - 1] == 0x0d && recChar == 0x0a)
		{
			rxBufferIndex = 0;
			recMsg = true;
			Serial.println ("OKrec");
			break;
		}
		rxBufferIndex++;
	}


	switch (rxBuffer[0])
	{
	default:
		break;
	case 'D': isDebug = (rxBuffer[1] == 'S') ? true : false; break;
	case 'E': VypisPametUdalosti (); break;
	case 'C': ClearEventList (); break;
	case 'W': WriteData (); break;
	case 'R': ReadData (); break;
	}


	//ClearRxBuffer ();
}


void ReadData ()
{
	String pomstr;
	int pom;
	Serial.print (u.s.entry_delay, 10);
	//Serial.print (pom,2);		
	Serial.write (' ');
	Serial.print (u.s.exit_delay, 10);
	//Serial.write (pom, 2);
	Serial.print (' ');
	Serial.print (u.s.time_alarm, 10);
	//Serial.write (pom, 2);
	Serial.write (' ');
	Serial.print (u.s.time_zone_activ, 10);
	//Serial.write (pom, 2);
	Serial.write (' ');
	Serial.print (u.s.time_zone_wait, 10);
	//Serial.write (pom, 2);
	Serial.print (' ');
	Serial.print (u.s.loop_types[0], 10);
	Serial.print (u.s.loop_types[1], 10);
	Serial.print (u.s.loop_types[2], 10);
	Serial.print (u.s.loop_types[3], 10);
	Serial.print (' ');
	Serial.print (u.s.loop_activate, 10);
	Serial.print (' ');
	//tel 1
	//pom = atoi (u.s.telNums[0].number);
	Serial.print (String (u.s.telNums[0].number));
	Serial.print (' ');
	Serial.print (u.s.telNums[0].is_sms_control, 10);
	Serial.print (' ');
	Serial.print (u.s.telNums[0].is_ring_control, 10);
	Serial.print (' ');
	Serial.print (u.s.telNums[0].send_sms, 10);
	Serial.print (' ');
	Serial.print (u.s.telNums[0].ring, 10);
	Serial.print (' ');
	//tel 2
	//	pom = atoi (u.s.telNums[1].number);
	Serial.print (String (u.s.telNums[1].number));
	Serial.print (' ');
	Serial.print (u.s.telNums[1].is_sms_control, 10);
	Serial.print (' ');
	Serial.print (u.s.telNums[1].is_ring_control, 10);
	Serial.print (' ');
	Serial.print (u.s.telNums[1].send_sms, 10);
	Serial.print (' ');
	Serial.print (u.s.telNums[1].ring, 10);
	Serial.print (' ');
	//tel 3
	//	pom = atoi (u.s.telNums[2].number);
	Serial.print (String (u.s.telNums[2].number));
	Serial.print (' ');
	Serial.print (u.s.telNums[2].is_sms_control, 10);
	Serial.print (' ');
	Serial.print (u.s.telNums[2].is_ring_control, 10);
	Serial.print (' ');
	Serial.print (u.s.telNums[2].send_sms, 10);
	Serial.print (' ');
	Serial.print (u.s.telNums[2].ring, 10);
	Serial.print (' ');
	//spinacky
	Serial.print (u.s.minutespans[0].startTime, 10);
	Serial.print (':');
	Serial.print (u.s.minutespans[0].stopTime, 10);
	Serial.print ('/');
	Serial.print (u.s.minutespans[1].startTime, 10);
	Serial.print (':');
	Serial.print (u.s.minutespans[1].stopTime, 10);
	Serial.print ('/');
	Serial.print (u.s.minutespans[2].startTime, 10);
	Serial.print (':');
	Serial.print (u.s.minutespans[2].stopTime, 10);
	Serial.print ('/');
	Serial.print (u.s.minutespans[3].startTime, 10);
	Serial.print (':');
	Serial.print (u.s.minutespans[3].stopTime, 10);

	Serial.print (' ');
	//teploty
	Serial.print (u.s.teplota, 10);
	Serial.print (':');
	Serial.print (u.s.hystereze, 10);
	Serial.print (':');
	Serial.print (u.s.alarmT1, 10);
	Serial.print (':');
	Serial.print (u.s.alarmT2, 10);
	Serial.print (':');
	Serial.print (u.s.aktivAlarmT1, 10);
	Serial.print (':');
	Serial.print (u.s.aktivAlarmT2, 10);
	Serial.print (' ');
	Serial.println ("data");
}
void WriteData ()
{
	//memcpy (u.data, &rxBuffer[1], sizeof (u));

	char pom[18];
	memcpy (&u.s.entry_delay, &rxBuffer[1], 2);
	memcpy (pom, &u.s.entry_delay, 2);
	//Serial.write (pom, 2);
	memcpy (&u.s.exit_delay, &rxBuffer[3], 2);
	//Serial.write (pom, 2);
	memcpy (&u.s.time_alarm, &rxBuffer[5], 2);
	//Serial.write (pom, 2);
	memcpy (&u.s.time_zone_activ, &rxBuffer[7], 2);
	//Serial.write (pom, 2);
	memcpy (&u.s.time_zone_wait, &rxBuffer[9], 2);
	//Serial.write (pom, 2);
	memcpy (u.s.loop_types, &rxBuffer[11], 4);
	memcpy (&u.s.loop_activate, &rxBuffer[12], 1);

	//tel 1
	memcpy (u.s.telNums[0].number, &rxBuffer[16], 10);
	memcpy (&u.s.telNums[0].is_sms_control, &rxBuffer[26], 1);
	memcpy (&u.s.telNums[0].is_ring_control, &rxBuffer[27], 1);
	memcpy (&u.s.telNums[0].send_sms, &rxBuffer[28], 1);
	memcpy (&u.s.telNums[0].ring, &rxBuffer[29], 1);
	//tel 2
	memcpy (u.s.telNums[1].number, &rxBuffer[32], 10);
	memcpy (&u.s.telNums[1].is_sms_control, &rxBuffer[42], 1);
	memcpy (&u.s.telNums[1].is_ring_control, &rxBuffer[43], 1);
	memcpy (&u.s.telNums[1].send_sms, &rxBuffer[44], 1);
	memcpy (&u.s.telNums[1].ring, &rxBuffer[45], 1);
	//tel 3
	memcpy (u.s.telNums[2].number, &rxBuffer[48], 10);
	memcpy (&u.s.telNums[2].is_sms_control, &rxBuffer[58], 1);
	memcpy (&u.s.telNums[2].is_ring_control, &rxBuffer[59], 1);
	memcpy (&u.s.telNums[2].send_sms, &rxBuffer[60], 1);
	memcpy (&u.s.telNums[2].ring, &rxBuffer[61], 1);
	//spinacky
	memcpy (u.s.minutespans, &rxBuffer[64], 16);
	//Serial.write (pom, 16);
	//teploty
	memcpy (&u.s.teplota, &rxBuffer[80], 2);
	//Serial.write (pom, 2);
	memcpy (&u.s.hystereze, &rxBuffer[82], 1);
	//Serial.write (pom, 2);
	memcpy (&u.s.alarmT1, &rxBuffer[83], 2);
	//Serial.write (pom, 2);
	memcpy (&u.s.alarmT2, &rxBuffer[85], 2);
	//Serial.write (pom, 2);
	memcpy (&u.s.aktivAlarmT1, &rxBuffer[87], 1);
	memcpy (&u.s.aktivAlarmT2, &rxBuffer[88], 1);
	Serial.println ("ack");
	EEPROM.put (0, u.data);
}
boolean isSms = false;
int rx_index;
void GsmReceive ()
{
	Timer1.stop ();
	boolean isRec = false;
	String gsm_string;
	char c;
	signed int start, end;
	String telnmb;//pomocna pro tel cislo
	//if (!isSms)gsm_string = "";
	//else isSms = false;
	while (isRec == false)
	{
		
		if (COMGSM.available ())
		{
			c = (char)COMGSM.read ();
			
			if (c == 'M')
			{
				isSms = true; 				
			}
		    if (c == '\n')
			{
				if (isSms == true) {isSms = false; if (isDebug)Serial.println ('#'); continue; }
				else isRec = true;
			}
			rxBuffer[rxBufferIndex++] = c;
			//Serial.print (c);
		}
	}
	
	rxBufferIndex = 0;
	_delay_ms (20);
	gsm_string = String (rxBuffer);
	if (isDebug)Serial.println (gsm_string);

	if (gsm_string.indexOf ("Q:") != -1)
	{
		//Serial.println (">>Q");
		start = gsm_string.indexOf ("Q:");
		start += 3;
		end = start + 2;
		gsmSignal = String (gsm_string).substring (start, end);
		//COMDEBUG.println (gsmSignal);
	}

	else if (gsm_string.indexOf ("+420") != -1)
	{
		//Serial.println (">>+");
		start = gsm_string.indexOf ("+420");
		telnmb = gsm_string.substring (start + 4, start + 13);
		//Serial.println (telnmb);
	}

	if (gsm_string.indexOf ("T:") != -1)
	{
		//Serial.println (">>T");
			start = gsm_string.length () - 5;
			String command = gsm_string.substring (start, start + 3);
			//Serial.print ("OK1");
			//Serial.println (gsm_string);
			char pom2[4];//pomocna pro prikaz
						 command.toCharArray (pom2, 4);
						 //Serial.println (pom2);
			char nmbx = pom2[0];
			if (pom2[0] == '?')
			{
				SendStatus ();
				SaveEvent (GSM_QEST, 0);
			}
			else if (pom2[2] == 'n' || pom2[2] == 'f')
			{
				gsmOut.state = (pom2[2] == 'n') ? 1 : 0;
				ChangeOutput (gsmOut, (pom2[2] == 'n') ? ON_SMS : OFF_SMS);
			}
		
	}



	//DATE TIME
	else if (gsm_string.indexOf ("K:") != -1)//CCLK
	{
		//Serial.println (">>C");
		start = gsm_string.indexOf ("K:");
		//Serial.print ("index "); Serial.println (start, 10);
		if (start != -1)
		{
			char c[3];
			start -= 4;
			 memcpy(c,&gsm_string[start + 8] ,3);
			ts.year = atoi (c) + 2000;
			memcpy (c, &gsm_string[start + 11], 3);
			ts.mon = atoi (c);
			memcpy (c, &gsm_string[start + 14], 3);
			ts.mday = atoi (c);
			memcpy (c, &gsm_string[start + 17], 3);
			ts.hour = atoi (c);
			memcpy (c, &gsm_string[start + 20], 3);
			ts.min = atoi (c);
			 memcpy(c,&gsm_string[start + 23] ,3);
			ts.sec = atoi (c);

		}
	}

	else if (gsm_string.indexOf ("RR") != -1 /*&& gsmData.isRinging*/)//no carrier
	{
		if (isDebug)Serial.println ("nocarr");
		gsmData.isRinging = false;
		gsmData.isActivated = false;
		//COMDEBUG.println (gsmData.isFound);
		//COMDEBUG.println (gsmData.isResponse);
		if (gsmData.isFound && gsmData.isResponse)
		{
			gsmData.isFound = false;
			delay (100);
			SendStatus ();
		}
	}
	else if (gsm_string.indexOf ("RING") != -1 && gsmData.isRinging == false) gsmData.isRinging = true;


	if (gsmData.isRinging && !gsmData.isActivated )
	{
		if (telnmb.equals (String (u.s.telNums[0].number)))
		{
		//Serial.println ("ring2");
		gsmData.isFound = true;
		gsmData.isActivated = true;
		if (gsmOut.state == HIGH)gsmOut.state = LOW;
		else gsmOut.state = HIGH;
		ChangeOutput (gsmOut, (gsmOut.state == HIGH) ? ON_RNG : OFF_RNG);
		}


	}


	else if (telnmb.equals (String (u.s.telNums[0].number)))
	{
		//Serial.println ("numOK");
		gsmData.isFound = true;

	}
	ClearRxBuffer ();
	Timer1.start ();
}

void SendStatus ()
{
	for (int i = 0; i < sizeof (txBuffer); i++, txBuffer[i] = 0);
	//COMDEBUG.println ("sms");
	//vystupy
	strncpy (txBuffer, "alarm ", 6);
	strcat (txBuffer, (system_active == true)? "zap\n" : "vyp\n");
	strcat (txBuffer, "re1 ");
	if (outputs[0].state == HIGH)strcat (txBuffer, "zap\n");
	else strcat (txBuffer, "vyp\n");
	strcat (txBuffer, "re2 ");
	if (outputs[1].state == HIGH)strcat (txBuffer, "zap\n");
	else strcat (txBuffer, "vyp\n");
	strcat (txBuffer, "out3 ");
	if (outputs[2].state == HIGH)strcat (txBuffer, "zap\n");
	else strcat (txBuffer, "vyp\n");
	strcat (txBuffer, "out4 ");
	if (outputs[3].state == HIGH)strcat (txBuffer, "zap\n");
	else strcat (txBuffer, "vyp\n");
	strcat (txBuffer, "out5 ");
	if (outputs[4].state == HIGH)strcat (txBuffer, "zap\n");
	else strcat (txBuffer, "vyp\n");
	strcat (txBuffer, "out6 ");
	if (outputs[5].state == HIGH)strcat (txBuffer, "zap\n");
	else strcat (txBuffer, "vyp\n");

	//teploty
	if (!numberOfFDallasDevices)teploty_new[0] = teploty_new[1] = 0;
	char tepl[11];
	sprintf (tepl, "t1 = %2u,%1u\n", teploty_new[0] / 10, teploty_new[0] % 10);
	strcat (txBuffer, tepl);
	char tepl2[11];
	sprintf (tepl2, "t2 = %2u,%1u", teploty_new[1] / 10, teploty_new[1] % 10);
	strcat (txBuffer, tepl2);
	strcat (txBuffer, "\0");
	GSM.Sms (String(u.s.telNums[0].number), txBuffer);
	//Serial.println (txBuffer);
	casBlokovaniSms = 15;
}

void SaveEvent (int ev, char nmb_i_o)
{
	event.yy = ts.year - 2000;
	event.mnt = ts.mon;
	event.day = ts.mday;
	event.hr = ts.hour;
	event.min = ts.min;
	event.ss = ts.sec;
	event.evnt = ev | nmb_i_o;
	unsigned char ee_ptr_events;
	EEPROM.get (EE_EVENT_POINTER, ee_ptr_events);
	EEPROM.put (START_POINT + (ee_ptr_events * sizeof (Event)), event);
	if (++ee_ptr_events > 50)ee_ptr_events = 0;
	EEPROM.put (EE_EVENT_POINTER, ee_ptr_events);

}

void ClearEventList ()
{
	Event pomEvent;
	pomEvent.yy = 0xFF;
	pomEvent.mnt = 0xFF;
	pomEvent.day = 0xFF;
	pomEvent.hr = 0xFF;
	pomEvent.min = 0xFF;
	pomEvent.ss = 0xFF;
	pomEvent.evnt = 0xFFFF;

	for (int i = 0; i <= 50; i++)
	{
		EEPROM.put (START_POINT + i * sizeof (Event), pomEvent);
	}
	EEPROM.put (EE_EVENT_POINTER, 0x00);
}

void VypisPametUdalosti ()
{
	Timer1.stop ();
	delay (100);
	Serial.print ("event");
	signed char ptr;	//ukazatel v bufferu 50ti udalosti
	EEPROM.get (EE_EVENT_POINTER, ptr);//nacteni hodnoty z eeprom
									   //COMDEBUG.print (ptr,10);
									   //ptr;//snizeni o 1
	char pom_ptr = ptr;//pomocna pro dalsi pouziti

					   //COMDEBUG.println (ptr,10);
	char pom_buf[17];
	do
	{
		int ptr2 = START_POINT + (ptr * sizeof (Event));
		//COMDEBUG.println (ptr2,10);
		EEPROM.get (ptr2, event);
		//      
		//sprintf (pom_buf, ">%02x %02x %02x %02x %02x %02x %04X", event.yy, event.mnt, event.day, event.hr, event.min, event.ss, event.evnt);
		//COMDEBUG.println (pom_buf);
		if (event.yy != 0xFF)
		{
			sprintf (pom_buf, ">%u %u %u %u %02u %02u %04X", event.yy, event.mnt, event.day, event.hr, event.min, event.ss, event.evnt);
			Serial.print (pom_buf);
		}

		ptr--;
	} while (ptr >= 0);
	//COMDEBUG.println ("dalsi");
	ptr = 50;
	int ptr2;
	do
	{
		ptr2 = START_POINT + (ptr * sizeof (Event));
		EEPROM.get (ptr2, event);
		//COMDEBUG.println (ptr2, 10);
		if (event.yy != 0xFF)
		{
			//char pom_buf[17];
			sprintf (pom_buf, ">%u %u %u %u %02u %02u %04X", event.yy, event.mnt, event.day, event.hr, event.min, event.ss, event.evnt);
			Serial.print (pom_buf);
		}
		ptr--;
	} while (ptr > pom_ptr);
	//Serial.println (">");
	Timer1.start ();

}

void ChangeOutput (Out out, int ev)//cislo vystupu,cislo pinu,stav
{
	//outputs[out].state = state;
	if (isDebug){Serial.print (out.state ==  1 ? "on" : "off");
	Serial.println (out.nmb,10);}
	digitalWrite (out.nmb, out.state);//zapis stav na vystup
	SaveEvent (ev, 0);
	sendOutsFlg = true;
}

String TSToString (TS ts)
{
	char str[22];
	sprintf (str, "%u.%u.%u  %02u:%02u:%02u", ts.mday, ts.mon, ts.year, ts.hour, ts.min, ts.sec);
	return String (str);
}

void SendDateTime ()
{
	static char cnt = 0, cnt1 =0;
	char pomstr[10];
	//Serial.print ("cnt");
	//Serial.println (cnt,10);
	//cnt++;
	//
	if (isDebug)
	{
	if (++cnt > 10)
	{
		cnt = 0;
		GSM.Signal ();
	}
	}


	sendDateTimeFlg = false;
	if (numberOfFDallasDevices > 0)CtiTeploty ();
	sprintf (pomstr, "%2u,%1u %2u,%1u", teploty_new[0] / 10, teploty_new[0] % 10, teploty_new[1] / 10, teploty_new[1] % 10);
	if(isDebug)Serial.println ("dt>" + TSToString (ts) + '<' + pomstr + '<' + gsmSignal);
	delay (10);	
}

void TempControl ()
{
	static boolean blockTempOn, blockTempOff;
	if (teploty_new[0] > u.s.teplota && !blockTempOff)
	{
		blockTempOn = false;
		blockTempOff = true;
		digitalWrite (TERMOSTAT, LOW);
		SaveEvent (OFF_TEMP, 0);
		if (isDebug)Serial.println ("tempOn");
	}
	else if (teploty_new[0] < u.s.teplota - u.s.hystereze && !blockTempOn)
	{
		blockTempOn = true;
		blockTempOff = false;
		digitalWrite (TERMOSTAT, HIGH);
		SaveEvent (ON_TEMP, 0);
		if (isDebug)Serial.println ("tempOff");
	}


	if (teploty_new[0] > u.s.alarmT1)
	{
		alarmT1LoBlocked = false;
		if (alarmT1HiBlocked == false && u.s.aktivAlarmT1 == 1)
		{
			if (isDebug)Serial.println ("alarm T1 hi");
			GSM.Sms (u.s.telNums[0].number, u.s.aktivAlarmT1 == 1 ? "prekrocena teplota 1": "pokles teplota 1");
			SaveEvent (ALARM, 5);
			alarmT1HiBlocked = true;
		}

	}
	if (teploty_new[0] < u.s.alarmT1)
	{
		alarmT1HiBlocked = false;
		if (alarmT1LoBlocked == false && u.s.aktivAlarmT1 == 0)
		{
			if (isDebug)Serial.println ("alarm T1 lo");
			GSM.Sms (u.s.telNums[0].number, u.s.aktivAlarmT2 == 1 ? "prekrocena teplota 2" : "pokles teplota 2");
			SaveEvent (ALARM, 5);
			alarmT1LoBlocked = true;
		}

	}


	if (teploty_new[1] > u.s.alarmT2)
	{
		alarmT2LoBlocked = false;
		if (alarmT2HiBlocked == false && u.s.aktivAlarmT2 == 1)
		{
			if (isDebug)Serial.println ("alarm T2 hi");
			SaveEvent (ALARM, 6);
			alarmT2HiBlocked = true;
		}

	}
	if (teploty_new[1] < u.s.alarmT2)
	{
		alarmT2HiBlocked = false;
		if (alarmT2LoBlocked == false && u.s.aktivAlarmT2 == 0)
		{
			if (isDebug)Serial.println ("alarm T2 lo");
			SaveEvent (ALARM, 6);
			alarmT2LoBlocked = true;
		}

	}





}

void ClearRxBuffer ()
{
	for (int i = 0; i < sizeof (rxBuffer); i++)
	{
		rxBuffer[i] = 0;
	}
}

//ISR (WDT_vect) {
//
//	//  Anything you use in here needs to be declared with "volatile" keyword
//
//	//  Track the passage of time.
//	//setup ();
//
//}