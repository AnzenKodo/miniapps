#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>
#include <wchar.h>

#define PROJECT_NAME "cymbols"
#define MAX_STRING_SIZE 200
#define MAX_BUFFER_SIZE 8192
#define MAX_LINE_LENGTH 450

#define VERSION "0.1"

bool is_file_exist(const char *path) {
    bool result;

    if (access(path, F_OK) == 0) {
        result = true;
    } else {
        result = false;
    }

    return result;
}

bool str_cmp(char *str1, char *str2) {
    if (strcmp(str1, str2) == 0) return true;
    else return false;
}

void fetch(const char *hostname, const char *path, const char *filename) {
    char buffer[MAX_BUFFER_SIZE];
    struct sockaddr_in server_addr;

    // Step 1: Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error: socket creation failed.");
    }

    // Step 2: Resolve hostname to IP address
    struct hostent *he = gethostbyname(hostname);
    if (he == NULL) {
        perror("Error: gethostbyname failed.");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    memcpy(&server_addr.sin_addr.s_addr, he->h_addr_list[0], he->h_length);

    // Step 3: Connect to server
    if (connect(
        sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)
    ) < 0) {
        perror("Error: connection failed");
        close(sockfd);
        exit(1);
    }

    // Step 4: Send HTTP GET request
    char request[2048];
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\n"
                                        "Host: %s\r\n"
                                        "Connection: close\r\n"
                                        "\r\n", path, hostname);

    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("Error: send failed");
        exit(1);
    }

    // Step 5: Open file to save data
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error: failed to open file");
        fclose(fp);
        exit(1);
    }

    // Step 6: Receive the response and write to file
    ssize_t bytes_read;
    while ((bytes_read = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_read, fp);
    }

    if (bytes_read < 0) {
        perror("Error: receive failed.");
    }

    printf("File downloaded 'http://%s%s' as '%s'\n", hostname, path, filename);

    fclose(fp);
    close(sockfd);
}

char cache_dir[MAX_STRING_SIZE];
char *get_cache_dir() {
    const char *cache_env = getenv("XDG_CACHE_HOME");

    if (cache_env == NULL) {
        const char *home = getenv("HOME");

        if (home != NULL) {
            strcpy(cache_dir, home);
            strcat(cache_dir, "/.cache");
        } else {
            perror("Error: could not find home directory.\n");
            exit(0);
        }
    } else {
        strcpy(cache_dir, cache_env);
    }

    if (!is_file_exist(cache_dir)) {
        fprintf(stdout, "Error: '%s' does not exist.\n", cache_dir);
        exit(0);
    }

    return cache_dir;
}

void create_dir(const char *path) {
    if (mkdir(path, 0755) == -1) {
        if (errno != EEXIST) {
            fprintf(stdout, "Error: creating directory '%s'. %s", path, strerror(errno));
            exit(1);
        }
    } else {
        printf("Directory created successfully: %s\n", path);
    }
}

void mk_emoji_cache_file(char *emoji_og_cache_file, char *emoji_cache_file) {
    FILE *emoji_og_cache_file_p = fopen(emoji_og_cache_file, "r");
    if (emoji_og_cache_file_p == NULL) {
        perror("Error: couldn't opening ");
        perror(emoji_og_cache_file);
        perror(" file.");
        fclose(emoji_og_cache_file_p);
        exit(1);
    }

    FILE *emoji_cache_file_p = fopen(emoji_cache_file, "w");
    if (emoji_cache_file_p == NULL) {
        perror("Error: couldn't opening ");
        perror(emoji_cache_file);
        perror(" file.");
        fclose(emoji_og_cache_file_p);
        fclose(emoji_cache_file_p);
        exit(1);
    }

    char line[MAX_LINE_LENGTH];

    while (fgets(line, MAX_LINE_LENGTH, emoji_og_cache_file_p)) {
        int index = 77;

        if (line[index] == '#') {
            bool id_found = false;

            for (size_t i = 0; i < sizeof(line) ; i++) {
                int emoji_start_i = index+2;
                char emoji_code = line[i+emoji_start_i];

                if (id_found && emoji_code != ' ') {
                    continue;
                } else {
                    id_found = false;
                }

                if (emoji_code == 'E') {
                    id_found = true;
                    fprintf(emoji_cache_file_p, "...");
                } else {
                    fprintf(emoji_cache_file_p, "%c", emoji_code);
                }

                if (line[i+emoji_start_i] == '\n') break;
            }
        }
    }

    printf("Created emoji cache file in '%s'\n", emoji_cache_file);

    fclose(emoji_cache_file_p);
    fclose(emoji_og_cache_file_p);
}

void mk_math_cache_file(char *math_og_cache_file, char *math_cache_file) {
    FILE *math_og_cache_file_p = fopen(math_og_cache_file, "r");
    if (math_og_cache_file_p == NULL) {
        perror("Error: couldn't opening ");
        perror(math_og_cache_file);
        perror(" file.");
        exit(1);
    }

    FILE *math_cache_file_p = fopen(math_cache_file, "w");
    if (math_cache_file_p == NULL) {
        perror("Error: couldn't opening ");
        perror(math_cache_file);
        perror(" file.");
        fclose(math_og_cache_file_p);
        exit(1);
    }

    char line[MAX_LINE_LENGTH];

    while (fgets(line, MAX_LINE_LENGTH, math_og_cache_file_p)) {
        if (!isdigit(line[0])) continue;

        int colen_count = 0;
        bool is_break_set = false;
        for (size_t i = 0; line[i] != '\0'; i++) {
            if (line[i] == ';') {
                colen_count += 1;
            }
            if (colen_count > 1 && colen_count <= 2) {
                if (line[i+1] != ';') {
                    fprintf(math_cache_file_p, "%c", line[i+1]);
                }
            }
            if (colen_count > 4) {
                if (colen_count == 5 && !is_break_set) {
                    fprintf(math_cache_file_p, " ...");
                    is_break_set = true;
                } else {
                    fprintf(math_cache_file_p, "%c", tolower(line[i+1]));
                }
            }
            if (line[i] == '\n') break;
        }
    }

    printf("Created math cache file in '%s'\n", math_cache_file);

    fclose(math_cache_file_p);
    fclose(math_og_cache_file_p);
}

int count_word(char *str, const char word) {
    int word_count = 0;

    for (size_t i = 0; i < strlen(str); i++) {
        if (str[i] == word) word_count += 1;
    }

    return word_count;
}

void run_fzf(char *filename) {
    char selected_line[MAX_STRING_SIZE] = "";
    int pipe_input[2];
    int pipe_output[2];

    if (pipe(pipe_input) == -1) {
        perror("Error: failed to create pipe input.");
        exit(1);
    }
    if (pipe(pipe_output) == -1) {
        perror("Error: failed to create pipe input.");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error: failed to create fork.");
        exit(1);
    }

    if (pid == 0) {
        close(pipe_input[1]);
        close(pipe_output[0]);

        dup2(pipe_input[0], STDIN_FILENO);
        dup2(pipe_output[1], STDOUT_FILENO);

        close(pipe_input[0]);
        close(pipe_output[1]);

        execlp("fzf", "fzf", (char *)NULL);
        perror("Error: execlp failed.");
        exit(1);
    } else {
        close(pipe_input[0]);
        close(pipe_output[1]);

        FILE *fd = fopen(filename, "r");
        if (fd == NULL) {
            perror("Failed to open file.");
            exit(1);
        }

        char line[MAX_LINE_LENGTH];
        while (fgets(line, sizeof(line), fd) != NULL) {
            write(pipe_input[1], line, strlen(line));
        }

        fclose(fd);
        close(pipe_input[1]);

        char xline[MAX_LINE_LENGTH];
        ssize_t nbytes = read(pipe_output[0], xline, sizeof(xline) - 1);

        if (nbytes > 0 && pipe_input[0] == 3) {
            xline[nbytes] = '\0';
            strcpy(selected_line, xline);
        }

        close(pipe_output[0]);
        wait(NULL);
    }

    FILE *xclip = popen("xclip -selection clipboard", "w");
    if (xclip) {
        for (size_t i = 0; i < sizeof(selected_line); i++) {
            if (selected_line[i] == ' ') break;
            fprintf(xclip, "%c", selected_line[i]);
            printf("%c", selected_line[i]);
        }
    } else {
        perror("Error: failed to open pipe to xclip");
        exit(1);
    }
    pclose(xclip);
}

int main(int argc, char *argv[]) {
    const char *hostname = "unicode.org";
    const char *cache_dir = get_cache_dir();
    bool rebuild;

    if (argc == 2) rebuild = str_cmp(argv[1], "--rebuild") ||  str_cmp(argv[1], "-r");
    if (rebuild) printf("Rebuilding cache files...\n");

    char cache_proj_dir[MAX_STRING_SIZE];
    char emoji_og_cache_file[MAX_STRING_SIZE];
    char math_og_cache_file[MAX_STRING_SIZE];
    char emoji_cache_file[MAX_STRING_SIZE];
    char math_cache_file[MAX_STRING_SIZE];
    char kaomoji_cache_file[MAX_STRING_SIZE];

    strcpy(cache_proj_dir, cache_dir);
    strcat(cache_proj_dir, "/"PROJECT_NAME);
    create_dir(cache_proj_dir);

    strcpy(emoji_og_cache_file, cache_dir);
    strcat(emoji_og_cache_file, "/"PROJECT_NAME"/emoji_og.txt");
    if (!is_file_exist(emoji_og_cache_file) || rebuild) {
        fetch(hostname, "/Public/emoji/latest/emoji-test.txt", emoji_og_cache_file);
    }

    strcpy(math_og_cache_file, cache_dir);
    strcat(math_og_cache_file, "/"PROJECT_NAME"/math_og.txt");
    if (!is_file_exist(math_og_cache_file) || rebuild) {
        fetch(hostname, "/Public/math/latest/MathClassEx-15.txt", math_og_cache_file);
    }

    strcpy(kaomoji_cache_file, cache_dir);
    strcat(kaomoji_cache_file, "/"PROJECT_NAME"/kaomoji.txt");
    if (!is_file_exist(kaomoji_cache_file) || rebuild) {
        fetch("gist.githubusercontent.com", "/AnzenKodo/d35434596cc94c6577817f1c5893ea49/raw/2d605a4b3179451a77b85dbb9e79d0b9d036c863/kaomoji.txt", kaomoji_cache_file);
    }

    strcpy(emoji_cache_file, cache_dir);
    strcat(emoji_cache_file, "/"PROJECT_NAME"/emoji.txt");
    if (!is_file_exist(emoji_cache_file) || rebuild) {
        mk_emoji_cache_file(emoji_og_cache_file, emoji_cache_file);
    }

    strcpy(math_cache_file, cache_dir);
    strcat(math_cache_file, "/"PROJECT_NAME"/math.txt");
    if (!is_file_exist(math_cache_file) || rebuild) {
        mk_math_cache_file(math_og_cache_file, math_cache_file);
    }

    char *help_msg = "cymbols: Unicode picker with fzf.\n"
    "Usage:\n"
    "   --emoji   -e    Open emoji piker\n"
    "   --math    -m    Open math piker\n"
    "   --kaomoji -k    Open Kaomoji piker\n"
    "   --rebuild -r    Rebuild cache files\n"
    "   --help    -h    Print help message\n"
    "   --version -v    Print verion\n"
    "Version: "VERSION"\n"
    "SPDX-License-Identifier: MIT (https://spdx.org/licenses/MIT)\n";

    if (argc == 1) run_fzf(emoji_cache_file);
    for (size_t i = 1; i < (size_t)argc; i++) {
        char *arg = argv[i];

        if (str_cmp(arg, "--help") || str_cmp(arg, "-h")) {
            printf(help_msg);
        } else if (str_cmp(arg, "--version") || str_cmp(arg, "-v")) {
            printf("Version: "VERSION"\n");
        } else if (str_cmp(arg, "--emoji") || str_cmp(arg, "-e")) {
            run_fzf(emoji_cache_file);
        } else if (str_cmp(arg, "--math") || str_cmp(arg, "-m")) {
            run_fzf(math_cache_file);
        } else if (str_cmp(arg, "--kaomoji") || str_cmp(arg, "-k")) {
            run_fzf(kaomoji_cache_file);
        } else if (str_cmp(arg, "--symbol") || str_cmp(arg, "-s")) {
        } else if (rebuild) {
        } else {
            fprintf(stdout, "Error: wrong argument are passed.\n");
            printf(help_msg);
        }
    }

    return 0;
}
