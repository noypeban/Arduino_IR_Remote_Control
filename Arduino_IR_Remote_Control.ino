#include <SPI.h>		// for Ethernet.h
#include <Ethernet.h>	// for Ethernet

#define PIN_IR_OUT 2
#define READ_PIN 8
#define LOW_STATE 0
#define HIGH_STATE 1

unsigned char MACADDRESS[] = { 0x90, 0xA2, 0xDA, 0x00, 0xF9, 0x44 };
unsigned char IPADDRESS[] = { 192, 168, 11, 8 };
int irData[128];
char readstring[512];

EthernetServer server(80);

void setup()
{
  Serial.begin(9600);
  while(!Serial){
    ;
  }
  Serial.println("serial start!");

  pinMode(PIN_IR_OUT, OUTPUT);
  pinMode(READ_PIN,INPUT);

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
  int mode_get = 0;
  int mode_test = 0;
  EthernetClient client = server.available();
  if(client) {
    while(client.connected()){
      if(client.available()){
        char c = client.read();
        if(c=='\n'){
          if(strstr(readstring,"GET /?mode=receive")){
            Serial.println("mode get");
            mode_get=1;
          }else if(strstr(readstring,"GET /?mode=test")){
            Serial.println("mode test");
            mode_test=1;
          }
          strcpy(readstring,"");
          n=0;
        }else{
          readstring[n] = c;
          n++;
        }
      }else{
        if(mode_get){
          client.println("HTTP/1.1 200 OK");
          client.println("Access-Control-Allow-Origin: *");
          client.println("Content-Type: text/plain");
          client.println();
          client.println("Hit IR to sencer.");
          receiveIR(client);
          delay(10);
          client.stop();
        }else if(mode_test){
          client.println("HTTP/1.1 200 OK");
          client.println("Access-Control-Allow-Origin: *");
          client.println("Content-Type: text/plain");
          client.println();
          client.println("send IR test.");
          delay(1);
          client.stop();

          int testdata[71] = {905,440,63,46,66,158,66,47,63,46,66,47,63,47,66,47,64,46,63,160,64,160,63,160,64,160,64,159,64,160,63,158,67,157,66,46,63,47,67,46,64,159,64,159,64,46,64,159,64,46,66,157,66,158,65,158,63,50,64,46,64,160,64,46,66,157,66,3850,905,213,66};
          sendIr(testdata,71);

        }else if(n==0){
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println();
          client.println("can't get any data.");
          delay(1);
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

void waitLow() {
  while (digitalRead(READ_PIN)==LOW) {
    ;
  }
}
 
int waitHigh() {
  unsigned long start = micros();
  while (digitalRead(READ_PIN)==HIGH) {
    if (micros() - start > 5000000) {
      return 1;
    }
  }
  return 0;
}
 
void receiveIR( EthernetClient& client ){
  unsigned long now;
  unsigned long lastStateChangedMicros = 0;
  int state = HIGH_STATE;
  unsigned int cnt = 0;
  while(1){
    if (++cnt>=30000 && lastStateChangedMicros){
      return;
    }
    if (state == LOW_STATE) {
      waitLow();
    } else {
      int ret = waitHigh();
      if (ret == 1) {
        client.print("\n");
        return;
      }
    }

    now = micros();
    client.print((now - lastStateChangedMicros) / 10, DEC);
    client.print(",");
    lastStateChangedMicros = now;
    if (state == HIGH_STATE) {
      state = LOW_STATE;
    } else {
      state = HIGH_STATE;
    }
  }
}
