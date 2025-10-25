#include <gb/gb.h>
#include <stdio.h>

void init_link(void) {
    SB_REG = 0x00;
    SC_REG = 0x80; // modo maestro
}

void send_link(UINT8 data) {
    SB_REG = data;
    SC_REG = 0x81;
    while (SC_REG & 0x80);
}

void main(void) {
    UINT8 joy, last_joy = 0;

    DISPLAY_ON;
    SHOW_BKG;

    printf("LINK TEST SERVER\n");
    printf("--------------------\n");
    printf("Pulsa un boton...\n");

    init_link();

    while (1) {
        wait_vbl_done();
        joy = joypad();

        if (joy != last_joy) {
            if (joy & J_A) {
                printf("\nBoton: A       \n");
                send_link(0xA1);
                printf("Enviado: 0xA1  \n");
            } else if (joy & J_B) {
                printf("\nBoton: B       \n");
                send_link(0xB2);
                printf("Enviado: 0xB2  \n");
            } else if (joy & J_UP) {
                printf("\nBoton: UP      \n");
                send_link(0xA3);
                printf("Enviado: 0xA3  \n");
            } else if (joy & J_DOWN) {
                printf("\nBoton: DOWN    \n");
                send_link(0xA4);
                printf("Enviado: 0xA4  \n");
            } else if (joy & J_LEFT) {
                printf("\nBoton: LEFT    \n");
                send_link(0xA5);
                printf("Enviado: 0xA5  \n");
            } else if (joy & J_RIGHT) {
                printf("\nBoton: RIGHT   \n");
                send_link(0xA6);
                printf("Enviado: 0xA6  \n");
            } else if (joy & J_SELECT) {
                printf("\nBoton: SELECT  \n");
                send_link(0xA7);
                printf("Enviado: 0xA7  \n");
            } else if (joy & J_START) {
                printf("\nBoton: START   \n");
                send_link(0xA8);
                printf("Enviado: 0xA8  \n");
            }

            last_joy = joy;
        }
    }
}
