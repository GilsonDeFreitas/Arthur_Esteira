/*
  Autor: Gilson de Freitas. Rondonópolis-MT
  Contato: 66-9-9998-6062, gilfreitaszbrasil@hotmail.com, www.curseagora.com.br
  Cliente: Artur. São Paulo - SP
  Contato: 11-9-9536-4806
  Ajuda: site displayLCD https://www.arduinoecia.com.br/modulo-i2c-display-16x2-arduino/
  Ajuda: biblioteca display: https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/NewliquidCrystal_1.3.4.zip
  
  Programa: Automatização da fabricação Caixas de verduras
  Funcionamento:
    Potenciometro 1(Pino A0): Controlar o número de pregos: 0 a 3
    Potenciometro 2(Pino A1): Controlar a distância de cada prego em tempo

    LCD_I2C: pinos analogicos A4(SDA) e A5(SCL), GND e VCC

    Botão Zerar Caixas e Emergencia Pino (D3): Quando Acionado, desativará os relés, zera a contagem de caixas e a luz de emergencia
    Botão de Emergência Pino (D4): Quando Acionado, desativará os relés até que a a placa seja reiniciada.
    Relé 1 pino(D7): Irá acionar uma válvula solenoide A para pregar a madeira
    Relé 2 pino(D8): Irá acionar uma válvula solenoide B para pregar a madeira
    Sensor de distancia (Pino D9): Irá detectar a madeira a ser pregada
*/

//inclusão das bibliotecas LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3,POSITIVE);

//inclusão das biblioteca EEPROM
#include <EEPROM.h>

//declaração dos pinos
byte pot_NumePregos = A0;
byte pot_DistPregos = A1;
byte bt_Emergencia = 2;
byte bt_zerar = 3;
byte luz_Emergencia = 4;
byte rele_1 = 7;
byte rele_2 = 8;
byte sensor1 = 9;
byte sensor2 = 10;

//variaveis auxiliares
byte LigaRele = 1;  /* Esta variável pode ser 0 ou 1*/
byte DesligaRele = !LigaRele;
byte varEmergencia = 0;
byte nrPregos = 1;
byte auxNrPregos = 0;

int tempoPregos = 0; // (1000 = 1 segundo);
unsigned long ultimo_tempoPregos = 0;

int tempoDisplay = 300;
unsigned long ultimo_tempoDisplay = 0;

byte Executa_1 = 0;
byte n1 = 0;

byte Executa_2 = 0;

String Texto = "";
int qtd_Caixas = 0;

//Evento para Emergencia
void Emergencia()
{
  if (varEmergencia == 0)
  {
    //Desligar todos os relés
    digitalWrite(rele_1, DesligaRele);
    digitalWrite(rele_2, DesligaRele);
    
    Serial.println("########## Emergencia ##########");
  } 
  varEmergencia = 1;
}

void (*funcReset)() = 0;

//Evento para Zerar_e_Resetar
void Zerar_Caixas()
{
  qtd_Caixas = 0;
  EEPROM.write(0, qtd_Caixas);// grava no endereço 0 da EEPROM, o valor da quantidade de caixas;

  digitalWrite(rele_1, DesligaRele);
  digitalWrite(rele_2, DesligaRele);
  digitalWrite(luz_Emergencia, DesligaRele);  
  funcReset();
}

void executa_display()
{
  Texto = String("Pr:" + String(nrPregos) + String(" / T:") + String(tempoPregos) + String(" ms"));
  lcd.setBacklight(LOW);
  lcd.setBacklight(HIGH);
  lcd.setCursor(0,0);
  lcd.print(Texto);
  Texto = String("Caixas:" + String(qtd_Caixas) );
  lcd.setCursor(0,1);
  lcd.print(Texto);
}

void setup()
{
  qtd_Caixas = EEPROM.read(0);// busca do endereço 0 da EEPROM, o valor da quantidade de caixas e armazena na variável;

  lcd.begin(16,2);
  Serial.begin(9600);
  
  //Configuração dos pinos
  //Obs.: Os pinos analógicos dos potenciometros não precisam ser configurados
  //      pois os analógicos de A0 a A5 são todos de entrada
  //pinMode(bt_Emergencia, INPUT); // vou adicionar ele na interrupção
  pinMode(luz_Emergencia, OUTPUT);
  pinMode(rele_1, OUTPUT);
  pinMode(rele_2, OUTPUT);
  pinMode(sensor1, INPUT);
  pinMode(bt_zerar, INPUT);

  //os relés devem começar desligados, caso contrário já iniciam ativos
  digitalWrite(rele_1, DesligaRele);
  digitalWrite(rele_2, DesligaRele);

  //A qualque momento em que o pino 2 for Acionado, vai chamar a função Emergencia e parar o processo.
  attachInterrupt(digitalPinToInterrupt(bt_Emergencia), Emergencia, RISING);
  attachInterrupt(digitalPinToInterrupt(bt_zerar), Zerar_Caixas, RISING);
}

void loop()
{
  if (varEmergencia == 1)
  {
    digitalWrite(luz_Emergencia, !digitalRead(luz_Emergencia));

    //nrPregos = map(analogRead(pot_NumePregos), 20, 900, 0, 3);
    //tempoPregos = analogRead(pot_DistPregos);
    Serial.print("Pr: "); Serial.println(nrPregos);
    Serial.print("T: "); Serial.print(tempoPregos); Serial.println(" ms");
    
    Texto = String("EMERGENCIA");    
    lcd.setBacklight(HIGH);
    lcd.setCursor(0,0);
    lcd.print(Texto);
  
    Texto = String("Caixas:" + String(qtd_Caixas) );
    lcd.setCursor(0,1);
    lcd.print(Texto);
    delay(1000);
    lcd.setBacklight(LOW);
    delay(1000);//Aguarda 1 segundo
  }// se foi acionada a emergencia, finaliza o loop aqui, sem continuar o código

  if (varEmergencia == 0) //Não está em emergência
  {
    nrPregos = map(analogRead(pot_NumePregos), 20, 900, 0, 3);
    tempoPregos = analogRead(pot_DistPregos);
  
    if ((millis() - ultimo_tempoDisplay) >= tempoDisplay)
    {
      executa_display();
      ultimo_tempoDisplay = millis();
    }
  
    //Primeiro Sensor
    if (digitalRead(sensor1) == 0)
    {
      auxNrPregos = nrPregos * 2;//recebe também, os intervalos para ligar e desligar;
  
      n1 = 1;
      Executa_1 = 1;
      ultimo_tempoPregos = millis();
    }
    
    if (digitalRead(sensor2) == 0)
    {
      Executa_2 = 1;
    }
  
    //Se não houve Emergência, continua o código para executar as funções 1 e/ou 2
    if ((varEmergencia == 0) && ((Executa_1 == 1) || (Executa_2 == 1)) )
    {
      if (Executa_1 == 1)
      {
        if (n1 <= auxNrPregos)
        {
          if ((millis() - ultimo_tempoPregos) >= tempoPregos)
          {
            if (n1 % 2 == 1)
            {//impar
              //Liga os 2 relés
              digitalWrite(rele_1, LigaRele);
              digitalWrite(rele_2, LigaRele);
            }
            else
            {//par
              //Desliga os 2 relés
              digitalWrite(rele_1, DesligaRele);
              digitalWrite(rele_2, DesligaRele);
            }
  
            if (n1 >= auxNrPregos)
            {
              //finalizou esta execução
              Executa_1 = 0;
              qtd_Caixas++;
              EEPROM.write(0, qtd_Caixas);// grava no endereço 0 da EEPROM, o valor da quantidade de caixas;
            }
  
            n1++;
            ultimo_tempoPregos = millis();          
          }
        }  
      }//fim: if (Executa_1 == 1)
    }//fim: if ((varEmergencia == 0) && ((Executa_1 == 1) || (Executa_2 == 1)) )
  }//fim: if (varEmergencia == 0) //Não está em emergência
}
