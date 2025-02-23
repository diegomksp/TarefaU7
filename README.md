# Projeto: Sistema de Controle de Nível de Bomba.

## Descrição

Este projeto utiliza um microcontrolador para monitorar sensores de nível, controlar uma bomba d'água e exibir informações em um display OLED SSD1306. O sistema faz a leitura de dois sensores de nível e, com base nos valores obtidos, controla o estado da bomba (ligada ou desligada) e indica os estados no display. LEDs RGB também são usados para sinalizar os diferentes estados do sistema.

### Principais Funcionalidades:
- **Leitura de Sensores de Nível**: Os sensores de nível são lidos através de entradas analógicas (VRX e VRY).
- **Controle de Bomba**: A bomba é ligada ou desligada dependendo dos valores dos sensores de nível.
- **Display OLED SSD1306**: Exibe as leituras dos sensores de nível e o estado da bomba.
- **LEDs RGB**: Indicam visualmente o estado do sistema.
- **Botões de Controle**: Dois botões (A e B) são utilizados para iniciar ou pausar o sistema.
- **Máquina de Estados**: O sistema possui vários estados como inicial, bomba ligada, volume baixo, erro e pausa, que controlam o comportamento do sistema.

## Hardware Utilizado

- **Microcontrolador**: Raspberry Pi Pico
- **Sensores de Nível**: VRX e VRY conectados aos canais ADC
- **Display OLED**: SSD1306 via barramento I2C
- **LEDs RGB**
- **Bomba d'água**: Controlada por um transistor/rele no pino GPIO
- **Botões de Controle**: Dois botões para interagir com o sistema

### Pinagem

- **I2C**:
  - SDA: Pino 14
  - SCL: Pino 15
  - Endereço do Display: `0x3C`

- **Pinos ADC**:
  - VRX: Pino 26 (ADC 0)
  - VRY: Pino 27 (ADC 1)

- **LEDs RGB**:
  - LED_R: Pino 13
  - LED_G: Pino 11
  - LED_B: Pino 12

- **Botões**:
  - Button_A: Pino 5
  - Button_B: Pino 6

- **Bomba**:
  - Pino 4

## Como Funciona

1. **Leitura dos Sensores de Nível**: Os valores dos sensores são convertidos para uma escala de porcentagem (0-100%).
2. **Exibição no Display OLED**: As leituras dos sensores são exibidas em tempo real no display OLED.
3. **Controle da Bomba**: A bomba é ligada ou desligada com base nas condições dos sensores.
4. **Máquina de Estados**: O sistema usa uma máquina de estados para gerenciar suas funções:
   - **Estado Inicial**: Aguardando leitura dos sensores.
   - **Bomba Ligada**: Quando as condições de nível são adequadas.
   - **Volume Baixo**: Quando o nível está abaixo do mínimo.
   - **Erro**: Em caso de falha ou tempo excedido.
   - **Pausa**: Sistema em pausa até nova ação do usuário.

## Como Compilar e Executar

1. Clone o repositório e adicione as bibliotecas necessárias para o display SSD1306 e o controle do hardware.
2. Configure o ambiente de desenvolvimento do Raspberry Pi Pico com SDK.
3. Compile o código utilizando a ferramenta `CMake` fornecida pelo SDK.
4. Carregue o código no Raspberry Pi Pico.
5. Conecte o hardware conforme a pinagem descrita e execute o sistema.

## Dependências

- **SDK do Raspberry Pi Pico**
- **Biblioteca de Controle do Display SSD1306**
- **Bibliotecas de ADC e GPIO do Raspberry Pi Pico**

## Demonstração

<<<>>>.