#include <LiquidCrystal.h>
#include <Keypad.h>
#include "HX711.h"
#include "DHT.h"

LiquidCrystal _lcd(2, 15, 4, 5, 18, 19);

const int LOADCELL_DOUT_PIN = 32;
const int LOADCELL_SCK_PIN = 12;
HX711 _scale;
float _calibrationFactor = 28955.0;

const byte LINHAS = 4;
const byte COLUNAS = 4;
char _teclas[LINHAS][COLUNAS] = {
    {'E', '7', '4', '1'},
    {'0', '8', '5', '2'},
    {'R', '9', '6', '3'},
    {'L', 'D', 'P', ' '}
};

byte _pinosLinhas[LINHAS] = {23, 22, 21, 13};
byte _pinosColunas[COLUNAS] = {25, 26, 27, 14};
Keypad _teclado = Keypad(makeKeymap(_teclas), _pinosLinhas, _pinosColunas, LINHAS, COLUNAS);

#define DHTPIN 33
#define DHTTYPE DHT22
DHT _dht(DHTPIN, DHTTYPE);

String _entradaTexto = "";
int _cursorPos = 0;
float _alturaUsuario = 0.0;
float _pesoUsuario = 0.0;
float _ultimoPesoLido = 0.0;
float _pesoAtual = 0.0;

enum Estado { AGUARDANDO_ALTURA, AGUARDANDO_PESO, STANDBY };
Estado _estadoAtual = AGUARDANDO_ALTURA;

bool _contandoPesagem = false;
unsigned long _lastActivity = 0;
unsigned long _tempoInicioPesagem = 0;
unsigned long _timerStandby = 0;
unsigned long _timerPesoDisplay = 0;

const long TEMPO_STANDBY = 10000;
const long TEMPO_PESAGEM = 5000;

void AtualizarDisplayAltura();
void AnimacaoErro();
void AnimacaoCalculando();
void FinalizarCalculo();
void ProcessarPesagem(float peso);
void ValidarAltura();
void ProcessarAltura(char tecla);
void ExecutarStandby();
void AcordarSistema();
void EntrarStandby();
void ReiniciarSistema();
void AnimacaoInicial();
void PrintSerialBanner();

void setup()
{
    Serial.begin(9600);
    _lcd.begin(16, 2);
    _dht.begin();
    
    _scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    _scale.set_scale(_calibrationFactor);
    _scale.power_up();
    _scale.tare();
    
    _teclado.setDebounceTime(50);
    _teclado.setHoldTime(400);

    PrintSerialBanner();
    AnimacaoInicial();
    ReiniciarSistema();
}

void loop()
{
    char tecla = _teclado.getKey();
    String entradaSerial = "";

    if (_scale.is_ready())
    {
        _pesoAtual = _scale.get_units(1);

        if (_pesoAtual < 0)
        {
            _pesoAtual = 0;
        }
    }

    if (Serial.available() > 0)
    {
        entradaSerial = Serial.readStringUntil('\n');
        entradaSerial.trim();
    }

    bool pesoMudou = abs(_pesoAtual - _ultimoPesoLido) > 0.5 || _pesoAtual > 2.0;

    if (tecla || entradaSerial.length() > 0 || pesoMudou)
    {
        _lastActivity = millis();
        _ultimoPesoLido = _pesoAtual;

        if (_estadoAtual == STANDBY)
        {
            AcordarSistema();

            return;
        }
    }

    if (_estadoAtual != STANDBY && (millis() - _lastActivity > TEMPO_STANDBY))
    {
        EntrarStandby();

        return;
    }

    switch (_estadoAtual)
    {
        case STANDBY:
            ExecutarStandby();
            break;

        case AGUARDANDO_ALTURA:
            if (entradaSerial.length() > 0)
            {
                Serial.print("[SERIAL] Valor recebido: ");
                Serial.println(entradaSerial);
                _entradaTexto = entradaSerial;
                _cursorPos = _entradaTexto.length();

                AtualizarDisplayAltura();
                ValidarAltura();
            }
            else
            {
                ProcessarAltura(tecla);
            }
            break;

        case AGUARDANDO_PESO:
            ProcessarPesagem(_pesoAtual);
            break;
    }

    delay(50);
}

void ReiniciarSistema()
{
    _estadoAtual = AGUARDANDO_ALTURA;
    _contandoPesagem = false;
    _entradaTexto = "";
    _cursorPos = 0;
    _lastActivity = millis();

    _lcd.clear();
    _lcd.print("Altura (m):");

    Serial.println();
    Serial.println("============================");
    Serial.println("  [AGUARDANDO] Altura (m)   ");
    Serial.println("  Teclado: digite + ENTER   ");
    Serial.println("  Serial:  envie o valor    ");
    Serial.println("  Ex: 1.75                  ");
    Serial.println("============================");
}

void EntrarStandby()
{
    _estadoAtual = STANDBY;
    _contandoPesagem = false;
    _timerStandby = 0;

    Serial.println("\n--- MODO STANDBY ATIVO ---");
}

void AcordarSistema()
{
    _lastActivity = millis();

    if (_estadoAtual == STANDBY)
    {
        _estadoAtual = (_alturaUsuario == 0.0) ? AGUARDANDO_ALTURA : AGUARDANDO_PESO;
    }

    Serial.println("--- SISTEMA RETOMADO ---");
    _lcd.clear();

    if (_estadoAtual == AGUARDANDO_ALTURA)
    {
        _lcd.print("Altura (m):");
        _lcd.setCursor(0, 1);
        _lcd.print(_entradaTexto);

        Serial.println("Aguardando Altura (m)...");
    }
    else
    {
        _lcd.print("Suba na balanca");
        _lcd.setCursor(0, 1);
        _lcd.print("e aguarde...");

        Serial.println("Aguardando Peso na Balanca...");
    }
}

void ExecutarStandby()
{
    if (millis() - _timerStandby > 3000)
    {
        _timerStandby = millis();

        float t = _dht.readTemperature();
        float h = _dht.readHumidity();

        _lcd.clear();

        if (!isnan(t) && !isnan(h))
        {
            _lcd.setCursor(0, 0);
            _lcd.print("Temp: ");
            _lcd.print(t, 1);
            _lcd.print((char)223);
            _lcd.print("C");
            
            _lcd.setCursor(0, 1);
            _lcd.print("Umid: ");
            _lcd.print(h, 1);
            _lcd.print("%");

            Serial.print("[STANDBY] Temp: ");
            Serial.print(t, 1);
            Serial.print(" C | Umid: ");
            Serial.print(h, 1);
            Serial.println(" %");
        }
        else
        {
            _lcd.print("Sensor DHT");
            _lcd.setCursor(0, 1);
            _lcd.print("Falha de leitura");

            Serial.println("[STANDBY] Falha no sensor DHT.");
        }
    }
}

void ProcessarAltura(char tecla)
{
    if (!tecla)
    {
        return;
    }

    Serial.print("[TECLADO] Tecla: '");
    Serial.print(tecla);
    Serial.println("'");

    if (isDigit(tecla) || tecla == 'P')
    {
        char c = (tecla == 'P') ? '.' : tecla;

        if (_cursorPos < (int)_entradaTexto.length())
        {
            _entradaTexto[_cursorPos] = c;
        }
        else
        {
            _entradaTexto += c;
        }

        _cursorPos++;

        Serial.print("[TECLADO] Entrada atual: ");
        Serial.println(_entradaTexto);

        AtualizarDisplayAltura();
    }
    else if (tecla == 'E')
    {
        if (_cursorPos > 0)
        {
            _entradaTexto.remove(_cursorPos - 1, 1);
            _cursorPos--;

            Serial.print("[TECLADO] Apagou. Entrada: ");
            Serial.println(_entradaTexto);

            AtualizarDisplayAltura();
        }
    }
    else if (tecla == 'D' && _cursorPos < (int)_entradaTexto.length())
    {
        _cursorPos++;
        _lcd.setCursor(_cursorPos, 1);
    }
    else if (tecla == 'L' && _cursorPos > 0)
    {
        _cursorPos--;
        _lcd.setCursor(_cursorPos, 1);
    }
    else if (tecla == 'R')
    {
        Serial.print("[TECLADO] ENTER. Valor: ");
        Serial.println(_entradaTexto);

        ValidarAltura();
    }
}

void ValidarAltura()
{
    float valor = _entradaTexto.toFloat();

    if (valor >= 0.50 && valor <= 2.50)
    {
        _alturaUsuario = valor;
        _estadoAtual = AGUARDANDO_PESO;
        _entradaTexto = "";
        _cursorPos = 0;

        _lcd.clear();
        _lcd.print("Altura: ");
        _lcd.print(_alturaUsuario, 2);
        _lcd.print("m");

        delay(1200);

        _lcd.clear();
        _lcd.print("Suba na balanca");
        _lcd.setCursor(0, 1);
        _lcd.print("e aguarde...");

        Serial.println();
        Serial.println("-----------------------------");
        Serial.print(" Altura registrada: ");
        Serial.print(_alturaUsuario, 2);
        Serial.println(" m  ");
        Serial.println("-----------------------------");
        Serial.println(" Suba na balanca e           ");
        Serial.println(" fique imóvel por 5 segundos ");
        Serial.println("-----------------------------");
    }
    else
    {
        AnimacaoErro();

        _lcd.clear();
        _lcd.print("Altura (m):");
        _entradaTexto = "";
        _cursorPos = 0;

        Serial.println();
        Serial.println("==============================");
        Serial.println("    ERRO: ALTURA INVALIDA     ");
        Serial.println("  Faixa aceita: 0.50 - 2.50   ");
        Serial.print("  Valor enviado: ");
        Serial.print(_entradaTexto);
        Serial.println("          ");
        Serial.println("==============================");
    }
}

void ProcessarPesagem(float pesoAtual)
{
    if (pesoAtual > 2.0)
    {
        if (!_contandoPesagem)
        {
            _contandoPesagem = true;
            _tempoInicioPesagem = millis();
            _timerPesoDisplay = 0;

            Serial.println("[BALANCA] Peso detectado! Estabilizando...");
        }

        if (millis() - _timerPesoDisplay > 1000)
        {
            _timerPesoDisplay = millis();
            int restante = 5 - (int)((millis() - _tempoInicioPesagem) / 1000);

            _lcd.clear();
            _lcd.print("Peso: ");
            _lcd.print(pesoAtual, 1);
            _lcd.print(" kg");
            
            _lcd.setCursor(0, 1);
            _lcd.print("Aguarde: ");
            _lcd.print(restante);
            _lcd.print("s...");

            Serial.print("[BALANCA] Lendo: ");
            Serial.print(pesoAtual, 1);
            Serial.print(" kg | Confirmando em: ");
            Serial.print(restante);
            Serial.println("s");
        }

        if (millis() - _tempoInicioPesagem >= TEMPO_PESAGEM)
        {
            _pesoUsuario = pesoAtual;

            AnimacaoCalculando();
            FinalizarCalculo();
        }
    }
    else
    {
        if (_contandoPesagem)
        {
            _contandoPesagem = false;

            _lcd.clear();
            _lcd.print("Suba na balanca");
            _lcd.setCursor(0, 1);
            _lcd.print("e aguarde...");

            Serial.println("[BALANCA] Processo interrompido. Mantenha-se na balanca!");
        }
    }
}

void FinalizarCalculo()
{
    float imc = _pesoUsuario / (_alturaUsuario * _alturaUsuario);
    String classificacao;
    String classificacaoLcd;

    if (imc < 18.5)
    {
        classificacao = "Abaixo do peso";
        classificacaoLcd = "Abaixo do peso";
    }
    else if (imc < 25.0)
    {
        classificacao = "Peso Normal";
        classificacaoLcd = "Normal";
    }
    else if (imc < 30.0)
    {
        classificacao = "Sobrepeso";
        classificacaoLcd = "Sobrepeso";
    }
    else if (imc < 35.0)
    {
        classificacao = "Obesidade I";
        classificacaoLcd = "Obesid. I";
    }
    else if (imc < 40.0)
    {
        classificacao = "Obesidade II";
        classificacaoLcd = "Obesid. II";
    }
    else
    {
        classificacao = "Obesidade III";
        classificacaoLcd = "Obesid. III";
    }

    _lcd.clear();
    _lcd.print("IMC: ");
    _lcd.print(imc, 1);
    
    _lcd.setCursor(0, 1);
    _lcd.print(classificacaoLcd);

    Serial.println();
    Serial.println("==============================");
    Serial.println("      RELATORIO BIOMETRICO    ");
    Serial.println("==============================");
    Serial.print("  Altura : ");
    Serial.print(_alturaUsuario, 2);
    Serial.println(" m              ");
    Serial.print("  Peso   : ");
    Serial.print(_pesoUsuario, 1);
    Serial.println(" kg             ");
    Serial.print("  IMC    : ");
    Serial.print(imc, 1);
    Serial.println("                ");
    Serial.println("------------------------------");
    Serial.print("  Status : ");
    Serial.print(classificacao);
    Serial.println("          ");
    Serial.println("==============================");

    delay(5000);

    _scale.tare();
    _alturaUsuario = 0.0;

    ReiniciarSistema();
}

void AtualizarDisplayAltura()
{
    _lcd.setCursor(0, 1);
    _lcd.print("                ");
    _lcd.setCursor(0, 1);
    _lcd.print(_entradaTexto);
    _lcd.setCursor(_cursorPos, 1);
}

void AnimacaoInicial()
{
    _lcd.clear();
    _lcd.print("Iniciando");

    for (int i = 0; i < 3; i++)
    {
        _lcd.print(".");
        delay(300);
    }

    _lcd.clear();
    _lcd.print("Pronto!");

    delay(600);
}

void AnimacaoCalculando()
{
    _lcd.clear();
    _lcd.print("Calculando");

    for (int i = 0; i < 6; i++)
    {
        _lcd.print(".");
        delay(250);
    }
}

void AnimacaoErro()
{
    _lcd.clear();
    _lcd.print("Entrada invalida");

    for (int i = 0; i < 3; i++)
    {
        _lcd.noDisplay();
        delay(200);
        _lcd.display();
        delay(200);
    }
}

void PrintSerialBanner()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("  CALCULADORA IMC + BALANCA   ");
    Serial.println("==============================");
    Serial.println("  ALTURA: teclado ou serial   ");
    Serial.println("  PESO:   balanca HX711       ");
    Serial.println("------------------------------");
    Serial.println("  Comandos Serial:            ");
    Serial.println("  Altura: envie ex. 1.75      ");
    Serial.println("  C = limpar entrada atual    ");
    Serial.println("==============================");
}