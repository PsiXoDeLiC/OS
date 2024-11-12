#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LENGTH 256

extern char **environ;

char *argument[MAX_LENGTH];  // Хранит аргументы команды
char directory[MAX_LENGTH];  // Хранит путь текущей директории
int count;

void code_processing(char *input) {  // Разбиение строки на команду и аргументы
    char *temp = strtok(input, " ");
    count = 0;
    while (temp != NULL) {
        argument[count++] = temp;
        temp = strtok(NULL, " ");
    }
    argument[count] = NULL;
}

void cd_processing() {
    if (argument[1] == NULL) {  // если нет аргумента, узнать текущую директорию
        printf("the current directory: %s\n", directory);
    } else {  // если не найдена директория, сообщить об этом
        if (chdir(argument[1]) != 0) {
            printf("The directory was not found\n");
        } else {  // изменить директорию и переменную среды на указанную в аргументе
            getcwd(directory, sizeof(directory));
            setenv("PWD", directory, 1);
        }
    }
}

void dir_processing() {
    DIR *dir;
    struct dirent *entry;
    char catalog[MAX_LENGTH];

    // при отсутствии аргумента показать содержимое текущей директории
    if (argument[1] == NULL) {
        strcpy(catalog, directory);
    } else {
        strcpy(catalog, argument[1]);
    }

    dir = opendir(catalog);
    if (dir == NULL) {
        printf("Invalid directory is specified\n");
    } else {  // иначе показать содержимое указанной директории
        while ((entry = readdir(dir)) != NULL) {
            printf("%s\n", entry->d_name);
        }
        closedir(dir);
    }
}

void environ_processing() {  // вывод переменных среды
    char **env = environ;
    while (*env != NULL) {
        printf("%s\n", *env);
        env++;
    }
}

void echo_processing() {
    for (int i = 1; argument[i] != NULL; ++i) {
        printf("%s ", argument[i]);
    }
    printf("\n");
}

void help_processing() {  // вывод поддерживаемых команд
    printf("Supported commands:\n");
    printf("cd <directory>: Change current directory\n");
    printf("clr: Clear the screen\n");
    printf("dir <directory>: List contents of directory\n");
    printf("environ: List all environment variables\n");
    printf("echo <comment>: Display comment\n");
    printf("help: Display this help message\n");
    printf("pause: Pause the shell until Enter is pressed\n");
    printf("quit: Exit the shell\n");
}

void pause_processing() {
    printf("Shell paused. Press Enter to continue...\n");
    while (getchar() != '\n');
}

void child_processing(int background, int input_fd, int output_fd) {
    pid_t pid = fork();

    if (pid == 0) {  // дочерний процесс
        setenv("parent", directory, 1);  // Устанавливаем переменную среды "parent"

        // Перенаправление ввода/вывода, если файлы указаны
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        if (background) {
            int devnull = open("/dev/null", O_RDWR);
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        execvp(argument[0], argument);
        fprintf(stderr, "Command not found: %s\n", argument[0]);
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
    } else {
        if (!background) {  
            int status;
            waitpid(pid, &status, 0);
        } else {
            printf("Running in background, PID: %d\n", pid);
        }
    }
}

void process_command() {
    int background = 0;
    int input_fd = -1, output_fd = -1;
    char *input_file = NULL, *output_file = NULL;
    int saved_stdout = -1; // Переменная для сохранения исходного stdout

    // Проверка на наличие символа фоновой задачи (&)
    if (strcmp(argument[count - 1], "&") == 0) {
        background = 1;
        argument[--count] = NULL;  // Убираем символ & из аргументов
    }

    // Поиск символов перенаправления
    for (int i = 0; i < count; i++) {
        if (strcmp(argument[i], "<") == 0) {
            input_file = argument[i + 1];
            argument[i] = NULL;
            input_fd = open(input_file, O_RDONLY);
            if (input_fd == -1) {
                perror("Failed to open input file");
                return;
            }
        } else if (strcmp(argument[i], ">") == 0) {
            output_file = argument[i + 1];
            argument[i] = NULL;
            output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd == -1) {
                perror("Failed to open output file");
                return;
            }
        } else if (strcmp(argument[i], ">>") == 0) {
            output_file = argument[i + 1];
            argument[i] = NULL;
            output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (output_fd == -1) {
                perror("Failed to open output file");
                return;
            }
        }
    }

    // Если есть перенаправление, сохраняем текущий stdout и перенаправляем его
    if (output_fd != -1) {
        saved_stdout = dup(STDOUT_FILENO); // Сохраняем текущий stdout
        dup2(output_fd, STDOUT_FILENO);    // Перенаправляем stdout в файл
    }

    // Обработка команд
    if (strcmp(argument[0], "quit") == 0) {
        exit(0);
    } else if (strcmp(argument[0], "cd") == 0) {
        cd_processing();
    } else if (strcmp(argument[0], "clr") == 0) {
        system("clear");
    } else if (strcmp(argument[0], "dir") == 0) {
        dir_processing();
    } else if (strcmp(argument[0], "environ") == 0) {
        environ_processing();
    } else if (strcmp(argument[0], "echo") == 0) {
        echo_processing();
    } else if (strcmp(argument[0], "help") == 0) {
        help_processing();
    } else if (strcmp(argument[0], "pause") == 0) {
        pause_processing();
    } else {
        child_processing(background, input_fd, output_fd);
    }

    // Восстанавливаем stdout, если было перенаправление
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO); // Восстанавливаем stdout
        close(saved_stdout);               // Закрываем сохраненный дескриптор
    }
}

int main(int argc, char *argv[]) {
    char input[MAX_LENGTH];
    FILE *input_file = NULL;

    if (argc > 1) {
        input_file = fopen(argv[1], "r");
        if (input_file == NULL) {
            perror("Failed to open file");
            return 1;
        }
    }

    getcwd(directory, sizeof(directory));
    setenv("shell", directory, 1);  // Устанавливаем переменную среды "shell"
    getcwd(directory, sizeof(directory));

    while (1) {
        if (input_file != NULL) {
            if (fgets(input, sizeof(input), input_file) == NULL) {
                fclose(input_file);
                break;
            }
            input[strcspn(input, "\n")] = '\0';
        } else {
            printf("myshell: %s> ", directory);
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0';
        }

        if (strlen(input) == 0) continue;

        code_processing(input);
        if (count == 0) continue;

        process_command();
    }

    return 0;
}

