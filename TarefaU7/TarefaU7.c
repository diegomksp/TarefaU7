#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/i2c.h"
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C // Endereço do display SSD1306
#define bomba_PIN 4
#define TEMP_MAX 6000

// Variáveis para controle de tempo e estados
static volatile uint32_t last_time = 0;
static volatile uint32_t last_time2 = 0;
static volatile uint8_t estado = 0;
alarm_id_t alarme_emergencia;

// Definições de pinos e constantes para o Sensor_de_nivel, botões e LEDs
const int VRX = 26, VRY = 27, ADC_CHANNEL_0 = 0, ADC_CHANNEL_1 = 1, LED_R = 13, LED_G = 11, LED_B = 12,button_A = 5,button_B = 6;
const float DIVIDER_PWM = 40.0;
const uint16_t PERIOD = 500;

// Variáveis para controle do PWM dos LEDs
uint slice_LED_R, slice_LED_G, slice_LED_B;

// Estrutura para o display OLED
ssd1306_t ssd;
static volatile bool ledON=true;

// Declaração das funções
void setup_Sensor_de_nivel();
void setup_PWM(uint led, uint *slice, uint16_t level);
void setup_leds();
void setup();
void setup_button();
void read_Sensores_de_nivel(uint16_t *VRX_value,uint16_t *VRY_value);
void setup_i2c();
void button_callback(uint gpio, uint32_t events);
void RGB(int r,int g, int b);
void setup_bomba();
void ligar_bomba();
void desligar_bomba();



int64_t Emergencia(alarm_id_t id, void *user_data){
    estado=3;
    desligar_bomba();
    return 0;
}
int64_t blink_led(alarm_id_t id, void *user_data){
    ledON=!ledON;
    add_alarm_in_ms(1000,blink_led,NULL,0);
    return 0;
}

int main()
{
    setup(); // Configuração inicial do sistema
    setup_i2c(); // Configuração do barramento I2C

    // Variáveis para armazenar leituras do Sensor_de_nivel
    uint16_t VRX_value, VRY_value;
    

    // Alarme para cadencia do blink
    add_alarm_in_ms(1000,blink_led,NULL,0);
    
    char string[10];
    char string2[10];
    gpio_set_irq_enabled_with_callback(button_A,GPIO_IRQ_EDGE_FALL,true,&button_callback);
    gpio_set_irq_enabled_with_callback(button_B,GPIO_IRQ_EDGE_FALL,true,&button_callback);
    
    while (1)
    {
        // Lê os valores do Sensor_de_nivel
        read_Sensores_de_nivel(&VRX_value,&VRY_value);
        
        // Converte os valores do Sensor_de_nivel para PWM
        int pwm_value_x=abs(VRX_value*PERIOD/4095);
        int pwm_value_y=abs(VRY_value*PERIOD/4095);
        
        int x2 = abs((VRX_value * 101) / 4095);
        int y2 = abs((VRY_value * 101) / 4095);
        sprintf(string, "R.Sup. ""%d%%", x2);
        sprintf(string2, "R.Inf. ""%d%%", y2);
        
        // Atualiza a tela OLED
        ssd1306_fill(&ssd,0);
        ssd1306_draw_string(&ssd,string,6,1);
        ssd1306_draw_string(&ssd,string2,6,11);
        
        switch(estado){
            case 1:
            if(x2<98&&y2>10){
                ssd1306_draw_string(&ssd,"Bomba ON!",6,50);
                ligar_bomba();
                RGB(0,0,1);
            }else{
                if (alarme_emergencia != (alarm_id_t)(intptr_t)NULL) {
                    cancel_alarm(alarme_emergencia);
                    alarme_emergencia = (alarm_id_t)(intptr_t)NULL; // Resetar o ID do alarme
                }
                desligar_bomba();
                estado=0;
            }
            break;
            
            case 2:
            if(y2<40){
                ssd1306_draw_string(&ssd,"Bomba OFF",6,21);
                ssd1306_draw_string(&ssd,"vol. insuf.!",6,50);
                if(y2>10){
                    RGB(1,1,0);}
                    else{
                        RGB(1,0,0);}
                    }else{
                        estado=0;
                    }
                    break;
            case 3:
                ssd1306_fill(&ssd,0);
                ssd1306_draw_string(&ssd,"Erro(timer)",6,11);
                ssd1306_draw_string(&ssd,"Ver. sensores",6,21);
                ssd1306_draw_string(&ssd,"e press 'A'",6,31);
                ssd1306_draw_string(&ssd,"Bomba OFF",6,41);
                RGB(1,0,1);
                break;
                case 4:
                ssd1306_fill(&ssd,0);
                ssd1306_draw_string(&ssd,"Ciclo em pausa",6,11);
                ssd1306_draw_string(&ssd,"pressione A",6,21);
                ssd1306_draw_string(&ssd,"p/ continue.,",6,31);
                ssd1306_draw_string(&ssd,"Bomba OFF",6,41);
                RGB(1,1,1);
                break;

            default:
                if(x2<30 && y2 > 40){
                    printf("definido o alarme emergencia.\n");
                    alarme_emergencia = add_alarm_in_ms(TEMP_MAX, Emergencia, NULL, 0);
                    estado=1;
                    }
                if(x2>=30 && y2 > 10){
                    ssd1306_draw_string(&ssd,"Aguardando...",6,50);
                    ssd1306_draw_string(&ssd,"Bomba OFF",6,21);
                    RGB(0,ledON,0);
                }
                if(y2 < 10){
                    estado=2;
                }
                break;
        }
        ssd1306_send_data(&ssd);
        sleep_ms(10); //delay para evitar alto consumo da CPU
    }
}

void setup_Sensor_de_nivel()
{
    adc_init();
    adc_gpio_init(VRX);
    adc_gpio_init(VRY);
}

void setup_PWM(uint led, uint *slice, uint16_t level)
{
    gpio_set_function(led, GPIO_FUNC_PWM);
    *slice = pwm_gpio_to_slice_num(led);
    pwm_set_clkdiv(*slice, DIVIDER_PWM);
    pwm_set_wrap(*slice, PERIOD);
    pwm_set_gpio_level(led, level);
    pwm_set_enabled(*slice, true);
}

void setup()
{
    stdio_init_all();
    setup_Sensor_de_nivel();
    setup_leds();
    setup_bomba();
    setup_button();
     
}

void read_Sensores_de_nivel(uint16_t *VRX_value,uint16_t *VRY_value)
{
    adc_select_input(ADC_CHANNEL_1);
    sleep_ms(2);
    *VRX_value = adc_read();  // Lê VRX corretamente
    adc_select_input(ADC_CHANNEL_0);
    sleep_ms(2);
    *VRY_value = adc_read();  // Lê VRX corretamente
}

void setup_i2c(){
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
        // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    // ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void button_callback(uint gpio, uint32_t events) {
    // Cancela o alarme se ele estiver ativo
    if (alarme_emergencia != (alarm_id_t)(intptr_t)NULL) {
        cancel_alarm(alarme_emergencia);
        alarme_emergencia = (alarm_id_t)(intptr_t)NULL; // Resetar o ID do alarme
    }
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());
    if (gpio == button_A) {
        if (tempo_atual - last_time > 200000) {
            last_time = tempo_atual;
            estado = 0;  // Lógica para o botão A
        }
    } else if (gpio == button_B) {  // Lógica separada para o botão B
        if (tempo_atual - last_time2 > 200000) {
            last_time2 = tempo_atual;
            estado = 4;  // Lógica para o botão B
        }
    }
}


void setup_leds(){
    
    gpio_init(LED_R);
    gpio_set_dir(LED_R,GPIO_OUT);
    gpio_init(LED_G);
    gpio_set_dir(LED_G,GPIO_OUT);
    gpio_init(LED_B);
    gpio_set_dir(LED_B,GPIO_OUT);

    gpio_put(LED_R,0);
    gpio_put(LED_G,0);
    gpio_put(LED_B,0);

}

void RGB(int r,int g, int b){
    gpio_put(LED_R,r);
    gpio_put(LED_G,g);
    gpio_put(LED_B,b);
}

void setup_bomba() {
    gpio_init(bomba_PIN);
    gpio_set_dir(bomba_PIN, GPIO_OUT);
    gpio_put(bomba_PIN, 0); // Começa desligada
}

void ligar_bomba() {
    gpio_put(bomba_PIN, 1);
}

void desligar_bomba() {
    gpio_put(bomba_PIN, 0);
}

void setup_button(){
    gpio_init(button_A);
    gpio_set_dir(button_A,GPIO_IN);
    gpio_pull_up(button_A);
    gpio_init(button_B);
    gpio_set_dir(button_B,GPIO_IN);
    gpio_pull_up(button_B);
}
