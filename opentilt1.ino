#include <avr/wdt.h>
#include <avr/sleep.h>
#include <AcceleroMMA7361.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <TimedButton.h>
#include <Entropy.h>
#include "color.h"
#include "shiv.h"

//#define DEBUG

// Requires DEBUG:
//#define COMM_DEBUG

// XXX master should send its value to the slaves
const float   shock_dead    = 2;     // centiG per millisecond?

// To start the game
const float   shock_shake   = 10;
const float   shakes_start  = 5;
const int     time_shake    = 100;
const int     timeout_shake = 500;

const int      max_players = 32;    // 4 bytes of SRAM per player!

const int      long_press  = 2000;  // power off
const int      timeout     = 1000;
const int      time_start  = 4000;

const int      time_heartbeat_min =  400;
const int      time_heartbeat_max =  600;
const int      time_comm_timeout = 5 * time_heartbeat_max + 100;

const int      broadcast_repeat = 15;
const int      broadcast_delay  = 0;
const int      unicast_tries    = 15;
const int      unicast_delay    = 0;

const int      delay_main_loop = 5;

const Color    color_setup1 = green;
const Color    color_setup2 = white;
const Color    color_master = magenta;
const Color    color_error  = red;
const Color    color_hello  = lightgreen;
const Color    color_player = royalblue;
const Color    color_dead   = coral;
const Color    color_win    = lightgreen;
const Color    color_single = gold;  // single player mode

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
const uint8_t msg_heartbeat  = 0;
const uint8_t msg_start_game = 1;

// client to master
const uint8_t msg_hello  = 100;
const uint8_t msg_status = 101;

// master to client
const uint8_t msg_config = 200;
const uint8_t msg_you_win = 201;
const uint8_t msg_you_die = 202;

const float shock_dead2 = shock_dead * shock_dead;
const float shock_shake2  = shock_shake  * shock_shake;

AcceleroMMA7361 acc;
RF24            rf(pin_rf_ce, pin_rf_cs);
TimedButton     button(pin_button);

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
        uint32_t broadcast;
    } config;
    struct {
        uint32_t time;
    } start_game;
    struct {
        uint8_t id;
        uint8_t alive;
    } status;
};

struct Payload {
    uint8_t seq;
    uint8_t msg;
    Param param;
};

static int time_heartbeat = time_heartbeat_max;

void setup() {
    #ifdef DEBUG
        Serial.begin(115200);
        Serial.println(sizeof(Payload));
    #endif
    pinMode(pin_power, OUTPUT);
    pinMode(pin_led_r, OUTPUT);
    pinMode(pin_led_g, OUTPUT);
    pinMode(pin_led_b, OUTPUT);
    led(color_setup1);

    digitalWrite(pin_power, HIGH);

    Entropy.Initialize();

    acc.begin(0, 0, 0, 0, pin_acc_x, pin_acc_y, pin_acc_z);

    rf.begin();
    rf.setRetries(unicast_delay, unicast_tries);
    rf.setPayloadSize(sizeof(Payload));

    time_heartbeat = Entropy.random(time_heartbeat_min, time_heartbeat_max);
}


enum state_enum {
    COMM_ERROR,
    HELLO, GET_CONFIG,
    MASTER_SETUP, MASTER_INVITE, MASTER_GAME_SETUP,
    PRE_GAME, GAME_START, GAME, GAME_OVER
} state = HELLO;

int oldstate = -1;

static long int broadcast = 0x96969696L;

bool send(long int destination, uint8_t msg, union Param param) {
    int i;
    bool is_broadcast = (destination == broadcast);
    static Payload p;

    p.seq++;
    p.msg = msg;
    p.param = param;
    #ifdef COMM_DEBUG
        Serial.println("SENDING: ");
        Serial.print("to "); Serial.println(destination, HEX);
        Serial.print("seq "); Serial.println(p.seq);
        Serial.print("msg "); Serial.println(p.msg);
    #endif
    rf.openWritingPipe(destination);
    rf.stopListening();

    bool ok = rf.write(&p, sizeof(p), is_broadcast);
    if (is_broadcast) {
        for (i = 1; i < broadcast_repeat; i++) {
            delay(broadcast_delay);
            bool ok = rf.write(&p, sizeof(p), is_broadcast);
        }
    }

    if (!is_broadcast) rf.openReadingPipe(0, broadcast);
    rf.startListening();
    return ok;
}

static bool received = 0;
static Payload payload;
static Param param;  // for sending
static long int my_addr;
static unsigned int me;
static long int master = 0x69690000L;
static float shock2;
static bool am_master;
static unsigned long state_change;

bool master_check_shake() {
    static unsigned long last_shake = 0;
    static unsigned int shakes;
    if (shock2 > shock_shake2 && last_shake < millis() - time_shake) {
        last_shake = millis();

        if (++shakes >= shakes_start) {
            shakes = 0;

            state = MASTER_GAME_SETUP;
            return true;
        }
    }

    if (last_shake < millis() - timeout_shake) shakes = 0;
    return false;
}

bool master_loop() {
    static int num_players = 1;
    static unsigned long wait_until = 0;
    static unsigned long alive[ max_players ];
    static unsigned int addr[ max_players ];
    static unsigned long next_heartbeat = millis() + time_heartbeat;
    bool ok;
    int i;

    if (millis() > next_heartbeat) {
        send(broadcast, msg_heartbeat, param);
        next_heartbeat = millis() + time_heartbeat;
    }

    switch (state) {
        case MASTER_SETUP: {
            led(color_master);
            broadcast = 0x96960000L + Entropy.random(0xFFFF);
            rf.openReadingPipe(0, broadcast);
            rf.openReadingPipe(1, master);
            rf.startListening();

            #ifdef DEBUG
                Serial.println("I am master.");
                Serial.println(get_free_memory(), DEC);
            #endif

            state = MASTER_INVITE;

            break;
        }

        case MASTER_INVITE: {
            if (received && payload.msg == msg_hello) {
                delay(10);
                param.config.id    = num_players;
                param.config.color = color_player;
                param.config.broadcast = broadcast;
                send(
                    master + payload.param.hello.id,
                    msg_config,
                    param
                );
                led(color_hello);
                wait_until = millis() + 500;
                addr[ num_players ] = payload.param.hello.id;
                num_players++;
            }

            if (wait_until && millis() >= wait_until) {
                led(color_master);
            }

            if (master_check_shake()) {
                param.config.id = 0;
                param.config.color =
                    num_players == 1
                    ? color_single
                    : color_player;
                payload.msg = msg_config;
                payload.param = param;
                return true;
            }

            break;
        }
        case MASTER_GAME_SETUP: {

            param.start_game.time = time_start;
            send(
                broadcast,
                msg_start_game,
                param
            );

            for (i = 0; i < num_players; i++)
                alive[i] = millis() + time_start + time_heartbeat;

            payload.msg = msg_start_game;
            payload.param = param;
            state = PRE_GAME;
            return true;
        }
        case GAME:
        case GAME_OVER: {
            if (master_check_shake()) break;

            if (received && payload.msg == msg_status) {
                int id = payload.param.status.id;
                if (payload.param.status.alive) {
                    if (alive[id]) {
                        alive[ payload.param.status.id ] = millis();
                    } else {
                        // In case you hadn't noticed, you were already dead.
                        // (e.g. communication timeout)
                        #ifdef DEBUG
                            Serial.print(id);
                            Serial.println(" says they're alive. They're not.");
                        #endif
                        send(master + addr[ id ], msg_you_die, param);
                    }
                } else {
                    alive[ id ] = 0;
                }
            }

            if (num_players < 2) break;  // No winners in single player :)

            int num_alive = 0;
            unsigned long survivor;
            for (i = 0; i < num_players; i++) {
                if (i ? (alive[i] > millis() - time_comm_timeout) : alive[i]) {
                    num_alive++;
                    survivor = i;
                } else if (alive[i]) {
                    #ifdef DEBUG
                        Serial.print(i, DEC);
                        Serial.println(" timed out.");
                        Serial.println(alive[i]);
                        Serial.println(millis());
                    #endif
                    alive[i] = 0;
                }
            }
            if (num_alive == 1) {
                if (survivor == 0) {  // that's me!
                    payload.msg = msg_you_win;
                    return true;
                } else {
                    send( master + addr[survivor], msg_you_win, param );
                }
            }


        }
    }

    return false;
}

bool client_loop() {
    static Color color;
    static unsigned long wait_until;
    static bool have_setup = false;
    static bool alive;
    static unsigned long heartbeat_received;
    static unsigned long next_heartbeat;
    int ok;

    if (received) {
        switch (payload.msg) {
            case msg_config: {
                me = payload.param.config.id;
                color = payload.param.config.color;
                if (!am_master) {
                    broadcast = payload.param.config.broadcast;
                    rf.openReadingPipe(0, broadcast);
                    rf.startListening();
                }

                #ifdef DEBUG
                    Serial.print("I am player ");
                    Serial.println(me);
                    Serial.println(get_free_memory());
                #endif

                have_setup = true;
                heartbeat_received = millis();
                break;
            }
            case msg_heartbeat: {
                heartbeat_received = millis();
                break;
            }
            case msg_start_game: {
                if (state == GAME_START || !have_setup) break;
                state = GAME_START;
                alive = true;
                wait_until = millis() + payload.param.start_game.time;
                break;
            }
        }
    }

    if (!am_master && state >= PRE_GAME) {
        if (millis() > heartbeat_received + time_comm_timeout)
            state = COMM_ERROR;

        if (millis() > next_heartbeat && state >= GAME) {
            next_heartbeat = millis() + time_heartbeat;
            param.status.id    = me;
            param.status.alive = alive;
            send(master, msg_status, param);
        }
    }

    switch (state) {
        case COMM_ERROR: {
            digitalWrite(pin_power, LOW);
            led(millis() % 200 < 100 ? off : color_error);

            // VVV doesn't work...
            if (received && payload.msg == msg_start_game) {
                state = GAME_START;
                wait_until = millis() + payload.param.start_game.time;
            }
            break;
        }
        case HELLO: {
            me = Entropy.random(0xFFFF);
            my_addr = master + me;


            wait_until = Entropy.random(1500);  // because random is slow

            led(color_setup2);
            delay(wait_until);

            rf.openReadingPipe(0, broadcast);  // needed, but why?
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
            if (have_setup) state = PRE_GAME;

            break;
        }

        case PRE_GAME: {
            led(((millis() - state_change) % 1000 < 100) ? off : color);
            break;
        }

        case GAME_START: {
            led(((millis() - state_change) % 200 < 100) ? off : color);

            if (millis() > wait_until) {
                alive = true;
                state = GAME;
            }
            break;
        }

        case GAME: {
            if (received && payload.msg == msg_you_win) {
                alive = true;
                state = GAME_OVER;
                break;
            } else if (received && payload.msg == msg_you_die) {
                alive = false;
                state = GAME_OVER;
                break;
            }

            if (shock2 > shock_dead2) {
                alive = false;
                state = GAME_OVER;

                param.status.id    = me;
                param.status.alive = alive;

                // XXX ugly.
                if (am_master) {
                    payload.msg = msg_status;
                    payload.param = param;
                    return true;
                } else {
                    send(master, msg_status, param);
                }
            } else { // if (millis() > wait_until) {
                led(color);
            }
            break;
        }

        case GAME_OVER: {
            led(
                am_master && (millis() % 5000 < 500)
                ? color_master
                : (
                    alive
                    ? ( (millis() - state_change) % 300 < 100 ? off : color_win)
                    : color_dead
                )
            );
            if (received && payload.msg == msg_start_game) {
                state = GAME_START;
                wait_until = millis() + payload.param.start_game.time;
            }
            break;
        }

    }

    return false;
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

    if (state != oldstate) {
        state_change = millis();
        oldstate = state;
    }
    
    int dx = x - oldx, dy = y - oldy, dz = z - oldz, dt = t - oldt;
    oldx = x; oldy = y; oldz = z; oldt = t;

    if (firstloop) {
        firstloop = false;
        return;
    }

    shock2 = (float) (dx*dx + dy*dy + dz*dz)/dt;

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
        #ifdef COMM_DEBUG
            Serial.println("RECEIVED: ");
            Serial.print("seq "); Serial.println(payload.seq);
            Serial.print("msg "); Serial.println(payload.msg);
        #endif
        received = 1;
    }

    if (!received) {
        received = client_loop();
    } else if (payload.msg < 100 || payload.msg >= 200) {
        // don't overwrite 'received' if mesage is for master.
        received = client_loop();
    }

    if (am_master) received = master_loop();
    else received = 0;

    delay(delay_main_loop);
}

// vim: ft=cpp
