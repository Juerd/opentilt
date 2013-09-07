#include <AcceleroMMA7361.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

const double limit = 2;  // centiG per millisecond?
#define RLED 3
#define GLED 5
#define BLED 6

#define RED 1
#define GREEN 2
#define BLUE 4
#define WHITE (RED | GREEN | BLUE)
#define YELLOW (RED | GREEN)
#define MAGENTA (RED | BLUE)
#define CYAN (GREEN | BLUE)

AcceleroMMA7361 accelero;

void led(int color) {
  digitalWrite(RLED, color & RED);
  digitalWrite(GLED, color & GREEN);
  digitalWrite(BLED, color & BLUE);
}

void setup() {
  Serial.begin(9600);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  
  pinMode(RLED, OUTPUT);
  pinMode(GLED, OUTPUT);
  pinMode(BLED, OUTPUT);

  led(RED); delay(100);
  led(GREEN); delay(100);
  led(BLUE); delay(100);
  led(YELLOW); delay(100);
  led(MAGENTA); delay(100);
  led(CYAN); delay(100);
  led(WHITE); delay(100);

  delay(200);
  accelero.begin(NULL, NULL, NULL, NULL, A6, A5, A7);
  //accelero.setSensitivity(HIGH);
  
  led(BLUE);
}

int oldx, oldy, oldz, oldt;
bool firstloop = true;

void loop() {
  int x = accelero.getXAccel();
  int y = accelero.getYAccel();
  int z = accelero.getZAccel();
  
  int t = millis();
  int dx = x - oldx, dy = y - oldy, dz = z - oldz, dt = t - oldt;
  oldx = x; oldy = y; oldz = z, oldt = t;
 
  if (firstloop) {
    firstloop = false;
    return;
  }
  int d2 = dx*dx + dy*dy + dz*dz;

  double shock = (double) d2/dt;
 
  if (shock > (limit*limit)) {
    // dood
    led(RED);
    delay(3000);
    wdt_enable(WDTO_30MS);
    while(1);
  }
 
  delay(10);
}
