// 기능 A 코드 
// 온습도와 미세먼지 값 게이트웨이에 전송
// 온습도와 미세먼지 값에 따라 창문과 선풍기 제어
// 사람이 0명이면 창문을 닫고 모든 기능 중지
#include "DHT.h"
#include <SoftwareSerial.h>
#include "SNIPE.h"
#include <Servo.h>
#include <Stepper.h>

#define DHTPIN 2        // DHT 센서의 데이터 핀
#define DHTTYPE DHT11    // DHT 센서 타입
#define DUST_SENSOR_PIN A0  // 먼지 센서의 아날로그 핀

#define ADDR 10          // 디바이스 주소
#define TXpin 4          // 송신 핀
#define RXpin 5          // 수신 핀
#define ATSerial Serial   // 소프트웨어 시리얼 객체

Servo myServo;  // 서보 모터 객체 생성
const int stepsPerRevolution = 2048;  // 스텝 모터 회전 당 스텝 수
Stepper myStepper(stepsPerRevolution, 11, 9, 10, 8);  // 스텝 모터 객체 생성

DHT dht(DHTPIN, DHTTYPE);  // DHT 센서 객체 생성

float thresholdTemperature = 22.0;    // 온도 임계치 설정
float thresholdHumidity = 60.0;       // 습도 임계치 설정
float thresholdDustValue = 33.0;      // 미세먼지 임계치 설정

String lora_app_key = "11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff 00";

SoftwareSerial DebugSerial(RXpin, TXpin);  // 디버그용 소프트웨어 시리얼 객체
SNIPE SNIPE(ATSerial);                      // LoRa 통신을 위한 SNIPE 라이브러리 객체
char buffer[20];                            // 데이터를 임시로 저장할 버퍼
int addr;                                   // 주소값을 저장하는 변수
int person;                                 // 인체 감지 여부를 저장하는 변수

void setup() {
  ATSerial.begin(115200);
  myServo.attach(7);  // 서보 모터 핀 설정
  myServo.write(0);   // 서보 모터 초기 각도 설정
  myStepper.setSpeed(17);  // 스텝 모터 속도 설정

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

  DebugSerial.println("SNIPE LoRa DHT22 수신");

  dht.begin();  // DHT 센서 초기화
}

void loop() {
  char temp[8];   // 온도 문자열 버퍼
  char humi[8];   // 습도 문자열 버퍼
  char dust[8];   // 미세먼지 문자열 버퍼
  String ver = SNIPE.lora_recv();  // LoRa 모듈로부터 데이터 수신

  DebugSerial.println(ver);
  memset(buffer, 0x0, sizeof(buffer));
  ver.toCharArray(buffer, sizeof(buffer));
  sscanf(buffer, "%d:%d", &addr, &person);  // 수신한 데이터에서 주소와 인체 감지 여부 추출

  // DHT 센서에서 온도, 습도 및 미세먼지 값 읽기
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float dustValue = analogRead(DUST_SENSOR_PIN);
  float Cal_dust = (0.17 * (dustValue * (5.0 / 1024)) - 0.1) * 1000;

  // 센서 읽기 실패 시 처리
  if (isnan(humidity) || isnan(temperature) || isnan(dustValue)) {
    DebugSerial.println("센서에서 읽기 실패!");
    return;
  }

  // 문자열 버퍼에 센서 값 문자열로 변환 및 저장
  memset(buffer, 0x0, sizeof(buffer));
  dtostrf(temperature, 2, 0, temp);
  dtostrf(humidity, 2, 0, humi);
  dtostrf(Cal_dust, 2, 0, dust);

  // LoRa 전송을 위한 데이터 문자열 생성
  sprintf(buffer, "%d:%s:%s:%s", ADDR, temp, humi, dust);
  String send_value = String(buffer);

  DebugSerial.println("-----LoRa_send_value-----");
  DebugSerial.println(send_value);

  // 디버그 정보 출력
  DebugSerial.print("Temperature: ");
  DebugSerial.print(temperature);
  DebugSerial.print(" *C\t ");
  DebugSerial.print("Humidity: ");
  DebugSerial.print(humidity);
  DebugSerial.print(" %\t ");
  DebugSerial.print("Cal_dust: ");
  DebugSerial.println(Cal_dust);

  // 온도에 따라 서보 모터 제어
  if (person == 0) {
    myServo.write(0);  // 인체 감지가 없으면 서보 모터 0도로 회전
  } else {
    // 온도 및 미세먼지 값에 따라 서보 모터 및 스텝 모터 제어
    if (temperature >= thresholdTemperature && Cal_dust < thresholdDustValue) {
      myServo.write(90);  // 온도가 임계치 이상이면 서보를 90도로 회전
    } else if (temperature >= thresholdTemperature && Cal_dust >= thresholdDustValue) {
      myServo.write(0);  // 온도와 미세먼지가 모두 임계치 이상이면 서보 0도, 스텝 모터 회전
      myStepper.step(stepsPerRevolution);
    } else if (temperature < thresholdTemperature && Cal_dust < thresholdDustValue) {
      myServo.write(0);  // 온도가 임계치 미만이면 서보 0도로 회전
    } else if (Cal_dust >= thresholdDustValue) {
      myServo.write(0);  // 미세먼지가 임계치 이상이면 서보 0도로 회전
    }
    delay(100);  // 측정 주기 설정 (1초 대기)
  }

  // LoRa로 데이터 전송
  if (SNIPE.lora_send(send_value)) {
    DebugSerial.println("전송 성공");
  } else {
    DebugSerial.println("전송 실패");
    delay(500);
  }
  delay(100);  // 메인 루프 주기 설정
}
