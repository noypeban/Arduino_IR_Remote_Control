/*
remoto_controler.ino
created: 130729
*/
#include <SPI.h>		// for Ethernet.h
#include <Ethernet.h>	// for Ethernet
#include <string.h>

unsigned char MACADDRESS[] = { 0x90, 0xA2, 0xDA, 0x00, 0xF9, 0x44 };
unsigned char IPADDRESS[] = { 192, 168, 11, 8 };
int irData[256];

int PIN_IR_OUT = 2;
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
  //char str[] = "905,440,63,46,66,158,66,47,63,46,66,47,63,47,66,47,64,46,63,160,64,160,63,160,64,160,64,159,64,160,63,158,67,157,66,46,63,47,67,46,64,159,64,159,64,46,64,159,64,46,66,157,66,158,65,158,63,50,64,46,64,160,64,46,66,157,66,3850,905,213,66";
  //parseCharAndSendIRData(str);
  char readstring[512];
  int n = 0;

  EthernetClient client = server.available();
  if(client) {
    while(client.connected()){
      if(client.available()){
        char c = client.read();
        //Serial.print(c);
        if(c=='\n'){
          readstring[n]='\0';
          //Serial.println(readstring);
          sprintf(readstring, "");
          n=0;
        }else{
          readstring[n] = c;
          n++;
        }
      }else{
        if(n==0){
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("Access-Control-Allow-Origin: *");
          client.println("can't get any data.");
          delay(1);
          client.stop();
        }else{
        Serial.println(readstring);
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("Access-Control-Allow-Origin: *");
          client.println("<html><head></head><body>");
          client.println("posted data<br/>");
          client.println(readstring);
          client.println("</body></heml>");
          delay(1);
          client.stop();
        }
      }
    }
  }
}

void parseCharAndSendIRData(char str[]){
  char *tp;
  int n = 0;
  Serial.println( str );
  tp = strtok(str,",");
  Serial.println(tp);
  irData[n] = atoi(tp);
  n++;
  while(tp!=NULL){
    tp = strtok(NULL,",");
    if (tp!=NULL){
      Serial.println(tp);
      irData[n] = atoi(tp);
      n++;
    }
  }
  Serial.println( n );
  Serial.println( "ok" );
  sendIr(irData,n);
}

unsigned long now;
unsigned long lastStateChangedMicros;
unsigned long until;
boolean lastState;
void sendIr(int irData[],int length){
  boolean signal = true;
  boolean onMode;
  Serial.println("Send start");
  //  引数の配列には、赤外線センサの出力がオンの時間、オフの時間が交互に記されている
  //  赤外線LEDはセンサ出力がオフの時は終始オフでよいが、
  //  センサ出力がオンの時は、13マイクロ秒ごとにオンオフを切り替える必要がある
  for(int i=0;i<length;i++){
    now = micros();
    onMode = (i%2==0);
    //  センサ出力の次のオンオフタイミングを計算
    until = now + irData[i]*10;
    while(until>now){
      if(onMode){
        //  センサ出力がオンなので、13マイクロ秒赤外線LEDを点灯する
        PORTD = PORTD|B00000100;
      }
      delayMicroseconds(13);
      //  センサ出力がオンでもオフでも、13マイクロ秒の休憩時間は必ず必要
      PORTD = PORTD&B11111011;
      delayMicroseconds(13);
      //  次のセンサ出力処理に移るべき時かを毎回チェックする
      now = micros();
    }
  }
  // データの欠損で最後につきっぱなしにならないよう、オフにしておく
  PORTD = PORTD&B11111011;
  Serial.println("Send finish");
  Serial.println();
}
