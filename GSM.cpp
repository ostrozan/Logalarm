// 
// 
// 

//#include "GSM.h"
//#include "LogoBoard.h"
#define COMGSM Serial1
GSMClass::GSMClass ()
{
}

GSMClass::~GSMClass ()
{
}

bool GSMClass::Init()
{
	COMGSM.print("AT\r\n");
	//isOK = GsmAck("OK");
	return isOK;
}

bool GSMClass::Setup ()
{
	COMGSM.print ("AT+CMGF=1\r\n");
	delay (100);
	COMGSM.print ("AT+CNMI=1,2,0,0,0\r\n");
	return GsmAck ("OK");
}

bool GSMClass::Call(String number)
{
	COMGSM.print("ATD " + number + ";\r\n");
	//return GsmAck("OK");
}

bool GSMClass::Sms(String number, String message)
{
	COMGSM.print(F("AT+CMGF=1\r")); //set sms to text mode  
	rx_buffer = readSerial();
	COMGSM.print("AT+CMGS=\""+number+ "\"\r");  // command to send sms
	delay (100);
	rx_buffer = readSerial();
	COMGSM.print(message+"\r");
	delay(100);
	COMGSM.print((char)26);//ctrl Z
	//Serial.println("sms " + rx_buffer);//test
	//expect CMGS:xxx   , where xxx is a number,for the sending sms.
	//return GsmAck("CMGS");	
}

bool GSMClass::Sms (String number, char* message)
{
	COMGSM.print (F ("AT+CMGF=1\r")); //set sms to text mode  
	rx_buffer = readSerial ();
	Serial.println (rx_buffer);
	COMGSM.print ("AT+CMGS=\"" + number + "\"\r");  // command to send sms
	delay (100);
	rx_buffer = readSerial ();
	COMGSM.print (message);
	COMGSM.print ("\r");
	delay (100);
	COMGSM.print ((char)26);//ctrl Z
							//Serial.println("sms " + rx_buffer);//test
							//expect CMGS:xxx   , where xxx is a number,for the sending sms.
	return GsmAck ("CMGS");
}
bool GSMClass::HangOut()
{
	COMGSM.print("ATH\r\n");
	//return GsmAck ("OK");
}

String GSMClass::Signal()
{
	COMGSM.print("AT+CSQ\r\n");
	//delay (150);
	//	int start = rx_buffer.indexOf ("SQ:", 5) + 4;
	//	int end = start+2;
	//	return rx_buffer.substring (start, end);
	
	return "";
	//else return "ERR";
}

String GSMClass::Operator()
{
	COMGSM.print("AT+COPS?\r\n");

	if (GsmAck ("OK"))
	{
		int start = rx_buffer.indexOf ("PS:", 5)+9;
		int end = rx_buffer.indexOf ('\"', start);
		return rx_buffer.substring (start, end);	
	}
	else return "ERR";
}

String GSMClass::readSerial()
{
	timeout = 0;
	while (!COMGSM.available() && timeout < 12000)
	{
		delay(10);
		timeout++;
	}
	Serial.println (timeout, 10);
	if (COMGSM.available()) 
	{
		return COMGSM.readString (); 
	}

	return " ";
}

bool GSMClass::GsmAck(String str)
{
	rx_buffer = readSerial();
	//Serial.println (rx_buffer);
	//Serial.println (str);
	//Serial.println (rx_buffer.indexOf(str,2));
	if (((rx_buffer.indexOf(str),5) != -1))return true;
	else return false;
}


// 
// 
// 


GSMClass GSM;

