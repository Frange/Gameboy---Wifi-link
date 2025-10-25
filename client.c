#include <gb/gb.h>
#include <gbdk/console.h>
#include <stdio.h>
#include <string.h>

#define MSG_DISCOVERY 0xD0
#define MSG_ACK       0xA0
#define MSG_BUTTON_A  0xA1
#define MSG_BUTTON_B  0xB2
#define MSG_BUTTON_UP 0xA3
#define MSG_BUTTON_DOWN 0xA4
#define MSG_BUTTON_LEFT 0xA5
#define MSG_BUTTON_RIGHT 0xA6
#define MSG_BUTTON_SELECT 0xA7
#define MSG_PING      0xF0
#define MSG_PONG      0xF1

uint8_t connected = 0;
uint8_t last_sent = 0x00;
uint8_t last_received = 0x00;
uint16_t ping_counter = 0;
uint16_t no_response_counter = 0;
uint8_t last_button_tx = 0;
uint8_t last_button_rx = 0;
uint8_t in_main_screen = 0;

#define LOG_SIZE 11
char log_buffer[LOG_SIZE][21];
uint8_t log_index = 0;

// ===== HELPERS =====
void set_cursor(uint8_t x, uint8_t y) { gotoxy(x, y); }

void clear_line(uint8_t y) {
    set_cursor(0, y);
    printf("                    ");
}

void clear_screen(void) {
    for (uint8_t y = 0; y < 18; y++) {
        clear_line(y);
    }
}

const char* get_button_name(uint8_t btn) {
    switch(btn) {
        case MSG_BUTTON_A: return "A";
        case MSG_BUTTON_B: return "B";
        case MSG_BUTTON_UP: return "UP";
        case MSG_BUTTON_DOWN: return "DOWN";
        case MSG_BUTTON_LEFT: return "LEFT";
        case MSG_BUTTON_RIGHT: return "RIGHT";
        case MSG_BUTTON_SELECT: return "SELECT";
        default: return "";
    }
}

// ===== PANTALLA DE INICIO =====
void show_title_screen(void) {
    clear_screen();
    
    set_cursor(0, 2);
    printf("####################");
    set_cursor(0, 3);
    printf("#                  #");
    set_cursor(0, 4);
    printf("#   GB LINK WIFI   #");
    set_cursor(0, 5);
    printf("#                  #");
    set_cursor(0, 6);
    printf("#      CLIENT      #");
    set_cursor(0, 7);
    printf("#                  #");
    set_cursor(0, 8);
    printf("####################");
    
    set_cursor(4, 11);
    printf("Press START");
    
    set_cursor(1, 15);
    printf("Created by Frange");
    set_cursor(8, 16);
    printf("2025");
    
    // Esperar a que no haya botones presionados
    while (joypad() != 0) {
        wait_vbl_done();
    }
    
    // Esperar START
    while (1) {
        wait_vbl_done();
        if (joypad() & J_START) {
            delay(300);
            break;
        }
    }
}

// ===== PANTALLA PRINCIPAL =====
void update_header(void) {
    set_cursor(0, 0);
    if (connected) {
        printf("CLIENT - CONNECTED    ");
    } else {
        printf("CLIENT NOT CONNECTED");
    }
    
    set_cursor(0, 1);
    if (connected) {
        printf("Peer: SERVER        ");
    } else {
        printf("Peer: NONE          ");
    }
    
    set_cursor(0, 2);
    if (last_sent == 0x00) {
        printf("TX: NO DATA         ");
    } else {
        printf("TX: 0x%02X           ", last_sent);
    }
    
    set_cursor(0, 3);
    if (last_button_tx == 0) {
        printf("   Button:          ");
    } else {
        const char* btn = get_button_name(last_button_tx);
        if (strlen(btn) > 0) {
            printf("   Button: %-8s ", btn);
        } else {
            printf("   Button:          ");
        }
    }
    
    set_cursor(0, 4);
    if (last_received == 0x00) {
        printf("RX: NO DATA         ");
    } else {
        printf("RX: 0x%02X           ", last_received);
    }
    
    set_cursor(0, 5);
    if (last_button_rx == 0) {
        printf("   Button:          ");
    } else {
        const char* btn = get_button_name(last_button_rx);
        if (strlen(btn) > 0) {
            printf("   Button: %-8s ", btn);
        } else {
            printf("   Button:          ");
        }
    }
    
    set_cursor(0, 6);
    printf("--------------------");
}

void init_screen(void) {
    clear_screen();
    
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
        set_cursor(0, 7 + i);
        
        if (log_buffer[idx][0] != '\0') {
            for (uint8_t c = 0; c < 20 && log_buffer[idx][c] != '\0'; c++) {
                printf("%c", log_buffer[idx][c]);
            }
            for (uint8_t c = strlen(log_buffer[idx]); c < 20; c++) {
                printf(" ");
            }
        } else {
            printf("                    ");
        }
    }
}

// ===== LINK CABLE =====
void init_link(void) {
    SB_REG = 0x00;
    SC_REG = 0x80;
}

uint8_t link_send(uint8_t data) {
    SB_REG = data;
    SC_REG = 0x81;
    
    uint16_t timeout = 0;
    while ((SC_REG & 0x80) && timeout < 30000) {
        timeout++;
    }
    
    if (timeout >= 30000) {
        return 0x00;
    }
    
    return SB_REG;
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
                return received;
            }
        }
    }
    return 0x00;
}

// ===== PROCESAR MENSAJES =====
void process_message(uint8_t msg) {
    last_received = msg;
    no_response_counter = 0;
    
    switch (msg) {
        case MSG_DISCOVERY:
            add_log("  RX DISCOVERY");
            connected = 1;
            delay(50);
            last_sent = MSG_ACK;
            link_send(MSG_ACK);
            add_log("TX ACK");
            break;
            
        case MSG_ACK:
            add_log("  RX ACK");
            connected = 1;
            break;
            
        case MSG_BUTTON_A:
            add_log("  RX Button A");
            last_button_rx = MSG_BUTTON_A;
            connected = 1;
            break;
            
        case MSG_BUTTON_B:
            add_log("  RX Button B");
            last_button_rx = MSG_BUTTON_B;
            connected = 1;
            break;
            
        case MSG_BUTTON_UP:
            add_log("  RX Button UP");
            last_button_rx = MSG_BUTTON_UP;
            connected = 1;
            break;
            
        case MSG_BUTTON_DOWN:
            add_log("  RX Button DOWN");
            last_button_rx = MSG_BUTTON_DOWN;
            connected = 1;
            break;
            
        case MSG_BUTTON_LEFT:
            add_log("  RX Button LEFT");
            last_button_rx = MSG_BUTTON_LEFT;
            connected = 1;
            break;
            
        case MSG_BUTTON_RIGHT:
            add_log("  RX Button RIGHT");
            last_button_rx = MSG_BUTTON_RIGHT;
            connected = 1;
            break;
            
        case MSG_BUTTON_SELECT:
            add_log("  RX Button SELECT");
            last_button_rx = MSG_BUTTON_SELECT;
            connected = 1;
            break;
            
        case MSG_PING:
            add_log("  RX PING");
            last_sent = MSG_PONG;
            link_send(MSG_PONG);
            add_log("TX PONG");
            connected = 1;
            break;
            
        case MSG_PONG:
            add_log("  RX PONG");
            connected = 1;
            break;
            
        default:
            add_log("  RX UNKNOWN");
            connected = 1;
            break;
    }
    
    update_header();
}

// ===== ENVIAR BOTÓN =====
void send_button(uint8_t button_code, const char* button_name) {
    char log_msg[21];
    sprintf(log_msg, "TX Button %s", button_name);
    add_log(log_msg);
    
    last_sent = button_code;
    last_button_tx = button_code;
    uint8_t response = link_send(button_code);
    update_header();
    
    if (response != 0x00 && response != 0xFF) {
        process_message(response);
    }
}

// ===== MAIN LOOP =====
void main_loop(void) {
    uint8_t response;
    uint8_t joy_prev = 0;
    uint8_t joy_cur = 0;
    uint16_t discovery_counter = 0;
    uint8_t button_pressed = 0;
    uint16_t start_lock = 0;
    
    init_screen();
    add_log("CLIENT INIT");
    add_log("Searching...");
    
    in_main_screen = 1;
    
    while (1) {
        wait_vbl_done();
        
        joy_prev = joy_cur;
        joy_cur = joypad();
        
        button_pressed = 0;
        
        // START: Volver a título (con lock de 2 segundos)
        if ((joy_cur & J_START) && !(joy_prev & J_START) && start_lock == 0) {
            connected = 0;
            last_sent = 0x00;
            last_received = 0x00;
            last_button_tx = 0;
            last_button_rx = 0;
            in_main_screen = 0;
            
            show_title_screen();
            
            // Reiniciar todo
            init_link();
            init_screen();
            add_log("RESTARTED");
            start_lock = 120; // Lock de 2 segundos (120 frames)
            continue;
        }
        
        // Decrementar lock
        if (start_lock > 0) {
            start_lock--;
        }
        
        // Botones
        if ((joy_cur & J_A) && !(joy_prev & J_A)) {
            send_button(MSG_BUTTON_A, "A");
            button_pressed = 1;
            delay(150);
        }
        
        if ((joy_cur & J_B) && !(joy_prev & J_B)) {
            send_button(MSG_BUTTON_B, "B");
            button_pressed = 1;
            delay(150);
        }
        
        if ((joy_cur & J_UP) && !(joy_prev & J_UP)) {
            send_button(MSG_BUTTON_UP, "UP");
            button_pressed = 1;
            delay(150);
        }
        
        if ((joy_cur & J_DOWN) && !(joy_prev & J_DOWN)) {
            send_button(MSG_BUTTON_DOWN, "DOWN");
            button_pressed = 1;
            delay(150);
        }
        
        if ((joy_cur & J_LEFT) && !(joy_prev & J_LEFT)) {
            send_button(MSG_BUTTON_LEFT, "LEFT");
            button_pressed = 1;
            delay(150);
        }
        
        if ((joy_cur & J_RIGHT) && !(joy_prev & J_RIGHT)) {
            send_button(MSG_BUTTON_RIGHT, "RIGHT");
            button_pressed = 1;
            delay(150);
        }
        
        if ((joy_cur & J_SELECT) && !(joy_prev & J_SELECT)) {
            send_button(MSG_BUTTON_SELECT, "SELECT");
            button_pressed = 1;
            delay(150);
        }
        
        // Polling pasivo
        if (!button_pressed) {
            response = link_poll();
            if (response != 0x00 && response != 0xFF) {
                process_message(response);
            }
        }
        
        // Auto-discovery
        if (!connected) {
            discovery_counter++;
            if (discovery_counter > 180) {
                add_log("TX DISCOVERY");
                last_sent = MSG_DISCOVERY;
                response = link_send(MSG_DISCOVERY);
                update_header();
                
                if (response != 0x00 && response != 0xFF) {
                    process_message(response);
                }
                
                discovery_counter = 0;
            }
        } else {
            discovery_counter = 0;
        }
        
        // Ping automático
        if (connected) {
            ping_counter++;
            if (ping_counter > 300) {
                add_log("TX PING auto");
                last_sent = MSG_PING;
                response = link_send(MSG_PING);
                update_header();
                
                if (response == 0x00 || response == 0xFF) {
                    no_response_counter++;
                    add_log("No response");
                    
                    if (no_response_counter >= 3) {
                        connected = 0;
                        add_log("DISCONNECTED");
                        update_header();
                        no_response_counter = 0;
                    }
                } else {
                    process_message(response);
                }
                
                ping_counter = 0;
            }
        } else {
            ping_counter = 0;
            no_response_counter = 0;
        }
        
        // Actualizar header
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
    
    show_title_screen();
    init_link();
    main_loop();
}