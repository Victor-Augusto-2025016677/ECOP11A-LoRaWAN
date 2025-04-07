# ECOP11A-LoRaWAN

Sensores LoRaWAN – Projeto em C (ECOP11A)

Proposta Geral
Desenvolver, em linguagem C, um sistema completo de comunicação LoRaWAN composto por dois emuladores: um dispositivo (end device) e um gateway. O objetivo é simular a comunicação com a plataforma The Things Network (TTN), com suporte a funcionalidades reais do protocolo, sem a necessidade de hardware físico.

Esse sistema permitirá testes locais e remotos, além da integração com plataformas de visualização como o TagoIO.

Tecnologias e Ferramentas
Linguagem: C (padrão C99 ou superior)

Ambiente de Desenvolvimento:

Visual Studio Code (VSCode)

PlatformIO (extensão do VSCode)

ESP-IDF (caso utilize hardware real no futuro)

Hardware de Referência (opcional): WiFi LoRa 32 V2 ou V3

Plataformas:

The Things Network (TTN)

TagoIO (visualização de dados)

Bibliotecas recomendadas
libloragw (Semtech) – para emulação de gateway

liblorawan – stack LoRaWAN em C

cJSON – leitura de arquivos de configuração JSON

mbed TLS – criptografia AES (necessária para OTAA e ABP)

Funcionalidades Esperadas
Emulador de Dispositivo LoRaWAN
Geração de mensagens LoRaWAN com payload e cabeçalhos criptografados.

Simulação de uplinks (envio de dados) com parâmetros configuráveis:

DevEUI, AppEUI, AppKey, DevAddr, NwkSKey, AppSKey.

Suporte a modos de ativação: OTAA ou ABP.

Logs detalhados das operações.

Emulador de Gateway LoRaWAN
Recebimento de pacotes simulados via UDP ou TCP.

Encapsulamento usando o protocolo Semtech UDP Packet Forwarder.

Encaminhamento de pacotes para o TTN por meio do protocolo:

Semtech UDP

(opcionalmente) Basic Station

Logs de operação e gerenciamento de múltiplos dispositivos.

Configurações
Parâmetros do sistema configuráveis via arquivos .ini ou .json.

Suporte à simulação de múltiplos dispositivos em paralelo.

Referências e Recursos Úteis
LoRaWAN Especificações Oficiais
https://lora-alliance.org/resource-hub/lorawanr-specification-v10

Documentação TTN
https://www.thethingsnetwork.org/docs/

Protocolos de Gateway

Semtech UDP: https://lora-developers.semtech.com/documentation/

Basic Station: https://github.com/lorabasics/basicstation

Projetos de referência

ttn-gateway-bridge (Go)

ChirpStack (LoRaWAN Network Server em Go)

Etapas e Entregas:

✅ Etapa 1 – Simulador simples com envio ao TagoIO
Criar um código em C que envie 2 variáveis para o TagoIO via TTN (simulado).

Entrega: 1 de abril de 2025

Etapa 2 – Emulador de Dispositivo LoRaWAN
Implementação da montagem e envio de pacotes LoRaWAN com criptografia.
Simulação de uplinks com configuração por JSON.

Entrega: 21 de abril de 2025

Etapa 3 – Emulador de Gateway (UDP Forwarder)
Recebimento de pacotes e reencaminhamento para o TTN usando o protocolo Semtech UDP.
Logs e gerenciamento básico de pacotes.

Entrega: 28 de abril de 2025

Etapa 4 – Integração dos Módulos
Comunicação entre o emulador de dispositivo e o gateway localmente.
Testes e validação de pacotes.

Entrega: 5 de maio de 2025

Etapa 5 – Comunicação com TTN
Encaminhamento de pacotes reais para a rede pública TTN.
Recebimento de downlinks e análise completa da comunicação.

Entrega: 12 de maio de 2025