//#define DEBUG

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
#include "debug.h"
#include "opentilt1.h"

// XXX master should send its value to the slaves
const float     shock_dead      =    2;  // centiG per millisecond?

// To start the game
const float     shock_shake     =   10;
const float     shakes_start    =    5;
const int       time_shake      =  100;
const int       timeout_shake   =  500;

const int       max_players     =   32;  // 4 bytes of SRAM per player!

const int       long_press      = 2000;  // power off
const int       timeout         = 1000;
const int       time_start      = 4000;

const int       broadcast_repeat =  15;
const int       broadcast_delay =    0;
const int       unicast_tries   =   15;
const int       unicast_delay   =    0;

const int       delay_main_loop =    5;

const int       time_heartbeat_master   = 1000;
const int       time_heartbeat_min      =  400;
const int       time_heartbeat_max      =  800;
const int       time_master_gone        = 4500;
const int       time_client_gone        = 6 * time_heartbeat_max + 100;
const int       time_error_delay        = 1000;

const Color     color_setup1 = white;
const Color     color_setup2 = cyan;
const Color     color_master = magenta;
const Color     color_error  = red;
const Blink     blink_error  = { 300, 200 };
const Color     color_hello  = lightgreen;
const Color     color_player = royalblue;
const Blink     blink_paired = { 800, 200 };
const Blink     blink_start  = { 200, 200 };
const Color     color_dead   = coral;
const Color     color_lose   = color_dead;
const Blink     blink_lose   = { 200, 2000 };
const Color     color_win    = lightgreen;
const Blink     blink_win    = { 300, 300 };
const Color     color_single = gold;  // single player mode

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

const float shock_dead2 = shock_dead * shock_dead;
const float shock_shake2  = shock_shake  * shock_shake;

static int time_heartbeat = time_heartbeat_max;
static unsigned long master    = 0x69690000L;
static unsigned long broadcast = 0x96969696L;
static bool received = 0;
static Payload payload;
static Param param;  // for sending
static long int my_addr;
static unsigned int me;
static float shock2;
static bool am_master = false;
static unsigned long prefix;
static signed long blinksync = 0;

AcceleroMMA7361 acc;
RF24            rf(pin_rf_ce, pin_rf_cs);
TimedButton     button(pin_button);

void led(Color color) {
    analogWrite(pin_led_r, color.r);
    analogWrite(pin_led_g, color.g);
    analogWrite(pin_led_b, color.b);
}

void led_error(int blinks) {
    unsigned int once     = blink_error.on + blink_error.off,
                 total    = blinks * once - blink_error.off,
                 interval = total + time_error_delay;

    unsigned long now = millis() + blinksync;

    led(   now % interval < total
        && now % interval % once < blink_error.on
        ? color_error
        : off
    );
}

void led_blink(Blink pattern, Color c1, Color c2 = off) {
    unsigned long now = millis() + blinksync;
    unsigned int interval = pattern.on + pattern.off;
    led(now % interval < pattern.on ? c1 : c2);
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


void setup() {
    #ifdef DEBUG
        Serial.begin(115200);
        debug_begin();
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

    // POST
    if (! rf.isPVariant()) {
        D("No nRF24L01+ detected.");
        digitalWrite(pin_power, LOW);
        for (;;) led_error(error_no_radio);
    }
}


enum state_enum {
    CLIENT_COMM_ERROR,
    CLIENT_HELLO, CLIENT_CONFIG, CLIENT_PAIRED,
    MASTER_SETUP, MASTER_INVITE, MASTER_GAME_SETUP,
    GAME_START, GAME, GAME_OVER
} state = CLIENT_HELLO;

int oldstate = -1;


bool send(unsigned long destination, uint8_t msg, union Param param) {
    int i;
    bool ok, is_broadcast = (destination == broadcast);
    static Payload p;

    p.seq++;
    p.msg = msg;
    p.param = param;
    D("SEND to=%08lx seq=%d msg=%d", destination, p.seq, p.msg);
    rf.openWritingPipe(destination);
    rf.stopListening();

    if (msg == msg_heartbeat) p.param.heartbeat.time = millis();
    ok = rf.write(&p, sizeof(p), is_broadcast);
    if (is_broadcast) {
        for (i = 1; i < broadcast_repeat; i++) {
            delay(broadcast_delay);
            if (msg == msg_heartbeat) p.param.heartbeat.time = millis();
            rf.write(&p, sizeof(p), is_broadcast);
        }
    }

    if (!is_broadcast) rf.openReadingPipe(0, broadcast);
    rf.startListening();
    return ok;
}


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
    static unsigned long next_heartbeat = millis() + time_heartbeat;
    int i;

    if (millis() > next_heartbeat) {
        send(broadcast, msg_heartbeat, param);
        next_heartbeat = millis() + time_heartbeat;
    }

    switch (state) {
        case CLIENT_COMM_ERROR:
        case CLIENT_HELLO:
        case CLIENT_CONFIG:
        case CLIENT_PAIRED:
            break;

        case MASTER_SETUP: {
            led(color_master);
            prefix = 0x96000000L + (Entropy.random(0xFFFF) << 8);
            broadcast = prefix + 0xff;
            time_heartbeat = time_heartbeat_master;

            rf.openReadingPipe(0, broadcast);
            rf.openReadingPipe(1, master);
            rf.startListening();

            D("I am master! prefix=%08lx free=%d", prefix, get_free_memory());

            state = MASTER_INVITE;

            break;
        }
        case MASTER_INVITE: {
            if (received && payload.msg == msg_hello) {
                delay(10);
                param.config.id    = num_players;
                param.config.color = color_player;
                param.config.prefix = prefix;
                send(
                    master + payload.param.hello.id,
                    msg_config,
                    param
                );
                led(color_hello);
                wait_until = millis() + 500;
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
            master = prefix + 0x00;
            rf.openReadingPipe(1, master);
            rf.startListening();

            param.start_game.time = time_start;
            send(
                broadcast,
                msg_start_game,
                param
            );

            for (i = 0; i < num_players; i++)
                alive[i] = millis() + time_start + time_heartbeat_max;

            payload.msg = msg_start_game;
            payload.param = param;
            state = CLIENT_PAIRED;
            return true;
        }
        case GAME:
        case GAME_START: {
            if (master_check_shake()) break;

            if (received && payload.msg == msg_status) {
                int id = payload.param.status.id;
                if (payload.param.status.alive) {
                    if (alive[id]) {
                        alive[ payload.param.status.id ] = millis();
                    } else {
                        // In case you hadn't noticed, you were already dead.
                        // (e.g. communication timeout)
                        D("%d says they're alive. They're not.", id);
                        send(prefix + id, msg_you_die, param);
                    }
                } else {
                    alive[ id ] = 0;
                }
            }

            if (num_players < 2) break;  // No winners in single player :)

            int num_alive = 0;
            unsigned long survivor;
            for (i = 0; i < num_players; i++) {
                if (i ? (alive[i] > millis() - time_client_gone) : alive[i]) {
                    num_alive++;
                    survivor = i;
                } else if (alive[i]) {
                    D("%d timed out: seen=%d uptime=%d", alive[i], millis());
                    alive[i] = 0;
                }
            }
            if (num_alive == 1) {
                if (survivor == 0) {  // that's me!
                    send(broadcast, msg_game_over, param);
                    payload.msg = msg_game_over_you_win;
                    return true;  // client_loop will set state = G_O_SETUP.
                } else {
                    send(prefix + survivor, msg_game_over_you_win, param);
                    send(broadcast, msg_game_over, param);
                    state = GAME_OVER;
                }
            }

            break;
        }
        case GAME_OVER: {
            master_check_shake();

            break;
        }
    }

    return false;
}

bool client_loop() {
    static Color my_color;
    static unsigned long wait_until;
    static bool have_config = false;
    static bool alive;
    static unsigned long heartbeat_received;
    static unsigned long next_heartbeat;
    int ok;

    if (received) {
        switch (payload.msg) {
            case msg_config: {
                if (have_config) break;

                me = payload.param.config.id;
                my_color = payload.param.config.color;
                if (!am_master) {
                    prefix = payload.param.config.prefix;
                    broadcast = prefix + 0xff;
                    rf.openReadingPipe(0, broadcast);
                    rf.openReadingPipe(1, prefix + me);
                    rf.startListening();
                }

                D("I am player %d. prefix=%08lx, free=%d",
                    me, prefix, get_free_memory());

                have_config = true;
                heartbeat_received = millis();
                break;
            }
            case msg_heartbeat: {
                heartbeat_received = millis();
                blinksync = payload.param.heartbeat.time - millis();
                D("My time: %ld, HB time: %ld, Blinksync: %ld",
                    millis(), payload.param.heartbeat.time, blinksync);
                break;
            }
            case msg_start_game: {
                master = prefix + 0x00;

                if (state == GAME_START || !have_config) break;
                state = GAME_START;
                alive = true;
                wait_until = millis() + payload.param.start_game.time;
                break;
            }
        }
    }

    if (!am_master && state >= CLIENT_PAIRED) {
        if (millis() > heartbeat_received + time_master_gone) {
            D("Master heartbeat timed out.");
            state = CLIENT_COMM_ERROR;
        }

        if (millis() > next_heartbeat && state >= GAME_START) {
            next_heartbeat = millis() + time_heartbeat;
            param.status.id    = me;
            param.status.alive = alive;
            send(master, msg_status, param);
        }
    }

    switch (state) {
        case MASTER_SETUP:
        case MASTER_INVITE:
        case MASTER_GAME_SETUP:
            break;

        case CLIENT_COMM_ERROR: {
            led_error(error_master_gone);

            if (received && payload.msg == msg_start_game) {
                state = GAME_START;
                wait_until = millis() + payload.param.start_game.time;
            }
            break;
        }
        case CLIENT_HELLO: {
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
            state = CLIENT_CONFIG;
            wait_until = millis() + timeout;
            break;
        }
        case CLIENT_CONFIG: {
            if (millis() >= wait_until) {
                am_master = true;
                state = MASTER_SETUP;
            }
            if (have_config) state = CLIENT_PAIRED;

            break;
        }
        case CLIENT_PAIRED: {
            led_blink(blink_paired, my_color);
            break;
        }
        case GAME_START: {
            led_blink(blink_start, my_color);

            if (millis() > wait_until) {
                alive = true;
                state = GAME;
            }
            break;
        }
        case GAME: {
            if (received && payload.msg == msg_game_over_you_win) {
                alive = true;
                state = GAME_OVER;
                break;
            } else if (received && payload.msg == msg_game_over) {
                alive = false;
                state = GAME_OVER;
                break;
            } else if (received && payload.msg == msg_you_die) {
                alive = false;
                break;
            }

            led(alive ? my_color : color_dead);
            if (alive && shock2 > shock_dead2) {
                alive = false;

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
            }
            break;
        }
        case GAME_OVER: {
            if (am_master && millis() % 5000 < 500) {
                led(color_master);
            } else {
                if (alive) led_blink(blink_win, color_win);
                else       led_blink(blink_lose, color_lose);
            }

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
    static int oldx = 0, oldy = 0, oldz = 0;
    static unsigned long oldt;
    int x = acc.getXAccel();
    int y = acc.getYAccel();
    int z = acc.getZAccel();
    unsigned long t = millis();
    static bool firstloop = true;
    static int previous_broadcast = -1;

    if (state != oldstate) {
        D("State changed from %d to %d.", oldstate, state);
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
    while (!received && rf.available()) {
        rf.read(&payload, sizeof(payload));
        received = 1;
        if (payload.msg < 100) {
            // broadcasts; skip duplicates
            if (payload.seq == previous_broadcast) received = 0;
            previous_broadcast = payload.seq;
        }
        if (received) D("RECEIVED: seq=%d msg=%d", payload.seq, payload.msg);
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
