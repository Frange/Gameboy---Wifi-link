#include <gb/gb.h>
#include <gbdk/console.h>
#include <stdio.h>
#include <string.h>

#define ROLE_CLIENT 0

#define MSG_DISCOVERY 0xD0
#define MSG_ACK       0xA0
#define MSG_BUTTON_A  0xA1
#define MSG_BUTTON_B  0xB2
#define MSG_PING      0xF0
#define MSG_PONG      0xF1

uint8_t connected = 0;
uint8_t last_sent = 0x00;
uint8_t last_received = 0x00;
uint16_t ping_counter = 0;

#define LOG_SIZE 12
char log_buffer[LOG_SIZE][21];
uint8_t log_index = 0;

// ===== HELPERS =====
void set_cursor(uint8_t x, uint8_t y) { gotoxy(x, y); }

void clear_line(uint8_t y) {
    set_cursor(0, y);
    printf("                    ");
}

const char* get_msg_name(uint8_t msg) {
    switch(msg) {
        case MSG_DISCOVERY: return "DISCOVERY";
        case MSG_ACK:       return "ACK";
        case MSG_BUTTON_A:  return "Button A";
        case MSG_BUTTON_B:  return "Button B";
        case MSG_PING:      return "PING";
        case MSG_PONG:      return "PONG";
        default:            return "";
    }
}

// ===== PANTALLA FIJA =====
void update_header(void) {
    // FILA 0: Rol y estado
    set_cursor(0, 0);
    printf("CLIENTE  ");
    if (connected) {
        printf("OK  ");
    } else {
        printf("OFF ");
    }
    
    // FILA 1: IP/ID del peer
    set_cursor(0, 1);
    if (connected) {
        printf("Peer: CONNECTED     ");
    } else {
        printf("Peer: -             ");
    }
    
    // FILA 2: TX
    set_cursor(0, 2);
    if (last_sent == 0x00) {
        printf("TX: NO DATA         ");
    } else {
        const char* name = get_msg_name(last_sent);
        if (strlen(name) > 0) {
            printf("TX: 0x%02X - %-8s", last_sent, name);
        } else {
            printf("TX: 0x%02X          ", last_sent);
        }
    }
    
    // FILA 3: RX
    set_cursor(0, 3);
    if (last_received == 0x00) {
        printf("RX: NO DATA         ");
    } else {
        const char* name = get_msg_name(last_received);
        if (strlen(name) > 0) {
            printf("RX: 0x%02X - %-8s", last_received, name);
        } else {
            printf("RX: 0x%02X          ", last_received);
        }
    }
    
    // FILA 4: Separador
    set_cursor(0, 4);
    printf("--------------------");
}

void init_screen(void) {
    for (uint8_t y = 0; y < 18; y++) {
        clear_line(y);
    }
    
    for (uint8_t i = 0; i < LOG_SIZE; i++) {
        log_buffer[i][0] = '\0';
    }
    log_index = 0;
    
    update_header();
}

void add_log(const char* msg) {
    uint8_t len = strlen(msg);
    if (len > 20) len = 20;
    
    memcpy(log_buffer[log_index], msg, len);
    log_buffer[log_index][len] = '\0';
    log_index = (log_index + 1) % LOG_SIZE;
    
    for (uint8_t i = 0; i < LOG_SIZE; i++) {
        uint8_t idx = (log_index + i) % LOG_SIZE;
        set_cursor(0, 5 + i);
        
        if (log_buffer[idx][0] != '\0') {
            printf("%-20s", log_buffer[idx]);
        } else {
            printf("                    ");
        }
    }
}

// ===== LINK CABLE =====
void init_link(void) {
    SB_REG = 0x00;
    SC_REG = 0x00;  // Slave mode
}

uint8_t link_send(uint8_t data) {
    SB_REG = data;
    SC_REG = 0x81;
    
    last_sent = data;
    
    uint16_t timeout = 0;
    while ((SC_REG & 0x80) && timeout < 30000) {
        timeout++;
    }
    
    if (timeout >= 30000) {
        add_log("! TX TIMEOUT");
        return 0x00;
    }
    
    uint8_t received = SB_REG;
    
    if (received != 0x00 && received != 0xFF) {
        last_received = received;
    }
    
    return received;
}

uint8_t link_poll(void) {
    if (SC_REG & 0x80) {
        uint16_t timeout = 0;
        while ((SC_REG & 0x80) && timeout < 30000) {
            timeout++;
        }
        
        if (timeout < 30000) {
            uint8_t received = SB_REG;
            if (received != 0x00 && received != 0xFF) {
                last_received = received;
                return received;
            }
        }
    }
    return 0x00;
}

// ===== PROCESAR MENSAJES =====
void process_message(uint8_t msg) {
    char buf[21];
    
    switch (msg) {
        case MSG_DISCOVERY:
            add_log("< DISCOVERY");
            connected = 1;
            delay(50);
            link_send(MSG_ACK);
            add_log("> ACK");
            break;
            
        case MSG_ACK:
            add_log("< ACK");
            connected = 1;
            break;
            
        case MSG_BUTTON_A:
            add_log("< Button A");
            connected = 1;
            break;
            
        case MSG_BUTTON_B:
            add_log("< Button B");
            connected = 1;
            break;
            
        case MSG_PING:
            add_log("< PING");
            link_send(MSG_PONG);
            add_log("> PONG");
            connected = 1;
            break;
            
        case MSG_PONG:
            add_log("< PONG");
            connected = 1;
            break;
            
        default:
            if (msg != 0x00 && msg != 0xFF) {
                sprintf(buf, "< 0x%02X", msg);
                add_log(buf);
                connected = 1;
            }
            break;
    }
}

// ===== MAIN LOOP =====
void main_loop(void) {
    uint8_t response;
    uint8_t joy_prev = 0;
    uint8_t joy_cur = 0;
    
    init_screen();
    add_log("* CLIENTE");
    add_log("* Esperando srv...");
    
    while (1) {
        wait_vbl_done();
        
        joy_prev = joy_cur;
        joy_cur = joypad();
        
        // Botón A
        if ((joy_cur & J_A) && !(joy_prev & J_A)) {
            add_log("> Button A");
            response = link_send(MSG_BUTTON_A);
            if (response != 0x00 && response != 0xFF) {
                process_message(response);
            }
            update_header();
            delay(200);
        }
        
        // Botón B
        if ((joy_cur & J_B) && !(joy_prev & J_B)) {
            add_log("> Button B");
            response = link_send(MSG_BUTTON_B);
            if (response != 0x00 && response != 0xFF) {
                process_message(response);
            }
            update_header();
            delay(200);
        }
        
        // SELECT: Ping manual
        if ((joy_cur & J_SELECT) && !(joy_prev & J_SELECT)) {
            add_log("> PING");
            response = link_send(MSG_PING);
            if (response != 0x00 && response != 0xFF) {
                process_message(response);
            }
            update_header();
            delay(200);
        }
        
        // START: Reiniciar
        if ((joy_cur & J_START) && !(joy_prev & J_START)) {
            connected = 0;
            last_sent = 0x00;
            last_received = 0x00;
            init_link();
            init_screen();
            add_log("* REINICIADO");
            delay(300);
            continue;
        }
        
        // Polling pasivo
        response = link_poll();
        if (response != 0x00 && response != 0xFF) {
            process_message(response);
            update_header();
        }
        
        // Ping automático cada 10 segundos si conectado
        if (connected) {
            ping_counter++;
            if (ping_counter > 600) {
                add_log("> PING (auto)");
                response = link_send(MSG_PING);
                if (response == 0x00 || response == 0xFF) {
                    connected = 0;
                    add_log("! TIMEOUT");
                } else {
                    process_message(response);
                }
                update_header();
                ping_counter = 0;
            }
        } else {
            ping_counter = 0;
        }
        
        // Actualizar header cada segundo
        static uint8_t refresh_counter = 0;
        refresh_counter++;
        if (refresh_counter > 60) {
            update_header();
            refresh_counter = 0;
        }
    }
}

// ===== MAIN =====
void main(void) {
    DISPLAY_ON;
    SHOW_BKG;
    
    init_link();
    main_loop();
}