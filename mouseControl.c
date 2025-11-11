#include "globals.h"

// Globals
extern volatile unsigned char byte1, byte2, byte3;
extern volatile int timeout, flag;
extern struct alt_up_dev up_dev;

// Function Prototypes
void draw_mouse_on_vga(int mouse_x, int mouse_y);
void update_button_status(int button_state);
void clear_screen(void);
void PS2_ISR(struct alt_up_dev *up_dev, unsigned int id);

int main(void)
{
    alt_up_parallel_port_dev *KEY_dev;
    alt_up_parallel_port_dev *green_LEDs_dev;
    alt_up_ps2_dev *PS2_dev;
    alt_up_character_lcd_dev *lcd_dev;
    alt_up_audio_dev *audio_dev;
    alt_up_char_buffer_dev *char_buffer_dev;
    alt_up_pixel_buffer_dma_dev *pixel_buffer_dev;
    volatile int * interval_timer_ptr = (int *) 0x10002000;  
    
    byte1 = 0; byte2 = 0; byte3 = 0;
    timeout = 0;  
    flag = 0;

    int mouse_x = 128;
    int mouse_y = 128;

    int counter = 0x960000;    
    *(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
    *(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;
    *(interval_timer_ptr + 1) = 0x7;    
    
    PS2_dev = alt_up_ps2_open_dev ("/dev/PS2_Port");
    if ( PS2_dev == NULL)
    {
        alt_printf ("Error: could not open PS2 device\n");
        return -1;
    }

    pixel_buffer_dev = alt_up_pixel_buffer_dma_open_dev ("/dev/VGA_Pixel_Buffer");
    if (pixel_buffer_dev == NULL) {
        alt_printf("Error: could not open pixel buffer device\n");
        return -1;
    }

    char_buffer_dev = alt_up_char_buffer_open_dev ("/dev/VGA_Char_Buffer");

    clear_screen();
    
    int screen_x = 319, screen_y = 239;
    short color = 0xC618; 
    alt_up_pixel_buffer_dma_draw_box(pixel_buffer_dev, 0, 0, screen_x, screen_y, color, 0);

    while (1)
    {
        if(flag){
            int dx = (signed char)byte2;
            int dy = (signed char)byte3;

            mouse_x += dx;
            mouse_y -= dy;

            flag = 0;
        }
        int button_state = byte1 & 0x07;
        update_button_status(button_state);

        timeout = 0;
    }
}

void clear_screen(void) {
    alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, 0, 0, 639, 479, 0x0000, 0);
}

void draw_mouse_on_vga(int mouse_x, int mouse_y) {
    if (mouse_x < 0 || mouse_x >= 640 || mouse_y < 0 || mouse_y >= 480) {
        return;
    }

    int color = 0xF800;
    alt_up_pixel_buffer_dma_draw_box(pixel_buffer_dev, mouse_x, mouse_y, mouse_x + 5, mouse_y + 5, color, 0);
}

void update_button_status(int button_state) {
    char status[20];
    alt_up_char_buffer_string(char_buffer_dev, status, 0, 30);  

    alt_up_char_buffer_string(char_buffer_dev, status, 0, 31);

    alt_up_char_buffer_string(char_buffer_dev, status, 0, 32);
}

void PS2_ISR(struct alt_up_dev *up_dev, unsigned int id) {
    unsigned char PS2_data;
    static int byte_count = 0;

    if (alt_up_ps2_read_data_byte (up_dev->PS2_dev, &PS2_data) ==0)
    {

        switch (byte_count) {
            case 0:
                byte1 = PS2_data;
                byte_count++;
                break;
            case 1:
                byte2 = PS2_data;
                byte_count++;
                break;
            case 2:
                byte3 = PS2_data;
                byte_count = 0;
                flag = 1;
                break;
        }
        
        if( (byte2 == (unsigned char) 0xAA) && (byte3 == (unsigned char) 0x00))
            (void) alt_up_ps2_write_data_byte (up_dev->PS2_dev, (unsigned char) 0xF4);
    }
    return;
}


