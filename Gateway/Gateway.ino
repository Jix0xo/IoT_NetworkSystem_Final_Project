// 게이트웨이 코드
#include <SoftwareSerial.h>
#include "SNIPE.h"
#include "ThingSpeak.h"
#include <ESP8266WiFi.h>

#define ADDR 10          // 디바이스 주소
#define TXpin 13         // 송신 핀
#define RXpin 15         // 수신 핀
#define ATSerial Serial   // 소프트웨어 시리얼 객체

String lora_app_key = "11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff 00";

SoftwareSerial DebugSerial(RXpin, TXpin);  // 디버그용 소프트웨어 시리얼 객체
SNIPE SNIPE(ATSerial);                      // LoRa 통신을 위한 SNIPE 라이브러리 객체
WiFiClient client;                          // ThingSpeak에 데이터를 전송하기 위한 WiFi 클라이언트 객체
char buffer[32];                            // 데이터를 임시로 저장할 버퍼

int temp, humi, dust;                        // 센서에서 측정한 온도, 습도, 미세먼지 값

char ssid[] = "Me";                          // WiFi SSID
char pass[] = "jio12345";                    // WiFi 비밀번호

unsigned long myChannelNumber = 2354928;    // ThingSpeak 채널 번호
const char* myWriteAPIKey = "WE1XH234A6AHAMPR";  // ThingSpeak Write API 키

void setup() {
  ATSerial.begin(115200);
  while (ATSerial.read() >= 0) {}   // 시리얼 버퍼를 비워줍니다.
  while (!ATSerial);                 // 시리얼 통신이 준비될 때까지 대기합니다.
  DebugSerial.begin(115200);

  // SNIPE LoRa 초기화 및 설정
  if (!SNIPE.lora_init() || !SNIPE.lora_setAppKey(lora_app_key) || 
      !SNIPE.lora_setFreq(LORA_CH_4) || !SNIPE.lora_setSf(LORA_SF_7) || 
      !SNIPE.lora_setRxtout(5000)) {
    DebugSerial.println("SNIPE LoRa 초기화 실패!");
    while (1);  // 초기화에 실패하면 무한루프에 빠지고 프로그램 실행이 중지됩니다.
  }

  DebugSerial.println("SNIPE LoRa PingPong Test");

  // WiFi 설정 및 연결
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);

  DebugSerial.println("Attempting to connect to SSID : ");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void loop() {
  char addr[2];   // 디바이스 주소를 저장하는 문자열
  String msg;     // 송신할 메시지를 저장하는 문자열
  int addr_rec;   // 수신한 주소값을 저장하는 변수

  sprintf(addr, "%d", ADDR);  // ADDR 값을 문자열로 변환하여 addr에 저장
  msg = (String)addr;         // msg에 주소값을 저장

  // LoRa 모듈로부터 데이터 수신
  String receivedData = SNIPE.lora_recv();
  DebugSerial.println(receivedData);

  // 수신한 데이터를 버퍼에 복사하고 주소와 센서값을 추출
  memset(buffer, 0x0, sizeof(buffer));
  receivedData.toCharArray(buffer, sizeof(buffer));
  sscanf(buffer, "%d:%d:%d:%d", &addr_rec, &temp, &humi, &dust);

  // 수신한 데이터가 오류 메시지가 아닌 경우 처리
  if (receivedData != "AT_RX_TIMEOUT" && receivedData != "AT_RX_ERROR") {
    // ThingSpeak에 데이터 업로드
    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, humi);
    ThingSpeak.setField(3, dust);

    if (addr_rec == 10) {  // 주소가 10일 경우 온도, 습도, 미세먼지 값 처리
      DebugSerial.print("Addr: ");
      DebugSerial.println(addr_rec);
      DebugSerial.print("Temp: ");
      DebugSerial.println(temp);
      DebugSerial.print("Humi: ");
      DebugSerial.println(humi);
      DebugSerial.print("Dust: ");
      DebugSerial.println(dust);
      DebugSerial.println(SNIPE.lora_getRssi());
      DebugSerial.println(SNIPE.lora_getSnr());

      // ThingSpeak에 데이터 전송
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

      if (x == 200) {
        Serial.println("Channel update successful.");
      } else {
        Serial.println("Problem updating channel. HTTP error code" + String(x));
      }
    }

    if (addr_rec == 15) {  // 주소가 15일 경우 인체 감지 데이터 처리
      DebugSerial.print("Addr: ");
      DebugSerial.println(addr_rec);
      DebugSerial.print("Person: ");
      DebugSerial.println(temp);
      DebugSerial.println(SNIPE.lora_getRssi());
      DebugSerial.println(SNIPE.lora_getSnr());

      // ThingSpeak에 데이터 전송
      int x = ThingSpeak.writeFields(2364071, "2D6VHEY3GPSJMTGI");

      if (x == 200) {
        Serial.println("Channel update successful.");
      } else {
        Serial.println("Problem updating channel. HTTP error code" + String(x));
      }
    }
  } else {
    DebugSerial.println("recv fail");
    delay(500);
  }

  delay(10);
}
