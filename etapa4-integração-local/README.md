# Etapa 4 - Integração Local

Este projeto implementa a comunicação entre dispositivos simulados LoRaWAN e um gateway local, utilizando o protocolo **Semtech UDP Packet Forwarder**. A etapa 4 foca na integração local, onde pacotes são enviados pelos dispositivos simulados, processados pelo gateway e respostas (ACKs) são retornadas.

## Estrutura do Projeto

etapa4-integração-local/ 
├── Makefile # Arquivo para compilar o projeto 
├── README.md # Documentação do projeto 
├── config/
│   │
│   └── Config.json # Configurações dos dispositivos simulados 
│
├── include/ 
│   │
│   ├── aes_cmac.h # Header para funções de criptografia AES-CMAC 
│   ├── Config.h # Header para manipulação do Config.json 
│   └── lorawan.h # Header para construção de pacotes LoRaWAN 
│
├── src/ 
│   │
│   ├── aes_cmac.c # Implementação de funções AES-CMAC 
│   ├── Config.c # Manipulação do Config.json 
│   ├── Gateway.c # Implementação do gateway local 
│   ├── lorawan.c # Construção de pacotes LoRaWAN 
│   └── Main.c # Simulador de dispositivos LoRaWAN
│
│
├── bin/
│   ├──Main.exe //Se Executado o Make
│   ├──Gateway.exe //Se Executado o Make
│
└── Fim

## Pré-requisitos

Certifique-se de que os seguintes itens estão instalados no sistema:

- **Compilador GCC** (para compilar o código C)
- **Biblioteca mbedtls** (para criptografia AES e CMAC)
- **Biblioteca cJSON** (para manipulação de JSON)

### Instalação das dependências no Linux

Execute os comandos abaixo para instalar as dependências:

bash:

sudo apt update
sudo apt install -y gcc make libmbedtls-dev libcjson-dev

## Pré-requisitos

Arquivo Config.json
O arquivo Config.json contém as configurações dos dispositivos simulados. Aqui está um exemplo de configuração:

json:
{
    "devices": [
        {
            "device_id": "lorawan-simu1",
            "application_id": "lorawan-unifei",
            "deveui": "70B3D57ED007035A",
            "devaddr": "260CECBA",
            "nwkskey": "A8811AA4AF7702185FC3ABD07FF3C3CD",
            "appskey": "ED3FD17362220C5B95C867AC42C2CB29",
            "fcnt": 0,
            "fport": 10,
            "payload": [55, 87]
        },
        {
            "device_id": "lorawan-simu2",
            "application_id": "lorawan-unifei",
            "deveui": "70B3D57ED0070507",
            "devaddr": "260CF6CF",
            "nwkskey": "388F49FB1FD9507358CFD6B102D3A627",
            "appskey": "F125B998CB5E21F5D083B9A2FB8B0F66",
            "fcnt": 0,
            "fport": 15,
            "payload": [64, 82]
        }
    ]
}

Onde:

device_id: Identificador do dispositivo.
application_id: Identificador da aplicação.
deveui: Identificador único do dispositivo.
devaddr: Endereço do dispositivo.
nwkskey: Chave de rede para autenticação
appskey: Chave de aplicação para criptografia.
fcnt: Contador de frames.
fport: Porta de comunicação.
payload: Dados a serem enviados pelo dispositivo.

## Compilação

Execute o terminal na pasta e em seguida: 

make

## Execução:

Execute o Gateway primeiro: 

./bin/Gateway

Em outro terminal na pasta, execute o Main:

./bin/Main

## Funcionamento: 

O simulador irá:

1. Carregar as configurações do arquivo Config.json.
2. Montar pacotes LoRaWAN para cada dispositivo configurado.
3. Enviar os pacotes para o gateway.
4. Aguardar o ACK do gateway antes de continuar.
5. Caso não receba, envia o pacote novamente.
6. Caso receba corretamente, envia o proximo pacote.
7. Após a finalização de todos os pacotes, ambos se encerram.

## Depuração

Leia o terminal

## Limpeza

Execute no terminal dentro da pasta: 

make clean

## Final

Funciona na minha maquina, se não funciona na sua, problema seu. (Brincadeira, só baixar as bibliotecas que funciona)

