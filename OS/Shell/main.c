#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>


#define MAX_LENGTH 256
extern char **environ;

char *argument[MAX_LENGTH];// Хранит аргументы команды
char directory[MAX_LENGTH];// Хранит путь текущей директории
int count;

void code_processing(char *input){ //разбиение строки на команду и аргументы
    char *temp = strtok(input, " ");
    count = 0;
    while (temp != NULL){
        argument[count++] = temp;
        printf("%s ", argument[count-1]);
        temp = strtok(NULL, " ");

    }
    argument[count] = NULL;
}


void cd_processing(){
    if (argument[1] == NULL) {//если нет аргумента узнать текущую директорию
        printf("the current directory: %s\n", directory);
    }
    else {//если не найдена директория соответствующая аргументу сообщить об этом
        if(chdir(argument[1]) != 0) {
            printf("The directory was not found\n");
        }
        else {//изменить дерикторию и переменную среды на указанный в аргументе
            getcwd(directory, sizeof(directory));
            setenv("PWD", directory, 1);
        }
    }
}

void dir_processing(){
    DIR *dir;
    struct dirent *entry;
    char catalog[MAX_LENGTH];
    //при отсутствии аргумента показать содержимое текущей директории
    if (argument[1] == NULL) {
        strcpy(catalog, directory);
    }
    else {
        strcpy(catalog, argument[1]);
    }//если указанной дериктории не существует сообщить об этом
    dir = opendir(catalog);
    if (dir == NULL) {
        printf("Invalid directory is specified\n");
    }
    else{//иначе показать содержимое указанной директории
        while ((entry = readdir(dir)) != NULL) {
            printf("%s\n", entry->d_name);
        }
    }
    closedir(dir);
}

void environ_processing(){//вывод переменных среды
    char **env = environ;;
    while (*env != NULL) {
        printf("%s\n", *env);
        env++;
    }
}

void echo_processing() {
    // Проход по каждому элементу массива argument и печать его
    for (int i = 1; i < count; ++i) {
        printf("%s ", argument[i]);
    }
    printf("\n");
}

void help_processing(){//вывод поддерживаемых комманд
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

void child_processing() {
    pid_t pid = fork();

    if (pid == 0) {//если не удалось запустить дочерний процес сообщаем о неизвестной команде
        execvp(argument[0], argument);
        fprintf(stderr, "Command not found: %s\n", argument[0]);
        exit(EXIT_FAILURE);
    } else if (pid < 0) {//если дочерний процесс выдал ошибку сообщаем о ней
        fprintf(stderr, "Fork failed\n");
    } else {//иначе ждем заверщения дочернего процесса 
        int status;
        waitpid(pid, &status, 0);
    }
}


int main(int argc, char *argv[]) {
    char input[MAX_LENGTH];// Хранит ввод пользователя

    // Получаем текущий рабочий каталог
    char path[MAX_LENGTH];
    getcwd(path, sizeof(path));
    strcat(path, "/myshell");

    setenv("shell", path, 1);

    getcwd(directory, sizeof(directory));

    // Основной цикл обработки команд
    while (1) {

        //обработка ввода

        printf("myshell: %s> ", directory);//Приглашение командной строки
        fgets(input, sizeof(input), stdin);// Запрос ввода пользователя
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "\n") == 0) {
            continue;//Пропускаем обработку пустой строки
        }

        code_processing(input);


//Определение и исполнение комманды

        if (strcmp(argument[0], "quit") == 0) {//выход из оболочки
            break;
        }
        else if (strcmp(argument[0], "cd") == 0) {//Директория
            cd_processing();
        }
        else if (strcmp(argument[0], "clr") == 0) {//очистить терминал
            system("clear");
        }
        else if (strcmp(argument[0], "dir") == 0) {//Содержимое каталога
            dir_processing();
        }//Показать содержимое среды
        else if (strcmp(argument[0], "environ") == 0) {
            environ_processing();
        }// вывод на экран комментария указанного в аргументе
        else if (strcmp(argument[0], "echo") == 0) {
            echo_processing();
        }//вывод руководства пользователя.
        else if (strcmp(argument[0], "help") == 0) {
            help_processing();
        }//приостановка операций оболочки до нажатия клавиши <Enter>.
        else if (strcmp(argument[0], "pause") == 0) {
            printf("Press Enter to continue...");
            getchar();
        }
            //иначе попытка запустить как дочерний процесс
        else {
            child_processing();
        }
    }


    return 0;
}
