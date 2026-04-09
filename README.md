# Sistema IoT: Balança de Precisão com Cálculo de IMC (ESP32)

Este projeto consiste em uma balança inteligente desenvolvida com tecnologias de IoT, capaz de realizar a medição de peso e o cálculo automático do Índice de Massa Corporal (IMC) em tempo real. O sistema conta com uma interface interativa via teclado e display LCD, além de um modo de economia de energia com monitoramento de temperatura.

## 🚀 Tecnologias e Linguagens
- **Linguagem:** C++ (Arduino Framework)
- **Plataforma:** ESP32 (DevKit V1)
- **Hardware Interfacing:** HX711 (ADC 24-bit), DHT22, Teclado Matricial 4x4.

## 🛠️ Componentes de Hardware
O sistema utiliza uma arquitetura robusta para garantir precisão nas leituras:

- **Processamento:** Microcontrolador ESP32.
- **Sensoriamento:**
  - **Célula de Carga:** Medição de peso com interface via módulo HX711.
  - **DHT22:** Sensor de temperatura e umidade (utilizado para o modo standby/espera).
- **Interface:**
  - **LCD 16x2:** Exibição de menus, peso, IMC e temperatura.
  - **Teclado Matricial 4x4:** Entrada de dados (altura do usuário).
- **Alimentação:**
  - Transformador (Trafo) 9V 250mA.
  - Regulador de tensão 7805 (Saída estável de 5V).

## 📌 Mapeamento de Pinos (ESP32)
| Componente | Função | Pino ESP32 |
| :--- | :--- | :--- |
| **LCD 16x2** | RS, EN, D4, D5, D6, D7 | 2, 15, 4, 5, 18, 19 |
| **HX711** | DOUT, SCK | 32, 12 |
| **DHT22** | Dados | 33 |
| **Teclado (Linhas)** | L1, L2, L3, L4 | 23, 22, 21, 13 |
| **Teclado (Colunas)**| C1, C2, C3, C4 | 25, 26, 27, 14 |

## 🎮 Operação do Sistema
O sistema opera em três estados principais:

1.  **Entrada de Altura:** O usuário digita sua altura no teclado.
    *   **Teclas Numéricas:** Insere os valores.
    *   **Tecla 'P':** Insere o ponto decimal (ex: 1.75).
    *   **Tecla 'E':** Apaga a entrada atual (Backspace/Clear).
    *   **Tecla 'R':** Confirma a altura e inicia a pesagem.
2.  **Pesagem e Travamento:** O sistema aguarda o peso estabilizar por 3 segundos. Após o travamento, calcula o IMC e exibe a classificação (Magreza, Normal, Sobrepeso, etc.).
3.  **Modo Espera (Screensaver):** Se não houver atividade na balança ou no teclado por 10 segundos, o LCD exibe a temperatura ambiente atual capturada pelo sensor DHT22.

## 🔋 Eficiência Energética e Estabilidade
- O ESP32 consome aproximadamente **150mA** em operação.
- A estabilidade da fonte de alimentação (regulador 7805 + capacitores) é crítica para evitar ruídos no ADC HX711, garantindo que as leituras de peso não flutuem.

---
*Este projeto faz parte de um portfólio de sistemas embarcados focado em precisão e usabilidade.*
