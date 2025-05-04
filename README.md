# ECOP11A-LoRaWAN

**Sensores LoRaWAN – Projeto em C**

## Proposta Geral

Este projeto tem como objetivo o desenvolvimento, em linguagem C, de um sistema completo de comunicação LoRaWAN composto por dois emuladores: um **dispositivo (end device)** e um **gateway**. A comunicação será simulada com a plataforma **The Things Network (TTN)**, implementando funcionalidades reais do protocolo LoRaWAN, sem a necessidade de hardware físico.  

A solução permite testes locais e remotos, além de integração com plataformas de visualização de dados como o **TagoIO**.

---

## Tecnologias e Ferramentas

- **Linguagem:** C (padrão C99 ou superior)  
- **Ambiente de Desenvolvimento:**  
  - Visual Studio Code (VSCode)  
  - PlatformIO (extensão para VSCode)  
  - ESP-IDF *(para eventual uso com hardware real)*  
- **Hardware de Referência (opcional):** WiFi LoRa 32 V2 ou V3  
- **Plataformas de Integração:**  
  - The Things Network (TTN)  
  - TagoIO

---

## Bibliotecas Recomendadas

- `libloragw` (Semtech): emulação de gateway  
- `liblorawan`: implementação da pilha LoRaWAN  
- `cJSON`: leitura de arquivos de configuração em JSON  
- `mbed TLS`: criptografia AES (requerida para OTAA e ABP)

---

## Funcionalidades Esperadas

### Emulador de Dispositivo LoRaWAN

- Geração de mensagens com cabeçalhos e payload criptografados  
- Simulação de uplinks com parâmetros configuráveis:
  - `DevEUI`, `AppEUI`, `AppKey`, `DevAddr`, `NwkSKey`, `AppSKey`  
- Suporte aos modos de ativação:
  - OTAA (Over-The-Air Activation)  
  - ABP (Activation By Personalization)  
- Registro detalhado das operações por meio de logs

### Emulador de Gateway LoRaWAN

- Recebimento de pacotes simulados via UDP ou TCP  
- Encapsulamento e encaminhamento usando o protocolo **Semtech UDP Packet Forwarder**  
- Suporte opcional ao protocolo **Basic Station**  
- Logs operacionais e gerenciamento de múltiplos dispositivos simultaneamente

---

## Configurações

- Parâmetros configuráveis por arquivos `.ini` ou `.json`  
- Suporte à simulação de múltiplos dispositivos em paralelo

---

## Referências

- [Especificações Oficiais LoRaWAN](https://lora-alliance.org/resource-hub/lorawanr-specification-v10)  
- [Documentação TTN](https://www.thethingsnetwork.org/docs/)  
- [Semtech UDP Protocol](https://lora-developers.semtech.com/documentation/)  
- [Basic Station Protocol](https://github.com/lorabasics/basicstation)  
- Projetos de referência:
  - [ttn-gateway-bridge (Go)](https://github.com/TheThingsNetwork/ttn-gateway-bridge)  
  - [ChirpStack (LoRaWAN Network Server em Go)](https://www.chirpstack.io)

---

## Etapas do Projeto

| Etapa | Descrição | Status | Entrega |
|-------|-----------|--------|---------|
| 1 | Simulador simples com envio de 2 variáveis ao TagoIO via TTN (simulado) | ✅ Concluída | 01/04/2025 |
| 2 | Emulador de dispositivo com montagem e envio de pacotes LoRaWAN criptografados, configurável por JSON | ✅ Concluída | 21/04/2025 |
| 3 | Emulador de gateway com recepção e reencaminhamento de pacotes para TTN via Semtech UDP | ✅ Concluída | 28/04/2025 |
| 4 | Integração dos módulos: comunicação local entre dispositivo e gateway, com validação de pacotes | ✅ Concluída | 05/05/2025 |
| 5 | Comunicação com a TTN: envio real de pacotes, recebimento de downlinks, análise completa | 🔄 Em andamento | 12/05/2025 |

---

