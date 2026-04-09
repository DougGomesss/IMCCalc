#include <LiquidCrystal.h>
#include <Keypad.h>
#include "HX711.h"
#include "DHT.h" 

// ==========================================
// 1. CONFIGURAÇÕES DE HARDWARE
// ==========================================

LiquidCrystal lcd(2, 15, 4, 5, 18, 19);

const int LOADCELL_DOUT_PIN = 32;
const int LOADCELL_SCK_PIN = 12;

HX711 scale;
// SEU FATOR CALIBRADO
float calibration_factor = 28955.0; 

const byte LINHAS = 4;
const byte COLUNAS = 4;
char teclas[LINHAS][COLUNAS] = {
  {'E', '7', '4', '1'},
  {'0', '8', '5', '2'},
  {'R', '9', '6', '3'}, 
  {'L', 'D', 'P', ' '}  
};
byte pinosLinhas[LINHAS] = {23, 22, 21, 13};
byte pinosColunas[COLUNAS] = {25, 26, 27, 14};
Keypad teclado = Keypad(makeKeymap(teclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

#define DHTPIN 33
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ==========================================
// 2. VARIÁVEIS DO SISTEMA
// ==========================================
bool modoAltura = true; 
String entradaAltura = "";
float alturaUsuario = 0;
float pesoAtual = 0;
float pesoSuavizado = 0;

// Variáveis de Estabilidade
unsigned long timerEstabilidade = 0;
float ultimoPesoLido = 0;
bool contandoTempo = false;
const int TEMPO_PARA_TRAVAR = 3000; 

// Variáveis de Inatividade (Screensaver)
unsigned long lastActivityTime = 0;
const long TEMPO_DESCANSO = 10000; // 10 segundos para mostrar temperatura
bool emDescanso = false;

// ==========================================
// 3. SETUP
// ==========================================
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  dht.begin();

  lcd.clear();
  lcd.print("Iniciando...");
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.power_up(); 
  
  lcd.setCursor(0, 1);
  lcd.print("Zerando Tara...");
  delay(1000);
  scale.tare(); 
  
  resetarParaAltura();
}

// ==========================================
// 4. LOOP PRINCIPAL
// ==========================================
void loop() {
  // Lê a balança sempre (para saber se alguém subiu e acordar o sistema)
  bool balancaAtiva = verificarAtividadeBalanca();
  char tecla = teclado.getKey();

  // --- GERENCIAMENTO DE INATIVIDADE (Screensaver) ---
  if (tecla || balancaAtiva) {
    lastActivityTime = millis(); // Reseta o timer se houver ação
    if (emDescanso) {
      emDescanso = false;
      // Se acordou, volta a mostrar o que estava antes
      if (modoAltura) atualizarDisplayAltura();
    }
  }

  // Verifica se passou 10s sem fazer nada E a balança está vazia
  if (!emDescanso && (millis() - lastActivityTime > TEMPO_DESCANSO) && !balancaAtiva) {
    mostrarTelaDescanso();
    return; // Pula o resto do loop enquanto estiver descansando
  }

  if (emDescanso) {
    // Se está em descanso, só fica atualizando a temperatura a cada 2s
    static unsigned long timerTemp = 0;
    if (millis() - timerTemp > 2000) {
      mostrarTelaDescanso();
      timerTemp = millis();
    }
    return;
  }

  // --- LÓGICA NORMAL DO SISTEMA ---
  
  if (modoAltura) {
    if (tecla) {
      if (isDigit(tecla) || tecla == 'P') {
        char c = (tecla == 'P') ? '.' : tecla;
        if (entradaAltura.length() < 5) entradaAltura += c;
        atualizarDisplayAltura();
      }
      else if (tecla == 'E') { 
        entradaAltura = "";
        atualizarDisplayAltura();
      }
      else if (tecla == 'R') { 
        confirmarAltura();
      }
    }
  }
  else {
    // Modo Pesagem
    lerPesoEProcessar();
    
    if (tecla == 'E') {
      resetarParaAltura();
    }
  }
  
  delay(50); 
}

// ==========================================
// 5. FUNÇÕES AUXILIARES
// ==========================================

// Lê a balança e retorna TRUE se tiver peso (alguém em cima)
bool verificarAtividadeBalanca() {
  if (scale.is_ready()) {
    float leitura = scale.get_units(1);
    
    // Filtro rápido apenas para checar atividade
    pesoSuavizado = (pesoSuavizado * 0.80) + (leitura * 0.20);
    if (abs(pesoSuavizado) < 0.2) pesoSuavizado = 0.0;
    pesoAtual = pesoSuavizado;
    
    // Se tiver mais de 0.5kg, considera atividade (acorda o sistema)
    return (pesoAtual > 0.5);
  }
  return false;
}

void mostrarTelaDescanso() {
  if (!emDescanso) lcd.clear();
  emDescanso = true;
  
  float t = dht.readTemperature();
  
  lcd.setCursor(0, 0);
  lcd.print("   EM ESPERA    ");
  lcd.setCursor(0, 1);
  lcd.print("Temp: "); 
  if (!isnan(t)) {
    lcd.print(t, 1); lcd.print(" C     ");
  } else {
    lcd.print("-- C     ");
  }
}

void atualizarDisplayAltura() {
  lcd.clear();
  lcd.print("Altura (m):");
  lcd.setCursor(0, 1);
  lcd.print(entradaAltura);
  lcd.blink(); 
}

void confirmarAltura() {
  if (entradaAltura.length() > 0) {
    alturaUsuario = entradaAltura.toFloat();
    if (alturaUsuario > 0.5 && alturaUsuario < 2.5) {
      modoAltura = false;
      lcd.noBlink();
      lcd.clear();
      lcd.print("Ok! Suba agora.");
      delay(1500);
      lcd.clear();
      lastActivityTime = millis(); // Reseta timer
    } else {
      lcd.clear(); lcd.print("Altura Invalida"); delay(1000); atualizarDisplayAltura();
    }
  }
}

void resetarParaAltura() {
  modoAltura = true;
  entradaAltura = "";
  pesoSuavizado = 0;
  contandoTempo = false;
  emDescanso = false;
  lastActivityTime = millis();
  atualizarDisplayAltura();
}

void lerPesoEProcessar() {
  // ATENÇÃO: A leitura do peso já foi feita na função 'verificarAtividadeBalanca'
  // no início do loop. Usamos a variável global 'pesoAtual' que já está atualizada.

  // --- ATUALIZA O LCD ---
  static unsigned long timerLCD = 0;
  if (millis() - timerLCD > 200) {
      lcd.setCursor(0, 0);
      lcd.print("Lendo Peso...");
      
      lcd.setCursor(0, 1);
      if (pesoAtual > 0) {
          lcd.print(pesoAtual, 1); 
          lcd.print(" kg      "); 
      } else {
          lcd.print("0.0 kg      ");
      }
      
      if (contandoTempo) {
        int seg = 3 - ((millis() - timerEstabilidade) / 1000);
        lcd.setCursor(13, 1);
        lcd.print(seg); lcd.print("s ");
      } else {
        lcd.setCursor(13, 1);
        lcd.print("   ");
      }
      timerLCD = millis();
  }

  // --- LÓGICA DE TRAVAMENTO ---
  if (pesoAtual > 2.0) { 
    if (abs(pesoAtual - ultimoPesoLido) < 0.1) { 
      if (!contandoTempo) {
        contandoTempo = true;
        timerEstabilidade = millis(); 
      } else {
        if (millis() - timerEstabilidade >= TEMPO_PARA_TRAVAR) {
          finalizarProcesso(); 
        }
      }
    } else {
      contandoTempo = false; 
    }
    ultimoPesoLido = pesoAtual;
  } else {
    contandoTempo = false; 
  }
}

void finalizarProcesso() {
  // 1. TRAVAMENTO
  lcd.clear();
  lcd.print(">> TRAVADO <<");
  lcd.setCursor(0, 1);
  lcd.print(pesoAtual, 1); lcd.print(" kg");
  delay(2000); // Mostra o peso travado por 2s
  
  // 2. CÁLCULO
  float imc = pesoAtual / (alturaUsuario * alturaUsuario);
  
  lcd.clear();
  lcd.print("IMC: "); lcd.print(imc, 1);
  lcd.setCursor(0, 1);
  
  if (imc < 18.5) lcd.print("Magreza");
  else if (imc < 25) lcd.print("Normal");
  else if (imc < 30) lcd.print("Sobrepeso");
  else if (imc < 35) lcd.print("Obesidade I");
  else if (imc < 40) lcd.print("Obesidade II");
  else lcd.print("Obesidade III");
  
  // 3. ESPERA 5 SEGUNDOS E REINICIA
  delay(5000); 
  
  resetarParaAltura(); 
}