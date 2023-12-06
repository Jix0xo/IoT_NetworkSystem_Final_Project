// 기능 B 코드
// 초음파 센서를 활용해 인원 수 체크 후 게이트웨이에 데이터 전송
// 사람 수가 0명이면 LED 전원 OFF

#include <SoftwareSerial.h>
#include "SNIPE.h"

#define trigPin1 13      // 초음파 센서 1번의 트리거 핀
#define trigPin2 7       // 초음파 센서 2번의 트리거 핀
#define echoPin1 12      // 초음파 센서 1번의 에코 핀
#define echoPin2 6       // 초음파 센서 2번의 에코 핀
#define ledPin 2         // LED 연결 핀

#define ADDR 15          // 장치 주소
#define TXpin 11         // 송신 핀
#define RXpin 10         // 수신 핀
#define ATSerial Serial   // 소프트웨어 시리얼 객체

int duration, cm1, cm2;  // 초음파 센서 측정 변수
int val1 = 0, val2 = 0, count = 0;  // 거리 및 카운트 변수

String lora_app_key = "11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff 00";  // LoRa 앱 키

SoftwareSerial DebugSerial(RXpin, TXpin);  // 디버그를 위한 소프트웨어 시리얼 객체
SNIPE SNIPE(ATSerial);                      // LoRa 통신을 위한 SNIPE 라이브러리 객체
char buffer[20];                            // 임시 버퍼
int addr;                                   // 주소 변수

void setup() {
  ATSerial.begin(115200);      // 소프트웨어 시리얼 초기화
  pinMode(trigPin1, OUTPUT);    // 초음파 센서 1번 트리거 핀 설정
  pinMode(echoPin1, INPUT);     // 초음파 센서 1번 에코 핀 설정
  pinMode(trigPin2, OUTPUT);    // 초음파 센서 2번 트리거 핀 설정
  pinMode(echoPin2, INPUT);     // 초음파 센서 2번 에코 핀 설정
  pinMode(ledPin, OUTPUT);      // LED 핀 설정

  while (ATSerial.read() >= 0) {}  // 시리얼 버퍼 초기화
  while (!ATSerial);                // 시리얼 통신이 준비될 때까지 대기

  DebugSerial.begin(115200);  // 디버그용 시리얼 초기화

  // SNIPE LoRa 초기화 및 설정
  if (!SNIPE.lora_init() || !SNIPE.lora_setAppKey(lora_app_key) ||
      !SNIPE.lora_setFreq(LORA_CH_4) || !SNIPE.lora_setSf(LORA_SF_7) ||
      !SNIPE.lora_setRxtout(5000)) {
    DebugSerial.println("SNIPE LoRa 초기화 실패!");
    while (1);  // 초기화 실패 시 무한 루프로 진입하여 프로그램 중지
  }

  DebugSerial.println("SNIPE LoRa DHT22 수신");
}

// 초음파 센서 거리 측정 함수
long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

// 초음파 센서 동작 함수
void driveSensor(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
}

void loop() {
  char person[8];  // 인체 감지 여부 문자열 버퍼
  char CM1[8];     // 센서 1 거리 문자열 버퍼
  char CM2[8];     // 센서 2 거리 문자열 버퍼

  // 각 센서의 거리 측정
  driveSensor(trigPin1, echoPin1);
  cm1 = microsecondsToCentimeters(duration);
  driveSensor(trigPin2, echoPin2);
  cm2 = microsecondsToCentimeters(duration);

  // 거리에 따른 인체 감지 로직
  if (cm1 < 5 && val2 == 0)
    val1 = 1;
  else if (cm1 < 5 && val2 == 1) {
    count -= 1;
    val2 = 0;
    delay(1000);
  }

  if (cm2 < 5 && val1 == 1) {
    count += 1;
    val1 = 0;
    delay(1000);
  } else if (cm2 < 5 && val1 == 0) {
    val2 = 1;
  }

  // 카운트에 따른 LED 제어
  if (count > 0)
    digitalWrite(ledPin, HIGH);
  else
    digitalWrite(ledPin, LOW);

  String send_value;  // LoRa 전송을 위한 문자열 버퍼
  DebugSerial.println(SNIPE.lora_getRssi());  // LoRa 수신 신호 강도 출력
  DebugSerial.println(SNIPE.lora_getSnr());   // LoRa 수신 신호 대잡비 출력

  if (isnan(count)) {
    DebugSerial.println("센서에서 읽기 실패!");
    return;
  }

  // 문자열 버퍼 초기화 및 값 저장
  memset(buffer, 0x0, sizeof(buffer));
  dtostrf(count, 2, 0, person);
  dtostrf(cm1, 2, 0, CM1);
  dtostrf(cm2, 2, 0, CM2);

  // LoRa 전송을 위한 데이터 문자열 생성
  sprintf(buffer, "%d:%s:%s:%s", ADDR, person, CM1, CM2);
  send_value = String(buffer);

  DebugSerial.println("-----LoRa_send_value-----");
  DebugSerial.println(send_value);

  DebugSerial.print("person: ");
  DebugSerial.println(person);
  DebugSerial.println(CM1);
  DebugSerial.println(CM2);

  // LoRa로 데이터 전송
  if (SNIPE.lora_send(send_value)) {
    DebugSerial.println("전송 성공");
  } else {
    DebugSerial.println("전송 실패");
    delay(500);
  }
  delay(100);
}
