#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/i2c.h"

//Definições de pinos e constantes
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco_display 0x3C // Endereço do display SSD1306
#define bomba_PIN 4

//Enumeração de estádos da máquina
typedef enum{
    ESTADO_INICIAL =0,
    ESTADO_BOMBA_LIGADA=1,
    ESTADO_VOLUME_BAIXO=2,
    ESTADO_ERRO=3,
    ESTADO_PAUSA=4,
}Estados;

//variável para armazenamento o estado atual
static volatile uint8_t estado = ESTADO_INICIAL;

//Estrutura para configurações dos pinos 
typedef struct{
    const int VRX;
    const int VRY;
    const int ADC_CHANNEL_0;
    const int ADC_CHANNEL_1;
    const int LED_R;
    const int LED_G;
    const int LED_B;
    const int button_A;
    const int button_B;
} ConfigPinos;

//configuração dos pinos
static const ConfigPinos pinos = {26,27,0,1,13,11,12,5,6};

//tempo de parada para exemplificar no vídeo.
static int TEMP_MAX = 6000;

// Variáveis para controle de tempo e estados
static volatile uint32_t last_time = ESTADO_INICIAL;
static volatile uint32_t last_time2 = ESTADO_INICIAL;
static volatile bool ledON=true;

// Estrutura para o display OLED
ssd1306_t ssd;

// Declaração das funções
void setup_Sensor_de_nivel();
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

//Função callback para o alarme de tempo excedido
alarm_id_t alarme_tempo_excedido;
int64_t Emergencia(alarm_id_t id, void *user_data){
    estado=ESTADO_ERRO;
    desligar_bomba();
    return 0;
}

//Função callback para piscar o led
int64_t blink_led(alarm_id_t id, void *user_data){
    ledON=!ledON;
    add_alarm_in_ms(1000,blink_led,NULL,0);
    return 0;
}

//Função principal
int main()
{
    setup(); // Configuração inicial do sistema
    setup_i2c(); // Configuração do barramento I2C

    // Variáveis para armazenar leituras do Sensor_de_nivel
    uint16_t VRX_value, VRY_value;
    

    // Alarme para cadencia do blink
    add_alarm_in_ms(1000,blink_led,NULL,0);
    
    //variáveis para armazenar strings e exibir no display
    char string[10];
    char string2[10];

    //declaração das interrupções
    gpio_set_irq_enabled_with_callback(pinos.button_A,GPIO_IRQ_EDGE_FALL,true,&button_callback);
    gpio_set_irq_enabled_with_callback(pinos.button_B,GPIO_IRQ_EDGE_FALL,true,&button_callback);
    
    while (1)
    {
        // Lê os valores do Sensor_de_nivel
        read_Sensores_de_nivel(&VRX_value,&VRY_value);
        
        //recebe as entradas analógicas 12 bits e transforma em formado de porcentagem 100 + 1 devido ao 0.
        int x2 = abs((VRX_value * 101) / 4095);
        int y2 = abs((VRY_value * 101) / 4095);

        //Armazena valores de forma diâmica nas variáveis que serão exibidas no display
        sprintf(string, "R.Sup. ""%d%%", x2);
        sprintf(string2, "R.Inf. ""%d%%", y2);
        
        // Atualiza a display OLED
        ssd1306_fill(&ssd,0);
        ssd1306_draw_string(&ssd,string,6,1);
        ssd1306_draw_string(&ssd,string2,6,11);
        
        // Máquina de estados
        switch(estado){
            case 1:
            if(x2<98&&y2>10){
                ssd1306_draw_string(&ssd,"Bomba ON!",6,50);
                ligar_bomba();
                RGB(0,0,1);
            }else{
                if (alarme_tempo_excedido != (alarm_id_t)(intptr_t)NULL) {
                    cancel_alarm(alarme_tempo_excedido);
                    alarme_tempo_excedido = (alarm_id_t)(intptr_t)NULL; // Resetar o ID do alarme
                }
                desligar_bomba();
                estado=ESTADO_INICIAL;
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
                        estado=ESTADO_INICIAL;
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
                    alarme_tempo_excedido = add_alarm_in_ms(TEMP_MAX, Emergencia, NULL, 0);
                    estado=ESTADO_BOMBA_LIGADA;
                    }
                if(x2>=30 && y2 > 10){
                    ssd1306_draw_string(&ssd,"Aguardando...",6,50);
                    ssd1306_draw_string(&ssd,"Bomba OFF",6,21);
                    RGB(0,ledON,0);
                }
                if(y2 < 10){
                    estado=ESTADO_VOLUME_BAIXO;
                }
            break;
        }
        ssd1306_send_data(&ssd);
        sleep_ms(10); //delay para evitar alto consumo da CPU
    }
}

//Função para verificar os sensores de nível
void setup_Sensor_de_nivel()
{
    adc_init();
    adc_gpio_init(pinos.VRX);
    adc_gpio_init(pinos.VRY);
}

//função de configuração inicial
void setup()
{
    stdio_init_all();
    setup_Sensor_de_nivel();
    setup_leds();
    setup_bomba();
    setup_button();
     
}

//função para ler os valores dos sensores de nível
void read_Sensores_de_nivel(uint16_t *VRX_value,uint16_t *VRY_value)
{
    adc_select_input(pinos.ADC_CHANNEL_1);
    sleep_ms(2);
    *VRX_value = adc_read();  // Lê VRX corretamente
    adc_select_input(pinos.ADC_CHANNEL_0);
    sleep_ms(2);
    *VRY_value = adc_read();  // Lê VRY corretamente
}

//Função para configurar o barramento i2c
void setup_i2c(){
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
        // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco_display, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    // ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

//Funçao callback para os botões
void button_callback(uint gpio, uint32_t events) {
    // Cancela o alarme se ele estiver ativo
    if (alarme_tempo_excedido != (alarm_id_t)(intptr_t)NULL) {
        cancel_alarm(alarme_tempo_excedido);
        alarme_tempo_excedido = (alarm_id_t)(intptr_t)NULL; // Resetar o ID do alarme
    }
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());
    if (gpio == pinos.button_A) {
        if (tempo_atual - last_time > 200000) {
            last_time = tempo_atual;
            estado = ESTADO_INICIAL;  // Lógica para o botão A
        }
    } else if (gpio == pinos.button_B) {  // Lógica separada para o botão B
        if (tempo_atual - last_time2 > 200000) {
            last_time2 = tempo_atual;
            estado = ESTADO_PAUSA;  // Lógica para o botão B
        }
    }
}

//Função para iniciar os leds
void setup_leds(){
    
    gpio_init(pinos.LED_R);
    gpio_set_dir(pinos.LED_R,GPIO_OUT);
    gpio_init(pinos.LED_G);
    gpio_set_dir(pinos.LED_G,GPIO_OUT);
    gpio_init(pinos.LED_B);
    gpio_set_dir(pinos.LED_B,GPIO_OUT);

    gpio_put(pinos.LED_R,0);
    gpio_put(pinos.LED_G,0);
    gpio_put(pinos.LED_B,0);

}

//Função para controlar os leds
void RGB(int r,int g, int b){
    gpio_put(pinos.LED_R,r);
    gpio_put(pinos.LED_G,g);
    gpio_put(pinos.LED_B,b);
}

//função para inicializar o pino que se comunicará com a bomba por meio de transistor e relé
void setup_bomba() {
    gpio_init(bomba_PIN);
    gpio_set_dir(bomba_PIN, GPIO_OUT);
    gpio_put(bomba_PIN, 0); // Começa desligada
}

//Função para ligar a bomba
void ligar_bomba() {
    gpio_put(bomba_PIN, 1);
}

//Função para desligar a bomba
void desligar_bomba() {
    gpio_put(bomba_PIN, 0);
}

//Função para inicializar os botões A e B em pull_up
void setup_button(){
    gpio_init(pinos.button_A);
    gpio_set_dir(pinos.button_A,GPIO_IN);
    gpio_pull_up(pinos.button_A);
    gpio_init(pinos.button_B);
    gpio_set_dir(pinos.button_B,GPIO_IN);
    gpio_pull_up(pinos.button_B);
}
