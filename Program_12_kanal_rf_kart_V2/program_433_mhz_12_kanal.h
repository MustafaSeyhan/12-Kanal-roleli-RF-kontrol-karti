#include <16F876A.h>
#device ADC=10
#use delay(crystal=8000000)
#use FIXED_IO( B_outputs=PIN_B7,PIN_B6,PIN_B5,PIN_B4,PIN_B3,PIN_B2,PIN_B1 )
#use FIXED_IO( C_outputs=PIN_C7,PIN_C6,PIN_C5,PIN_C4,PIN_C3,PIN_C1 )
#define mod_sec_1	PIN_A0
#define mod_sec_2	PIN_A1
#define mod_sec_3	PIN_A2
#define mod_sec_4	PIN_A3
#define rf_data	PIN_B0
#define role_6	PIN_B1
#define role_7	PIN_B2
#define role_8	PIN_B3
#define role_9	PIN_B4
#define role_10	PIN_B5
#define role_11	PIN_B6
#define role_12	PIN_B7
#define led_kullanici	PIN_C1
#define buton_kullanici	PIN_C2
#define role_1	PIN_C3
#define role_2	PIN_C4
#define role_3	PIN_C5
#define role_4	PIN_C6
#define role_5	PIN_C7


