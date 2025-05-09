#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cjson/cJSON.h>

// Gateway EUI fictício
unsigned char gateway_eui[8] = { 0x75, 0x6E, 0x69, 0x66, 0x65, 0x69, 0x12, 0x09 };

#define TTN_ADDRESS "54.172.194.55"  // IP fixo do TTN router.us.thethings.network
#define TTN_PORT 1700
#define JSON_DIR "./out"

void send_packet(const char *json_str, int sockfd, struct sockaddr_in *ttn_addr) {
    unsigned char buffer[2048];
    int len = 0;

    // Cabeçalho Semtech
    buffer[0] = 0x02; // protocol version
    buffer[1] = 0x00; // token high
    buffer[2] = 0x00; // token low
    buffer[3] = 0x00; // PUSH_DATA
    memcpy(&buffer[4], gateway_eui, 8); // EUI do gateway
    len = 12;

    // Copia o JSON após o cabeçalho
    int json_len = strlen(json_str);
    if (json_len + len >= sizeof(buffer)) {
        fprintf(stderr, "JSON muito grande!\n");
        return;
    }

    memcpy(&buffer[len], json_str, json_len);
    len += json_len;

    sendto(sockfd, buffer, len, 0, (struct sockaddr *)ttn_addr, sizeof(*ttn_addr));
}

char *read_file_to_string(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    fread(content, 1, size, file);
    content[size] = '\0';

    fclose(file);
    return content;
}

int main() {
    DIR *dir;
    struct dirent *entry;

    // Criação do socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // Endereço do TTN
    struct sockaddr_in ttn_addr;
    memset(&ttn_addr, 0, sizeof(ttn_addr));
    ttn_addr.sin_family = AF_INET;
    ttn_addr.sin_port = htons(TTN_PORT);
    inet_pton(AF_INET, TTN_ADDRESS, &ttn_addr.sin_addr);

    dir = opendir(JSON_DIR);
    if (!dir) {
        perror("opendir");
        close(sockfd);
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", JSON_DIR, entry->d_name);

        struct stat st;
        if (stat(path, &st) == -1 || !S_ISREG(st.st_mode)) continue;

        char *json_data = read_file_to_string(path);
        if (!json_data) continue;

        send_packet(json_data, sockfd, &ttn_addr);
        printf("Enviado: %s\n", entry->d_name);
        free(json_data);
    }

    closedir(dir);
    close(sockfd);
    return 0;
}