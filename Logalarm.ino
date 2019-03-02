
#include <EEPROM.h>
#include "GsmModule.h"
#include "Logalarm.h"
#include "DateTime.h"
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire (ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors (&oneWire);
DateTime dateTime;
GsmModule GSM;

void setup ()
{
	COMGSM.begin (9600);
	while (!COMGSM);
	GSM.Init ();
	alarm_loops[0] = { SM1};
	alarm_loops[1] = { SM2};
	alarm_loops[2] = { SM3 };
	alarm_loops[3] = { SM4 };

	Spinacky.out_num = SPIN_HOD;
	delay (2000);
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
	dateTime.Init ();
	Timer1.initialize ();
	Timer1.attachInterrupt (TimerTick);
	////////test
	//u.s.teplota = 230;
	//u.s.hystereze = 5;
	//u.s.exit_delay = 5;
	//u.s.entry_delay = 35;
	//u.s.time_alarm = 10;


	//u.s.telNums[0].ring = true;
	//u.s.telNums[0].send_sms = true;
	//u.s.telNums[0].number[0] = '7';
	//u.s.telNums[0].number[1] = '7';
	//u.s.telNums[0].number[2] = '7';
	//u.s.telNums[0].number[3] = '6';
	//u.s.telNums[0].number[4] = '2';
	//u.s.telNums[0].number[5] = '2';
	//u.s.telNums[0].number[6] = '5';
	//u.s.telNums[0].number[7] = '3';
	//u.s.telNums[0].number[8] = '0';
	//u.s.telNums[0].number[9] = 0;

	//u.s.telNums[1].ring = false;
	//u.s.telNums[1].send_sms = false;
	//u.s.telNums[1].number[0] = '7';
	//u.s.telNums[1].number[1] = '7';
	//u.s.telNums[1].number[2] = '7';
	//u.s.telNums[1].number[3] = '6';
	//u.s.telNums[1].number[4] = '2';
	//u.s.telNums[1].number[5] = '2';
	//u.s.telNums[1].number[6] = '5';
	//u.s.telNums[1].number[7] = '3';
	//u.s.telNums[1].number[8] = '0';
	//u.s.telNums[1].number[9] = 0;

	//u.s.telNums[2].ring = false;
	//u.s.telNums[2].send_sms = false;
	//u.s.telNums[2].number[0] = '7';
	//u.s.telNums[2].number[1] = '7';
	//u.s.telNums[2].number[2] = '7';
	//u.s.telNums[2].number[3] = '6';
	//u.s.telNums[2].number[4] = '2';
	//u.s.telNums[2].number[5] = '2';
	//u.s.telNums[2].number[6] = '5';
	//u.s.telNums[2].number[7] = '3';
	//u.s.telNums[2].number[8] = '0';
	//u.s.telNums[2].number[9] = 0;

	EEPROM.get (0, u.data);


}

void loop ()
{
	if (Serial.available ())
	{
		GetSerialData ();
	}

	GsmAlarm ();
	TempControl ();

	//posli cas a datum
	if (sendDateTimeFlg)SendDateTime ();
	//komunikace prijem GSM
	if (gsmData.isEnabled && COMGSM.available ()) GsmReceive ();
	////komunikace prijem zpravy
	if (measure_temp_flag)
	{
		measure_temp_flag = false;
		CtiTeploty ();
	}
	TestActivity ();//kontroluj vstup pro aktivaci/deaktivaci

	if (is_active_entry_delay && !is_alarm_activated)//je aktivovan prichodovy cas
	{
		if (EntryTimeout ())
		{
			is_active_entry_delay = false;
			Alarm (entry_loop_active);
		}
	}

	for (int i = 0; i < 4; i++)//projdi hlidaci smycky
	{
		boolean is_loop_break = digitalRead (alarm_loops[i]);//optoclen spina zem
		if (LOOP_ACT_LO)is_loop_break = ~is_loop_break;//primy vstup bez optoclenu
		if (is_loop_break)//aktivovana smycka?
		{
			//Serial.print ("loop"); Serial.println (i, 10);
			if (system_active)//aktivni alarm?
			{
				switch (u.s.loop_types[i])
				{
				case no_use: break;//nic
				case inst:if (!is_alarm_activated) Alarm (i); break;//alarm hned
				case del: entry_loop_active = i; EntryTimeout (); break;//aktivuj prichodovy cas
				case h24: if (!is_alarm_activated)Alarm (i); break;//alarm hned
				case zona:  break;
				}
			}
			else //neaktivni alarm
			{
				switch (u.s.loop_types[i])
				{
				case no_use: break;//nic
				case inst: if (exit_timer)if (!is_alarm_activated) Alarm (i); break;// casuje odchodovy cas inst uz hlida - alarm hned
				case del: break;//nic
				case h24:if (!is_alarm_activated) Alarm (i); break;//alarm hned
				case zona:  break;
				}
			}
		}
	}
}

int tmr = 0;
boolean pushButton = false;
void TestActivity ()
{

	int state = digitalRead (LOCK);
	_delay_ms (1);

	if (!state && !pushButton)
	{
		if (++tmr > 1500)//stisk tlacitka 1,5 sec
		{
			tmr = 0;
			pushButton = true;
			Serial.println ("push");
			//while (digitalRead (LOCK));
		}

	}
	else tmr = 0;

	if (pushButton)
	{
		//pushButton = false;
		tmr = 0;
		if (!system_active)
		{
			system_active = ExitTimeout ();
			if (system_active)pushButton = false;
		}

		else if (system_active)
		{
			Serial.println ("disarm");
			digitalWrite (AKTIV, LOW);
			system_active = false;
			is_active_entry_delay = false;//shod priznak vstupniho zpozdeni
			entry_timer = 0;//vynuluj vstupni timer	
			is_alarm_activated = false;//shod priznak aktivovaneho alarmu
			digitalWrite (SIR, LOW);//vypni sirenu, pokud je zapla
			alarm_timer = 0;//vynuluj casovac alarmu
			alarm_counter = 0;//vynuluj pocitadlo alarmu
			pushButton = false;
		}
	}
}


boolean EntryTimeout ()
{
	if (!is_active_entry_delay)
	{
		Serial.println ("active entry");
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
		Serial.println ("arm");
		digitalWrite (AKTIV, HIGH);
		is_active_exit_delay = false;//shod priznak odpocitavani odchodu
		return true;//je aktivni odpocitavani odchodu a uplyunl nastaveny cas?

	}
	return false;
}

void Alarm (char loop_num)
{


	Serial.println ("prealarm");

	if (!is_alarm_activated)
	{
		for (int i = 0; i < 3; i++)
		{
			if (u.s.telNums[i].ring && !u.s.telNums[i].is_waiting_to_calling)u.s.telNums[i].is_waiting_to_calling = true;
			if (u.s.telNums[i].send_sms && !u.s.telNums[i].is_waiting_to_send_sms)u.s.telNums[i].is_waiting_to_send_sms = true;
		}


		if (alarm_counter < 3)
		{
			loop_in_alarm = loop_num;
			alarm_counter++;
			Serial.print ("alarm");
			Serial.print (alarm_counter, 10);
			Serial.println (loop_num, 10);
			is_alarm_activated = true;
			digitalWrite (SIR, HIGH);//zapni sirenu
			alarm_timer = u.s.time_alarm;//nastav casovac sireny
		}
	}



}

void GsmAlarm ()
{
	
	if (casProzvaneni || casZpozdeniSms)return;//pokud zrovna prozvani  - nepokracuj
	//pokud neceka zadna sms na odeslani
	if (!u.s.telNums[0].is_waiting_to_send_sms && !u.s.telNums[1].is_waiting_to_send_sms && !u.s.telNums[2].is_waiting_to_send_sms)
	{
		//Serial.println ("gsmalarm");
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
				GSM.Sms (u.s.telNums[i].number,buff);
				casZpozdeniSms = 5;//5 sec mezi jednotlivymi sms a pripadnym prozvanenim
				break;
			}
		}

	}
	//sms

}

void TimerTick ()
{
	static  boolean blockSetOn = false, blockSetOff = false;
	sendDateTimeFlg = true;
	for (int i = 0; i < 4; i++)
	{
		if (minutes = u.s.minutespans[i].startTime && blockSetOn == false)
		{
			blockSetOn = true;
			blockSetOff = false;
			digitalWrite (SPIN_HOD, HIGH);
		}
		if (minutes = u.s.minutespans[i].stopTime && blockSetOff == false)
		{
			blockSetOff = true;
			blockSetOn = false;
			digitalWrite (SPIN_HOD, HIGH);
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
		Serial.println (exit_timer, 10);
		if (--exit_timer == 0)
		{
			Serial.println ("odchod docasovan");
		}
	}

	if (entry_timer)
	{
		Serial.println (entry_timer, 10);
		if (--entry_timer == 0)
		{
		//	is_active_entry_delay = false;
			Serial.println ("prichod docasovan");

		}
	}

	if (alarm_timer)
	{
		if (--alarm_timer == 0)
		{
			is_alarm_activated = false;
			digitalWrite (SIR, LOW);//vypni sirenu
			Serial.println ("sirena off");
		}
	}

	if (casProzvaneni > 0)
	{
		Serial.print ("ring");
		Serial.println (casProzvaneni,10);
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
		Serial.print ("sms");
		Serial.println (casZpozdeniSms, 10);
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
		if (rxBuffer[rxBufferIndex-1] == 0x0d && recChar == 0x0a)
		{
			//COMDEBUG.println ("recmsg");
			rxBufferIndex = 0;
			recMsg = true;
			//Serial.println ("OKrec");
			break;
		}
		rxBufferIndex++;
	}

	if (rxBuffer[0] == 'W')
	{
		//memcpy (u.data, &rxBuffer[1], sizeof (u));
		
		char pom[18];
		memcpy (&u.s.entry_delay,&rxBuffer[1], 2);
		memcpy (pom, &u.s.entry_delay, 2);
		//Serial.write (pom, 2);
		memcpy ( &u.s.exit_delay, &rxBuffer[3], 2);
		//Serial.write (pom, 2);
		memcpy ( &u.s.time_alarm, &rxBuffer[5], 2);
		//Serial.write (pom, 2);
		memcpy ( &u.s.time_zone_activ, &rxBuffer[7], 2);
		//Serial.write (pom, 2);
		memcpy ( &u.s.time_zone_wait, &rxBuffer[9], 2);
		//Serial.write (pom, 2);
		memcpy (u.s.loop_types, &rxBuffer[11], 4);
		memcpy (&u.s.loop_activate, &rxBuffer[12], 1);

		//tel 1
		memcpy (u.s.telNums[0].number, &rxBuffer[16], 10);
		memcpy (&u.s.telNums[0].is_sms_control,&rxBuffer[26], 1);
		memcpy (&u.s.telNums[0].is_ring_control,&rxBuffer[27],1 );
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
		memcpy ( u.s.minutespans, &rxBuffer[64], 16);
		//Serial.write (pom, 16);
		//teploty
		memcpy ( &u.s.teplota, &rxBuffer[80], 2);
		//Serial.write (pom, 2);
		memcpy ( &u.s.hystereze, &rxBuffer[82], 1);
		//Serial.write (pom, 2);
		memcpy ( &u.s.alarmT1, &rxBuffer[83], 2);
		//Serial.write (pom, 2);
		memcpy ( &u.s.alarmT2, &rxBuffer[85], 2);
		//Serial.write (pom, 2);
		memcpy(&u.s.aktivAlarmT1, &rxBuffer[87],1 );
		memcpy (&u.s.aktivAlarmT2, &rxBuffer[88], 1);
		Serial.println ("ack");
		EEPROM.put (0, u.data);
	}

	if (rxBuffer[0] == 'R')
	{
		//for (int i = 0; i < sizeof (u.data); i++)
		//{
		//	Serial.write (u.data[i]);
		//}
		String pomstr;
		int pom;
		Serial.print ( u.s.entry_delay, 10);
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
		Serial.print (u.s.loop_types[0],10);
		Serial.print (u.s.loop_types[1], 10);
		Serial.print (u.s.loop_types[2], 10);
		Serial.print (u.s.loop_types[3], 10);
		Serial.print (' ');
		Serial.print (u.s.loop_activate, 10);
		Serial.print (' ');
		//tel 1
	    //pom = atoi (u.s.telNums[0].number);
		Serial.print (String(u.s.telNums[0].number));
		Serial.print (' ');
		Serial.print (u.s.telNums[0].is_sms_control,10);
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
		Serial.print (u.s.telNums[1].is_sms_control,10);
		Serial.print (' ');
		Serial.print (u.s.telNums[1].is_ring_control,10);
		Serial.print (' ');
		Serial.print (u.s.telNums[1].send_sms,10);
		Serial.print (' ');
		Serial.print (u.s.telNums[1].ring,10);
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
		Serial.print (u.s.aktivAlarmT1,10);
		Serial.print (':');
		Serial.print (u.s.aktivAlarmT2,10);
		Serial.print (' ');
		Serial.println ("data");
	}
}

void GsmReceive ()
{
	String gsm_string;
	signed int start, end;
	char telnmb[10];//pomocna pro tel cislo
	while (COMGSM.available ())
	{
		gsm_string = COMGSM.readString ();
	}
#ifdef DEBUG
	COMDEBUG.println (gsm_string);
#endif // DEBUG


	start = gsm_string.indexOf ("SQ:", 5);
	if (start != -1)
	{
		//COMDEBUG.println (">>");
		//
		//start = gsm_string.indexOf ("SQ:", 5) + 4;
		start += 4;
		end = start + 2;
		gsmSignal = gsm_string.substring (start, end);
		//COMDEBUG.println (gsmSignal);
	}
	else
	{
		start = gsm_string.indexOf ("+420");
		if (start != -1)
		{

			memcpy (telnmb, &gsm_string[start + 4], 10);
			//COMDEBUG.println (telnmb);

			//telnmb.toCharArray (pom, 10);
		}
	}


	if (gsm_string.indexOf ("CARR") != -1 /*&& gsmData.isRinging*/)//no carrier
	{
		//COMDEBUG.println ("nocarr");
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


	if (gsmData.isRinging && !gsmData.isActivated && strncmp (telnmb, gsmData.telNumber, 9) == 0)
	{
		//COMDEBUG.println ("ring");
		//COMDEBUG.println (gsmData.outNmb,2);
		gsmData.isFound = true;
		gsmData.isActivated = true;

		if (outputs[gsmData.outNmb].func == gsm)
		{
			if (outputs[gsmData.outNmb].state == HIGH)outputs[gsmData.outNmb].state = LOW;
			else outputs[gsmData.outNmb].state = HIGH;
			ChangeOutput (gsmData.outNmb, outputs[gsmData.outNmb].state, (outputs[gsmData.outNmb].state == HIGH) ? ON_RNG : OFF_RNG);
		}
	}


	else if (strncmp (telnmb, gsmData.telNumber, 9) == 0)
	{
		gsmData.isFound = true;
		//COMDEBUG.println ("found");
		if (gsm_string.indexOf ("T:") != -1)
		{
			//COMDEBUG.println ("+CMT");
			start = gsm_string.length () - 5;
			String command = gsm_string.substring (start, start + 3);
			char pom2[4];//pomocna pro prikaz
			command.toCharArray (pom2, 4);
			//COMDEBUG.println (pom2);
			char nmbx = pom2[0];
			if (pom2[0] != '?')
			{
				nmbx -= 0x30;
				nmbx -= 1;
			}
			if (outputs[nmbx].func == gsm)
			{
				ExecuteGsmCommad (pom2[0], pom2[2]);
				//if (gsmData.isResponse)SendStatus ();
			}
		}
	}

}

void ExecuteGsmCommad (char nmbOut, char outState)
{
	switch (nmbOut)
	{
	case '1': ChangeOutput (0, (outState == 'n') ? HIGH : LOW, (outState == 'n') ? ON_SMS : OFF_SMS); break;
	case '2': ChangeOutput (1, (outState == 'n') ? HIGH : LOW, (outState == 'n') ? ON_SMS : OFF_SMS); break;
	case '3': ChangeOutput (2, (outState == 'n') ? HIGH : LOW, (outState == 'n') ? ON_SMS : OFF_SMS); break;
	case '4': ChangeOutput (3, (outState == 'n') ? HIGH : LOW, (outState == 'n') ? ON_SMS : OFF_SMS); break;
	case '5': ChangeOutput (4, (outState == 'n') ? HIGH : LOW, (outState == 'n') ? ON_SMS : OFF_SMS); break;
	case '6': ChangeOutput (5, (outState == 'n') ? HIGH : LOW, (outState == 'n') ? ON_SMS : OFF_SMS); break;
	case '?': SendStatus (); SaveEvent (GSM_QEST, 0); break;
	}
	if (gsmData.isResponse && nmbOut != '?')SendStatus ();
}

void SendStatus ()
{
	for (int i = 0; i < sizeof (rxBuffer); i++, rxBuffer[i] = 0);
	//COMDEBUG.println ("sms");
	//vystupy
	strncpy (rxBuffer, "re1 ", 4);
	if (outputs[0].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	strcat (rxBuffer, "re2 ");
	if (outputs[1].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	strcat (rxBuffer, "out3 ");
	if (outputs[2].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	strcat (rxBuffer, "out4 ");
	if (outputs[3].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	strcat (rxBuffer, "out5 ");
	if (outputs[4].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	strcat (rxBuffer, "out6 ");
	if (outputs[5].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	//vstupy
	strcat (rxBuffer, "in1 ");
	if (inputs[0].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	strcat (rxBuffer, "in2 ");
	if (inputs[1].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	strcat (rxBuffer, "in3 ");
	if (inputs[2].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	strcat (rxBuffer, "in4 ");
	if (inputs[3].state == HIGH)strcat (rxBuffer, "zap\n");
	else strcat (rxBuffer, "vyp\n");
	//teploty
	if (!numberOfFDallasDevices)teploty_new[0] = teploty_new[1] = 0;

	char tepl[11];
	sprintf (tepl, "t1 = %2u,%1u\n", teploty_new[0] / 10, teploty_new[0] % 10);
	strcat (rxBuffer, tepl);
	char tepl2[11];
	sprintf (tepl2, "t2 = %2u,%1u", teploty_new[1] / 10, teploty_new[1] % 10);
	strcat (rxBuffer, tepl2);
	strcat (rxBuffer, "\0");
	GSM.Sms (gsmData.telNumber, rxBuffer);
	//COMDEBUG.println (rxBuffer);
	casBlokovaniSms = 15;
}

void SaveEvent (int ev, char nmb_i_o)
{
	event.yy = dateTime.dateTimeStr.year - 2000;
	event.mnt = dateTime.dateTimeStr.mon;
	event.day = dateTime.dateTimeStr.mday;
	event.hr = dateTime.dateTimeStr.hour;
	event.min = dateTime.dateTimeStr.min;
	event.ss = dateTime.dateTimeStr.sec;
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
	Serial.println (">");
	Timer1.start ();

}

void ChangeOutput (char out, char state, int ev)//cislo vystupu,cislo pinu,stav
{
	outputs[out].state = state;
	digitalWrite (outNmbs[out], state);//zapis stav na vystup
	SaveEvent (ev, out);
	sendOutsFlg = true;
}

void SendDateTime ()
{
	static char cnt = 0;
	char pomstr[10];
	//COMDEBUG.println ("tick");
	if (++cnt > 30)
	{
		cnt = 0;
		GSM.Signal ();
		//sendDateTimeFlg = false;
	}
	//else
	//{
	sendDateTimeFlg = false;
	if (numberOfFDallasDevices > 0)CtiTeploty ();

	sprintf (pomstr, "%2u,%1u %2u,%1u", teploty_new[0] / 10, teploty_new[0] % 10, teploty_new[1] / 10, teploty_new[1] % 10);
	dateTime.GetDateTime ();
	if (dateTime.dateTimeStr.sec == 0)minutes = dateTime.GetMinutes ();
	Serial.println ("dt>" + dateTime.ToString () + '<' + pomstr + '<' + gsmSignal);
	delay (10);
	//}
}

void TempControl ()
{
	if (teploty_new[0] > u.s.teplota)digitalWrite (TERMOSTAT, LOW);
	else if (teploty_new[0] < u.s.teplota - u.s.hystereze)digitalWrite (TERMOSTAT, HIGH);
	if(alarmT1blocked == false)
		{
			if (teploty_new[0] > u.s.alarmT1 /*&& u.s.aktivAlarmT1 == '1'*/)
			{
				Serial.println ("alarm T1 hi");
				alarmT1blocked = true;
			}
			else if (teploty_new[0] < u.s.alarmT1 && u.s.aktivAlarmT1 == '0')
			{
				Serial.println ("alarm T1 lo");
				alarmT1blocked = true;
			}
     }

	if (alarmT2blocked ==false)
	{
		if (teploty_new[1] > u.s.alarmT2 && u.s.aktivAlarmT2 == '1')
		{
			Serial.println ("alarm T2 hi");
			alarmT2blocked = true;
		}
		else if (teploty_new[1] < u.s.alarmT2 && u.s.aktivAlarmT2 == '0')
		{
			Serial.println ("alarm T2 lo");
			alarmT1blocked = true;
		}
	}





}