# ECOP11A-LoRaWAN
 
Prof Carlos soltou essa bomba, agora eu vou me virar pra fazer:

Sensores LoRaWAN

Proposta
Desenvolva um sistema completo para um sistema com comunicação LoRaWAN em linguagem C.


Entrega da Atividade
Criar um repositório no GitHub com o seguinte formato:

https://github.com/<nickname_aluno>/ECOP11A-LoRaWAN
O repositório deverá conter os seguintes itens:

README.md
Git Ignore para C
Licença de uso
Durante as etapas o repositório deverá ser atualizado com as informações solicitadas.

Uma pasta deverá ser criada no repositório para cada atividade entregue.

Ferramentas
Visual Studio Code (VSCode)
PlatformIO: Extensão do VSCode
ESP-IDF: plataforma de programação da empresa Espressif
WiFi LoRa 32: V2 e V3
The Thing Network (TTN)
TagoIO
Etapas
Simulador
Faça um código em C para enviar 2 variáveis para o TagoIO, simulando a coleta de dados pela interface do The Thing Network.

Entrega: 1 de abril de 2025.


Desenvolvimento de um Emulador de Gateway e Dispositivo LoRaWAN em C para Integração com The Things Network (TTN)

Desenvolver, em linguagem C, dois emuladores independentes:

Um emulador de gateway LoRaWAN compatível com os protocolos utilizados pela plataforma The Things Network (TTN).
Um emulador de dispositivo LoRaWAN (end device) capaz de simular o envio de uplinks e o recebimento de downlinks através do gateway.
Esses emuladores devem permitir testes locais e remotos da comunicação entre dispositivos e a rede TTN, sem a necessidade de hardware físico.
Funcionalidades Esperadas
Emulador de Dispositivo:

Simulação de pacotes LoRaWAN (uplinks) com parâmetros configuráveis: DevEUI, AppEUI, AppKey, etc.
Suporte a ABP ou OTAA.
Geração de mensagens LoRaWAN (payload + headers) criptografadas corretamente.
Emulador de Gateway:

Recebimento de pacotes via UDP ou TCP simulando rádio LoRa.
Encapsulamento dos pacotes no protocolo Semtech UDP Packet Forwarder.
Encaminhamento para TTN através do Basic Station ou Semtech UDP protocol.
Geral:

Logs claros de envio e recebimento.
Configuração por arquivo .ini ou .json.
Possibilidade de simular múltiplos dispositivos.
Tecnologias e Ferramentas
Linguagem: C (padrão C99 ou superior)
Bibliotecas recomendadas:
libloragw (Semtech)
liblorawan (LoRaWAN stack em C)
cJSON para leitura de arquivos de configuração JSON.
mbed TLS para criptografia AES (LoRaWAN).

Referências e Recursos Úteis
LoRaWAN Especificações Oficiais:
https://lora-alliance.org/resource-hub/lorawanr-specification-v10
The Things Network Documentação:
https://www.thethingsnetwork.org/docs/
Protocolos de Gateway TTN:
Semtech UDP Protocol: https://lora-developers.semtech.com/documentation/
Basic Station: https://github.com/lorabasics/basicstation
Repositórios similares de referência:
ttn-gateway-bridge (Go)
LoRaWAN Network Server (Go - ChirpStack)
Entregas de material no repositório:
Implementação do emulador de dispositivo LoRaWAN (uplink simples)
21 de abril de 2025
Implementação do emulador de gateway (UDP Forwarder básico)
28 de abril de 2025
Integração dos dois módulos e testes locais
5 de maio de 2025
Comunicação com TTN e testes em rede pública
12 de maio de 2025
