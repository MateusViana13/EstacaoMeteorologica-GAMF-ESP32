#include <Adafruit_BMP280.h>
#include <AS5600.h>
#include <BH1750.h>
#include <DHT.h>
#include <SimpleTimer.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include "SSD1306Wire.h" 
#include <SPI.h>
//#include "images.h"

// Definindo o pino para o encoder
#define PINO_ENCODER 35

// Inicializando os sensores
Adafruit_BMP280 sensorBMP;    // Sensor de Pressão e Temperatura (BMP280)
AS5600 sensorDirecaoVento;    // Sensor de Direção do Vento (AS5600)
BH1750 sensorLuminosidade;    // Sensor de Luminosidade (BH1750)
DHT sensorUmidade(32, DHT11); // Sensor de Umidade (DHT11)

//SERVO MOTOR
Servo servoMotor;
#define PINO_MOTOR 14

//BOMBA D'AGUA
#define PINO_WATERPUMP 17

SimpleTimer TimerLeituras[6]; // Array de temporizadores para leituras

int IntervalosLeitura[6] = {2000, 2000, 3000, 5000, 40000, 3000};
// Intervalos de leitura para cada sensor em milissegundos
// INDEX 0: Direção do Vento - 2s
// INDEX 1: Encoder (RPM) - 2s
// INDEX 2: Luminosidade - 3s
// INDEX 3: Pressão/Temperatura - 5s
// INDEX 4: Umidade - 40s
// INDEX 5: JSON (Node-Red) - 3s

float Percentuais[5] = {2, 10, 2, 1, 5};
// Percentuais de variação para cada sensor
// INDEX 0: Direção do Vento
// INDEX 1: Encoder (RPM)
// INDEX 2: Luminosidade
// INDEX 3: Pressão/Temperatura
// INDEX 4: Umidade

float ValoresAnteriores[6] = {0}; // Valores anteriores dos sensores
int Valores[6] = {0};             // Valores atuais dos sensores
// INDEX 0: Direção do Vento
// INDEX 1: Encoder (RPM)
// INDEX 2: Luminosidade
// INDEX 3: Pressão
// INDEX 4: Temperatura
// INDEX 5: Umidade

bool EstadoAnteriorEncoder = LOW; // Estado anterior do pino do encoder
bool EstadoAtualEncoder = LOW;    // Estado atual do pino do encoder
int ContadorPulsos = 0;           // Contador de pulsos do encoder

const int NumeroAmostras = 5;
float LeiturasAnteriores[NumeroAmostras] = {0};
float MediaMovel = 0;

//DISPLAY
SSD1306Wire display(0x3c, 25, 26);   // ADDRESS, SDA, SCL 
typedef void (*Demo)(void);

void setup()
{
    Serial.begin(115200); // Inicia a comunicação serial com uma taxa de 115200 bauds
    
    Wire.begin();         // Inicia a comunicação I2C (protocolo de comunicação) para sensores que utilizam o protocolo I2C
    display.init();

    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);

    //Wire.begin();

    pinMode(PINO_ENCODER, INPUT);                       // Define o pino do encoder como entrada
    sensorBMP.begin(0x76);                              // Inicia o sensor BMP280 com o endereço 0x76
    sensorDirecaoVento.begin(4);                        // Inicia o sensor de direção do vento no pino 4
    sensorDirecaoVento.setDirection(AS5600_CLOCK_WISE); // Configura a direção do sensor AS5600 como horário (clockwise)
    sensorLuminosidade.begin();                         // Inicia o sensor de luminosidade
    sensorUmidade.begin();                              // Inicia o sensor de umidade (DHT11)
    
    servoMotor.attach(PINO_MOTOR);
    servoMotor.write(0);

    pinMode(PINO_WATERPUMP, OUTPUT);



    for (int i = 0; i < 6; i++) // Configura os intervalos de leitura para cada sensor
        TimerLeituras[i].setInterval(IntervalosLeitura[i]);
}


// void drawValoresSensores() {
//     display.clear();
//     display.setTextAlignment(TEXT_ALIGN_LEFT);
//     display.setFont(ArialMT_Plain_10);

//     display.drawString(0, 0, "Temperatura: ");

//     // Exibe os valores dos sensores
//     //display.drawString(0, 0, "Temperatura: " + String(Valores[4]) + " C");
//     //display.drawString(0, 12, "Umidade: " + String(Valores[5]) + " %");
//     //display.drawString(0, 24, "Luminosidade: " + String(Valores[2]) + " lux");
//     //display.drawString(0, 36, "Pressão: " + String(Valores[3]) + " hPa");
 
//     display.display();
// }

// void drawDadosVento() {
//     display.clear();
//     display.setTextAlignment(TEXT_ALIGN_LEFT);
//     display.setFont(ArialMT_Plain_10);

//     // Exibe os dados relacionados ao vento
//     display.drawString(0, 0, "Direção do Vento: " + DirecaoVento(Valores[0]));
//     display.drawString(0, 12, "Velocidade do Vento: " + String(Valores[1]) + " RPM");

//     display.display();
// }

// Demo demos[] = { drawValoresSensores };

void loop()
{
   // display.clear();
   //   // draw the current demo method


   //   display.setFont(ArialMT_Plain_10);
   //   display.setTextAlignment(TEXT_ALIGN_RIGHT);
   //   display.drawString(128, 54, String(millis()));
   //   // write the buffer to the display
   //   display.display();

   //   delay(10);

    if (TimerLeituras[0].isReady()) // Verifica se é hora de fazer a leitura da Direção do Vento
    {
        //servoMotor.write(180);
        Valores[0] = abs((sensorDirecaoVento.rawAngle() * 360.0 / 4095.0));         // Lê a direção do vento do sensor e a converte para graus
        AtualizaSensorPercentual(Valores[0], ValoresAnteriores[0], Percentuais[0]); // Atualiza o valor anterior da direção do vento se a variação for maior que o percentual definido
        TimerLeituras[0].reset();   
        ControleDirecaoMotor(DirecaoVento(Valores[0]), 0);                                                // Reseta o temporizador
    }

    if (TimerLeituras[1].isReady()) // Verifica se é hora de fazer a leitura do Encoder (RPM)
    {
       // Valores[1] = ((ContadorPulsos * 6)) * 1000.00 / IntervalosLeitura[1];       // Calcula a velocidade em RPM a partir dos pulsos contados no intervalo de tempo
        Valores[1] = ((CalculaMediaMovel(ContadorPulsos) * 6)) * 1000.00 / IntervalosLeitura[1];
        ContadorPulsos = 0;                                                         // Zera o contador de pulsos para a próxima contagem
        AtualizaSensorPercentual(Valores[1], ValoresAnteriores[1], Percentuais[0]); // Atualiza o valor anterior da velocidade se a variação for maior que o percentual definido
        TimerLeituras[1].reset();                                                   // Reseta o temporizador
    }

    if (TimerLeituras[2].isReady()) // Verifica se é hora de fazer a leitura da Luminosidade
    {
        Valores[2] = sensorLuminosidade.readLightLevel();                           // Lê o nível de luminosidade do sensor
        AtualizaSensorPercentual(Valores[2], ValoresAnteriores[2], Percentuais[1]); // Atualiza o valor anterior da luminosidade se a variação for maior que o percentual definido
        TimerLeituras[2].reset();                                                   // Reseta o temporizador
    }

    if (TimerLeituras[3].isReady()) // Verifica se é hora de fazer a leitura da Pressão e Temperatura
    {
        // Lê a pressão e a temperatura do sensor BMP280
        Valores[3] = sensorBMP.readPressure() / 100;
        Valores[4] = sensorBMP.readTemperature();
        AtualizaSensorPercentual(Valores[3], ValoresAnteriores[3], Percentuais[2]); // Atualiza o valor anterior da pressão se a variação for maior que o percentual definido
        AtualizaSensorPercentual(Valores[4], ValoresAnteriores[4], Percentuais[3]); // Atualiza o valor anterior da temperatura se a variação for maior que o percentual definido
        TimerLeituras[3].reset();                                                   // Reseta o temporizador
    }

    if (TimerLeituras[4].isReady()) // Verifica se é hora de fazer a leitura da Umidade
    {
        Valores[5] = sensorUmidade.readHumidity();                                  // Lê o nível de umidade do sensor DHT11
        if(Valores[5] <= 40){
          LigarBomba();
        }
        AtualizaSensorPercentual(Valores[5], ValoresAnteriores[5], Percentuais[4]); // Atualiza o valor anterior da umidade se a variação for maior que o percentual definido
        TimerLeituras[4].reset();                                                   // Reseta o temporizador
    }

    if (TimerLeituras[5].isReady()) // Verifica se é hora de gerar o JSON para Node-RED
    {
        // Gera uma string JSON com os dados lidos
        Serial.print("{\"Tempo\":");
        Serial.print(millis());
        Serial.print(",\"LeituraDirecaoVento\":");
        Serial.print(Valores[0]);
        Serial.print(",\"LeituraEncoder\":");
        Serial.print(Valores[1]);
        Serial.print(",\"LeituraLuminosidade\":");
        Serial.print(Valores[2]);
        Serial.print(",\"LeituraPressao\":");
        Serial.print(Valores[3]);
        Serial.print(",\"LeituraTemperatura\":");
        Serial.print(Valores[4]);
        Serial.print(",\"LeituraUmidade\":");
        Serial.print(Valores[5]);
        Serial.print(",\"DirecaoVento\":\"");
        Serial.print(DirecaoVento(Valores[0]));
        Serial.println("\"}");
        // display.setFont(ArialMT_Plain_10);
        // display.setTextAlignment(TEXT_ALIGN_RIGHT);
        // display.drawString(128, 54, String(millis()));
        // display.display();

        TimerLeituras[5].reset(); // Reseta o temporizador
    }

    EstadoAtualEncoder = digitalRead(PINO_ENCODER);                 // Lê o estado atual do pino do encoder
    if (EstadoAtualEncoder == HIGH && EstadoAnteriorEncoder == LOW) // Verifica se houve uma transição de estado de LOW para HIGH no pino do encoder
    {
        ContadorPulsos++;             // Incrementa o contador de pulsos (uma rotação completa do encoder)
        EstadoAnteriorEncoder = HIGH; // Atualiza o estado anterior do pino do encoder para HIGH
    }

    if (EstadoAtualEncoder == LOW && EstadoAnteriorEncoder == HIGH) // Verifica se houve uma transição de estado de HIGH para LOW no pino do encoder
        EstadoAnteriorEncoder = LOW;                                // Atualiza o estado anterior do pino do encoder para LOW
}

String DirecaoVento(int Valor)
{
    String Direcao;
    // Condições para determinar a direção do vento baseada no valor numérico (Valor)
    if (Valor > 337.5 || Valor < 22.5)
        Direcao = "N"; // Norte
    if (Valor > 22.5 && Valor < 67.5)
        Direcao = "NE"; // Nordeste
    if (Valor > 67.5 && Valor < 112.5)
        Direcao = "L"; // Leste
    if (Valor > 112.5 && Valor < 157.5)
        Direcao = "SE"; // Sudeste
    if (Valor > 157.5 && Valor < 202.5)
        Direcao = "S"; // Sul
    if (Valor > 202.5 && Valor < 247.5)
        Direcao = "SO"; // Sudoeste
    if (Valor > 247.5 && Valor < 292.5)
        Direcao = "O"; // Oeste
    if (Valor > 292.5 && Valor < 337.5)
        Direcao = "NO"; // Noroeste
    return Direcao;     // Retorna a string que representa a direção do vento
}

void LigarBomba(){
   digitalWrite(PINO_WATERPUMP, HIGH);
   delay(10000);
   digitalWrite(PINO_WATERPUMP, LOW);
}

void ControleDirecaoMotor(String valorDirecaoVento, float valorEncoder){
    //if(valorEncoder > 100){

       if(valorDirecaoVento == "L" || valorDirecaoVento == "SE")
            servoMotor.write(180); 
 
       if(valorDirecaoVento == "NE")
            servoMotor.write(0); 

        if(valorDirecaoVento == "N")
            servoMotor.write(0);  

        if(valorDirecaoVento == "NO")
            servoMotor.write(0);  
        
        if(valorDirecaoVento == "O")
            servoMotor.write(180); 

        if(valorDirecaoVento == "S0")
            servoMotor.write(90);  
    //}
}

float CalculaMediaMovel(float novoValor)
{
    // Atualiza o array de leituras anteriores
    for (int i = NumeroAmostras - 1; i > 0; i--)
    {
        LeiturasAnteriores[i] = LeiturasAnteriores[i - 1];
    }
    LeiturasAnteriores[0] = novoValor;

    // Calcula a média móvel
    float soma = 0;
    for (int i = 0; i < NumeroAmostras; i++)
    {
        soma += LeiturasAnteriores[i];
    }

    MediaMovel = soma / NumeroAmostras;

    return MediaMovel;
}

void AtualizaSensorPercentual(float valorAtual, float &valorAnterior, float percentual)
{
    // Calcula a diferença absoluta entre o valor atual e o valor anterior
    // Compara essa diferença com um percentual da leitura atual
    // Se a diferença for maior ou igual a esse percentual, atualiza o valor anterior
    if (abs(valorAtual - valorAnterior) >= (percentual / 100.0 * valorAtual))
        valorAnterior = valorAtual; // Atualiza o valor anterior com o valor atual
}