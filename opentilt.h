// indicate error by blinking x times.
const int error_master_gone = 2;  // 1 deliberately not used
const int error_no_radio    = 3;

// blink type
struct Blink {
    unsigned int on;
    unsigned int off;
};

// PROTOCOL

// broadcast (master to clients)
const uint8_t msg_heartbeat  = 0;
const uint8_t msg_start_game = 1;
const uint8_t msg_game_over  = 2;

// client to master
const uint8_t msg_hello  = 100;
const uint8_t msg_status = 101;

// master to client
const uint8_t msg_config = 200;
const uint8_t msg_game_over_you_win = 201;
const uint8_t msg_you_die = 202;

union Param {
    struct {
        uint16_t id;
    } hello;
    struct {
        uint8_t  id;
        Color    color;
        uint32_t prefix;
    } config;
    struct {
        uint32_t time;
    } start_game;
    struct {
        uint8_t id;
        uint8_t alive;
    } status;
    struct {
        uint32_t time;
    } heartbeat;
};

struct Payload {
    uint8_t seq;
    uint8_t msg;
    Param param;
};

#define zero(x) do { memset( &x, 0, sizeof(x)); } while (0)

// vim: ft=cpp
