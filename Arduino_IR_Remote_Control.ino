#include <SPI.h>		// for Ethernet.h
#include <Ethernet.h>	// for Ethernet

unsigned char MACADDRESS[] = { 0x90, 0xA2, 0xDA, 0x00, 0xF9, 0x44 };
unsigned char IPADDRESS[] = { 192, 168, 11, 8 };
int irData[256];
char readstring[512];

const int PIN_IR_OUT = 2;
EthernetServer server(80);

void setup()
{
  Serial.begin(9600);
  while(!Serial){
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("serial start!");
  pinMode(PIN_IR_OUT, OUTPUT);
  Ethernet.begin(MACADDRESS, IPADDRESS);
  //Ethernet.begin(MACADDRESS);
  Serial.print("IP Address:         ");
  Serial.println(Ethernet.localIP());
  Serial.print("Subnet Mask:        ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("Gateway IP Address: "); 
  Serial.println(Ethernet.gatewayIP());
  Serial.print("DNS Server Address: "); 
  Serial.println(Ethernet.dnsServerIP());
  server.begin();
}

void loop()
{
  int n = 0;
  EthernetClient client = server.available();
  if(client) {
    while(client.connected()){
      if(client.available()){
        char c = client.read();
        if(c=='\n'){
          readstring[n]='\0';
          sprintf(readstring, "");
          n=0;
        }else{
          readstring[n] = c;
          n++;
        }
      }else{
        if(n==0){
          client.println("HTTP/1.1 200 OK");
          client.println("Access-Control-Allow-Origin: *");
          client.println("Content-Type: text/plain");
          client.println();
          client.println("can't get any data.");
          delay(10);
          client.stop();
        }else{
          client.println("HTTP/1.1 200 OK");
          client.println("Connection: Keep-Alive");
          client.println("Access-Control-Allow-Origin: *");
          client.println("Content-Type: text/plain");
          client.println();
          client.println("ok");
          delay(1);
          client.stop();
          Serial.println(readstring);
          parseCharAndSendIRData(readstring);
        }
      }
    }
  }
}

void parseCharAndSendIRData(char str[]){
  char *tp;
  int n = 0;
  tp = strtok(str,",");
  irData[n] = atoi(tp);
  n++;
  while(tp!=NULL){
    tp = strtok(NULL,",");
    if (tp!=NULL){
      irData[n] = atoi(tp);
      n++;
    }
  }
  sendIr(irData,n);
}

void sendIr(int irData[],int length){
  int now;
  int until;
  boolean onMode;
  Serial.println("Send start");
  for(int i=0;i<length;i++){
    now = micros();
    onMode = (i%2==0);
    until = now + irData[i]*10;
    while(until>now){
      if(onMode){
        PORTD = PORTD|B00000100;
      }
      delayMicroseconds(13);
      PORTD = PORTD&B11111011;
      delayMicroseconds(13);
      now = micros();
    }
  }
  PORTD = PORTD&B11111011;
  Serial.println("Send finish");
  Serial.println();
}
