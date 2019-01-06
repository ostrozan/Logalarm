
//#include "GSM.h"
#include "Logalarm.h"
#include "DateTime.h"
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire (ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors (&oneWire);
DateTime dateTime;
//GsmModule GSM;
void setup ()
{
	alarm_loops[0] = { SM1,del };
	alarm_loops[1] = { SM2,inst };
	alarm_loops[2] = { SM3,inst };
	alarm_loops[3] = { SM4,h24 };
	u.s.exit_delay = 5;
	u.s.entry_delay = 5;
	u.s.time_alarm = 10;
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
	u.s.teplota = 230;
	u.s.hysterze = 5;
}

void loop ()
{
	if (teploty_new[0] > u.s.teplota)digitalWrite (TERMOSTAT, HIGH);
	else if (teploty_new[0]  < u.s.teplota - u.s.hysterze)digitalWrite (TERMOSTAT, LOW);

	if (sendDateTimeFlg)
	{
		sendDateTimeFlg = false;
		dateTime.GetDateTime ();
		minutes = dateTime.GetMinutes ();
		Serial.println (dateTime.ToString ());
	}
	if (measure_temp_flag)
	{
		measure_temp_flag = false;
		CtiTeploty ();
	}
	TestActivity ();//kontroluj vstup pro aktivaci/deaktivaci

	if (is_active_entry_delay &&!is_alarm_activated)//je aktivovan prichodovy cas
	{
		if (EntryTimeout ())Alarm (entry_loop_active);
	}

	for (int i = 0; i < 4; i++)//projdi hlidaci smycky
	{
		if (digitalRead (alarm_loops[i].input))//rozepnuta smycka?
		{
			//Serial.print ("loop"); Serial.println (i, 10);
			if (system_active)//aktivni alarm?
			{
				switch (alarm_loops[i].type)
				{
				case no_use: break;//nic
				case inst:if (!is_alarm_activated) Alarm (i); break;//alarm hned
				case del: entry_loop_active = i; EntryTimeout (); break;//aktivuj prichodovy cas
				case h24: if (!is_alarm_activated)Alarm (i); break;//alarm hned
				}
			}
			else //neaktivni alarm
			{
				switch (alarm_loops[i].type)
				{
				case no_use: break;//nic
				case inst: if (exit_timer)if (!is_alarm_activated) Alarm (i); break;// casuje odchodovy cas inst uz hlida - alarm hned
				case del: break;//nic
				case h24:if (!is_alarm_activated) Alarm (i); break;//alarm hned
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
		if (++tmr > 1500)
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
			if (alarm_counter < 3)
			{
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

void TimerTick ()
{
	sendDateTimeFlg = true;
	for (int i = 0; i < 4; i++)
	{
		if (minutes = u.s.minutespans[i].startTime)digitalWrite (SPIN_HOD, HIGH);
		if (minutes = u.s.minutespans[i].stopTime)digitalWrite (SPIN_HOD, LOW);
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
			is_active_entry_delay = false;
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
		casProzvaneni--;

		if (casProzvaneni <= 1)
		{
			casProzvaneni = 0;
			//COMDEBUG.println ("ath");

			//GSM.HangOut ();
			inputs[currCallingInput].isCallingGsm = false;
			delay (100);
		}
		/*	else
		{*/
		//COMDEBUG.println (casProzvaneni, 10);
		//}
	}

	//if (casZpozdeniSms)
	//{
	//	//COMDEBUG.println (casZpozdeniSms, 10);
	//	//if (--casZpozdeniSms == 0)SendLocalSms (currCallingInput);//
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
	//}
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
	sprintf (pomstr, "%2u,%1u %2u,%1u", teploty_new[0] / 10, teploty_new[0] % 10, teploty_new[1] / 10, teploty_new[1] % 10);
	Serial.println (pomstr);
	//unsigned long cas = millis () - time;
	//COMDEBUG.println (cas, 10);
}
