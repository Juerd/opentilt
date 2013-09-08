#include <avr/wdt.h>
#include <avr/sleep.h>
#include <AcceleroMMA7361.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <TimedButton.h>
#include <Entropy.h>
#include "color.h"

const double   max_shock   = 2;     // centiG per millisecond?
const double   go_shock    = 20;    // to start the game
// XXX master should send its value to the slaves
const int      max_players = 32;    // 4 bytes of SRAM per player!

const int      long_press  = 2000;  // power off
const int      timeout     = 1000;
const int      time_start  = 1000;

const Color    color_setup  = white;
const Color    color_master = magenta;
const Color    color_error  = red;
const Color    color_hello  = lightgreen;
const Color    color_player = royalblue;

const int pin_button = 2;   // interrupt
const int pin_led_r  = 3;   // pwm
const int pin_led_g  = 5;   // pwm
const int pin_led_b  = 6;   // pwm
const int pin_power  = 7;   // powers 3.3V LDO for mma7361 and nrf24l01+
const int pin_rf_ce  = 8;
const int pin_rf_cs  = 10;  // mosi = 11, miso = 12, clk = 13
const int pin_acc_x  = A6;  // non-nano doesn't have A6+A7
const int pin_acc_y  = A7;
const int pin_acc_z  = A5;

// broadcast (master to clients)
const uint8_t msg_start_game = 1;

// client to master
const uint8_t msg_hello  = 100;

// master to client
const uint8_t msg_config = 200;

const double max_shock2 = max_shock * max_shock;
const double go_shock2  = go_shock  * go_shock;

AcceleroMMA7361 acc;
RF24            rf(pin_rf_ce, pin_rf_cs);
TimedButton     button(pin_button);

//const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void led(Color color) {
    analogWrite(pin_led_r, color.r);
    analogWrite(pin_led_g, color.g);
    analogWrite(pin_led_b, color.b);
}

void pin2_isr() {
    sleep_disable();
    detachInterrupt(0);
}

void power_down() {
    AGAIN:
    wdt_disable();
    sleep_enable();
    attachInterrupt(0, pin2_isr, LOW);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    ADCSRA |= (0<<ADEN);  // disable ADC
    cli(); sleep_bod_disable(); sei();
    sleep_cpu();
    sleep_disable();
    ADCSRA |= (1<<ADEN);  // enable ADC
    delay(5);  // workaround: sometimes millis() is 0...
    unsigned long min_time = millis() + long_press;
    delay(10);  // for more reliable readings
    while (digitalRead(pin_button) == LOW && millis() < min_time);
    if (millis() < min_time) goto AGAIN;
    reset();
}

void reset() {
    wdt_enable(WDTO_30MS);
    while (42);
}

extern int __bss_end;
extern void *__brkval;

int get_free_memory() {
    int free_memory;
    if((int)__brkval == 0)
         free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else free_memory = ((int)&free_memory) - ((int)__brkval);
    return free_memory;
}

union Param {
    struct {
        uint16_t id;
    } hello;
    struct {
        uint8_t  id;
        Color    color;
    } config;
    struct {
        uint32_t time;
    } start_game;
};

struct Payload {
    uint8_t seq;
    uint8_t msg;
    Param param;
};

void setup() {
    Serial.begin(115200);
    pinMode(pin_power, OUTPUT);
    pinMode(pin_led_r, OUTPUT);
    pinMode(pin_led_g, OUTPUT);
    pinMode(pin_led_b, OUTPUT);
    led(color_setup);

    digitalWrite(pin_power, HIGH);

    Entropy.Initialize();

    acc.begin(NULL, NULL, NULL, NULL, pin_acc_x, pin_acc_y, pin_acc_z);

    rf.begin();
    rf.setRetries(10, 10);
    Serial.println(sizeof(Payload));
    rf.setPayloadSize(sizeof(Payload));
}


enum state_enum {
    HELLO,
    GET_CONFIG,
    MASTER_SETUP, MASTER_INVITE, MASTER_GAME_SETUP,
    PRE_GAME, GAME_SETUP, GAME
} state = HELLO;

bool send(long int destination, uint8_t msg, union Param param) {
    static Payload p;
    p.seq++;
    p.msg = msg;
    p.param = param;
    Serial.println("SENDING: ");
    Serial.print("to "); Serial.println(destination, HEX);
    Serial.print("seq "); Serial.println(p.seq);
    Serial.print("msg "); Serial.println(p.msg);
    rf.openWritingPipe(destination);
    rf.stopListening();
    bool ok = rf.write(&p, sizeof(p));
    rf.startListening();
    return ok;
}

static bool received = 0;
static Payload payload;
static Param param;  // for sending
static long int my_addr;
static unsigned int me;
static long int master = 0xFF000000L;
static float shock2;
static bool am_master;

bool master_loop() {
    static int num_players = 1;
    static unsigned long wait_until = 0;
    static unsigned long alive[ max_players ];
    static unsigned int addr[ max_players ];
    bool ok;
    int i;

    switch (state) {
        case MASTER_SETUP: {
            led(color_master);
            rf.openReadingPipe(1, master);
            rf.startListening();
            Serial.println("I am master.");
            state = MASTER_INVITE;
            Serial.println(get_free_memory(), DEC);
        }

        case MASTER_INVITE: {
            if (received && payload.msg == msg_hello) {
                delay(10);
                param.config.id     = num_players;
                param.config.color  = color_player;
                send(
                    master + payload.param.hello.id,
                    msg_config,
                    param
                );
                led(color_hello);
                wait_until = millis() + 500;
                alive[ num_players ] = millis();
                addr[ num_players ] = payload.param.hello.id;
                num_players++;
            }

            if (wait_until && millis() >= wait_until) {
                led(color_master);
            }

            if (shock2 > go_shock2) {
                state = MASTER_GAME_SETUP;
            }

            break;
        }
        case MASTER_GAME_SETUP: {
            param.start_game.time = time_start;
            for (i = 1; i < num_players; i++)  // i am 0
            send(
                master + addr[ i ],
                msg_start_game,
                param
            );

            state = MASTER_INVITE;// PRE_GAME;
            break;
        }
    }

    return false;
}

void client_loop() {
    static Color color;
    static unsigned long wait_until;
    int ok;

    switch (state) {
        case HELLO: {
            me = Entropy.random(0xFFFF);
            my_addr = master + me;

            led(color_error);
            // color will be overwritten immediately, if a master replies.

            rf.openReadingPipe(1, my_addr);
            rf.startListening();
            param.hello.id = me;
            ok = send(master, msg_hello, param);
            rf.startListening();
            if (!ok) {
                am_master = true;
                state = MASTER_SETUP;
                break;
            }
            state = GET_CONFIG;
            wait_until = millis() + timeout;
            break;
        }

        case GET_CONFIG: {
            if (millis() >= wait_until) {
                am_master = true;
                state = MASTER_SETUP;
            }

            if (received && payload.msg == msg_config) {
                me = payload.param.config.id;
                color = payload.param.config.color;
                Serial.print("I am player ");
                Serial.println(me);
                Serial.println(get_free_memory());
                led(color);
                state = PRE_GAME;
            }

            break;
        }

        case PRE_GAME: {
            led(((millis() % 1000) < 100) ? off : color);
            if (received && payload.msg == msg_start_game) {
                state = GAME_SETUP;
                wait_until = millis() + payload.param.start_game.time;
            }
            break;
        }

        case GAME_SETUP: {
            led(green);
            if (millis() > wait_until) {
                led(white);
            }
        }
    }
}

void loop() {

    int ok;
    static int oldx = 0, oldy = 0, oldz = 0;
    static unsigned long oldt;
    int x = acc.getXAccel();
    int y = acc.getYAccel();
    int z = acc.getZAccel();
    unsigned long t = millis();
    static bool firstloop = true;
    
    int dx = x - oldx, dy = y - oldy, dz = z - oldz, dt = t - oldt;
    oldx = x; oldy = y; oldz = z; oldt = t;

    if (firstloop) {
        firstloop = false;
        return;
    }

    shock2 = (double) (dx*dx + dy*dy + dz*dz)/dt;
    //Serial.println(shock2);

    if (button.down(long_press)) {
        led(off);
        while (button.down());
        digitalWrite(pin_power, LOW);
        delay(1000);  // XXX debounce
        power_down();
    }

    // If received is already 1, it's because we're master and sending a
    // message to ourselves :)
    if (!received && rf.available()) {
        ok = rf.read(&payload, sizeof(payload));
        Serial.println("RECEIVED: ");
        Serial.print("seq "); Serial.println(payload.seq);
        Serial.print("msg "); Serial.println(payload.msg);
        received = 1;
    }


    client_loop();
    if (am_master) received = master_loop();
    else received = 0;

    delay(5);
}


/*
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
}*/


// vim: ft=cpp
