#include <SPI.h>		// for Ethernet.h
#include <Ethernet.h>	// for Ethernet

#define PIN_IR_OUT 2
#define READ_PIN 8
#define LOW_STATE 0
#define HIGH_STATE 1

unsigned char MACADDRESS[] = { 0x90, 0xA2, 0xDA, 0x00, 0xF9, 0x44 };
unsigned char IPADDRESS[] = { 192, 168, 11, 8 };
unsigned int irData[128];
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

          unsigned int testdata[75] = {902,440,63,53,59,157,66,53,57,53,59,54,56,54,59,54,56,54,57,160,63,161,62,161,64,160,63,160,63,160,63,158,65,158,66,53,57,54,59,54,56,161,64,160,63,53,57,160,64,53,59,158,66,158,66,158,63,53,59,53,57,160,63,53,60,157,66,3851,904,214,66,9567,905,216,66};
          sendIr(testdata,75);

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

void sendIr(unsigned int irData[],int length){
  unsigned long now;
  unsigned long lastStateChangedMicros;
  boolean lastState;
  unsigned long until;
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
        //digitalWrite(PIN_IR_OUT,HIGH);
        PORTD = PORTD|B00000100;
      }
      delayMicroseconds(13);
      //  センサ出力がオンでもオフでも、13マイクロ秒の休憩時間は必ず必要
      //digitalWrite(PIN_IR_OUT,LOW);
      PORTD = PORTD&B11111011;
      delayMicroseconds(13);
      //  次のセンサ出力処理に移るべき時かを毎回チェックする
      now = micros();
    }
  }
  // データの欠損で最後につきっぱなしにならないよう、オフにしておく
  //digitalWrite(PIN_IR_OUT,LOW);
  PORTD = PORTD&B11111011;
  Serial.println("Send finish");
  Serial.println();
}

void waitLow() {
  while((~PIND & B10000000)==B10000000){
    ;
  }
}
 
int waitHigh() {
  unsigned long start = micros();
  while((PIND & B10000000)==B10000000){
    if (micros() - start > 5000000) {
      return 1;
    }
  }
  return 0;
}
 
void receiveIR( EthernetClient& client ){
  unsigned long now;
  unsigned long lastStateChangedMicros;
  int state = HIGH_STATE;
  unsigned int cnt = 0;
  while(1){
    if (++cnt>=3000){
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
