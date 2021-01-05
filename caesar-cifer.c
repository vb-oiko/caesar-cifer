#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define ALPHABET_LEN 26

void printHelpMsg();
void printWrongCommandMsg(char *str);
void printSeeHelp();
int isHelpOption(char *str);
int isShiftOption(char *str);
int isOptionTag(char *str);
int isOption(char *str);
int isDecodeCommand(char *str);
int isEncodeCommand(char *str);
void parseFileNames(int argc, char *argv[], int fileNameArgInd);
FILE *openFile(const char *filename, const char *mode);
void closeFiles();
void getFilenameWithPath(const char *filename, char *filenameWithPath);
char *readMsg();
int getRndShift();

int shift = 0;
int commandArgInd = 1;
FILE *input = NULL;
FILE *output = NULL;

int main(int argc, char *argv[])
{
    if (
        argc == 1 ||
        (argc > 1 && isHelpOption(argv[1])))
    {
        printHelpMsg();
        return EXIT_SUCCESS;
    }

    if (argc > 1 && isShiftOption(argv[1]))
    {
        if (argc == 2)
        {
            fprintf(stderr, "No shift value specified\n");
            exit(EXIT_FAILURE);
        }

        char *n_endptr;
        int n = strtol(argv[2], &n_endptr, 10);

        if (n_endptr == argv[2] || *n_endptr != '\0')
        {
            fprintf(stderr, "Shift value specified is not a decimal number\n");
            exit(EXIT_FAILURE);
        }

        if (n < 1 || n > 25)
        {
            fprintf(stderr, "Shift value must be in range 1 .. %d\n", ALPHABET_LEN - 1);
            exit(EXIT_FAILURE);
        }

        shift = n;
        commandArgInd = 3;
    }

    if (argc > 1 && isOption(argv[1]) && !shift)
    {
        fprintf(stderr, "unknown flag: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if (argc > 1 && isOptionTag(argv[1]) && !shift)
    {
        fprintf(stderr, "unknown shorthand flag: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if (argc == commandArgInd)
    {
        fprintf(stderr, "no command specified\n");
        exit(EXIT_FAILURE);
    }

    if (isEncodeCommand(argv[commandArgInd]))
    {
        if (shift == 0)
        {
            fprintf(stderr, "shift value not specified, random value will be used\n");
        }

        fprintf(stderr, "encoding...\n");
        parseFileNames(argc, argv, commandArgInd + 1);

        char *msg = readMsg();
        printf("message\n%s\n", msg);

        shift = shift ? shift : getRndShift();
        printf("shift: %d\n", shift);

        free(msg);
        closeFiles();
        exit(EXIT_SUCCESS);
    }

    if (isDecodeCommand(argv[commandArgInd]))
    {
        if (shift != 0)
        {
            fprintf(stderr, "shift value ignored for decoding, trying all possible shift values\n");
        }

        fprintf(stderr, "decoding...\n");
        parseFileNames(argc, argv, commandArgInd + 1);
        closeFiles();
        exit(EXIT_SUCCESS);
    }

    printWrongCommandMsg(argv[commandArgInd]);
    return EXIT_FAILURE;
}

int isOptionTag(char *str)
{
    return str[0] == '-';
}

int isOption(char *str)
{
    return strlen(str) >= 2 && str[0] == '-' && str[1] == '-';
}

int isHelpOption(char *str)
{
    static char *tag = "-h";
    static char *option = "--help";
    return strcmp(str, tag) == 0 || strcmp(str, option) == 0;
}

int isShiftOption(char *str)
{
    static char *tag = "-s";
    static char *option = "--shift";
    return strcmp(str, tag) == 0 || strcmp(str, option) == 0;
}

int isDecodeCommand(char *str)
{
    static char *command = "decode";
    return strcmp(str, command) == 0;
}

int isEncodeCommand(char *str)
{
    static char *command = "encode";
    return strcmp(str, command) == 0;
}

void printHelpMsg()
{
    fprintf(stderr, "caesar-cifer [OPTIONS] COMMAND [[input-file] ouput-file]\n\n");
    fprintf(stderr, "Caesar cifer decoder/encoder\n");
    fprintf(stderr, "If input file is not specified, input data are read from stdin.\n");
    fprintf(stderr, "If output file is not specified, processed data are written to stdout.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s, --shift integer      Use specified shift value when encoding\n");
    fprintf(stderr, "  -h, --help               Display current manual\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  encode      Encode stream\n");
    fprintf(stderr, "  decode      Decode stream\n");
    fprintf(stderr, "\n");
}

void printWrongCommandMsg(char *str)
{
    fprintf(stderr, "caesar-cifer: '%s' is not a caesar-cifer command.\n", str);
    printSeeHelp();
}

void printSeeHelp()
{
    fprintf(stderr, "See 'caesar-cifer --help'\n\n");
    printHelpMsg();
}

void parseFileNames(int argc, char *argv[], int fileNameArgInd)
{
    if (argc == fileNameArgInd)
    {
        return;
    }

    if (argc > fileNameArgInd)
    {
        input = openFile(argv[fileNameArgInd], "r");
    }

    if (argc > fileNameArgInd + 1)
    {
        output = openFile(argv[fileNameArgInd + 1], "w");
    }

    if (argc > fileNameArgInd + 2)
    {
        fprintf(stderr, "extra arguments ignored\n");
    }
}

FILE *openFile(const char *filename, const char *mode)
{
    char fileNameWithPath[1024];
    getFilenameWithPath(filename, fileNameWithPath);

    FILE *fptr = fopen(fileNameWithPath, mode);
    if (fptr != NULL)
    {
        return fptr;
    }

    fprintf(stderr, "Failed to open file '%s'\n\n", filename);
    exit(EXIT_FAILURE);
}

void closeFiles()
{
    if (input != NULL)
    {
        fclose(input);
    }

    if (output != NULL)
    {
        fclose(output);
    }

    fprintf(stderr, "closing files...\n");
}

void getFilenameWithPath(const char *filename, char *filenameWithPath)
{
    if (strchr(filename, '/') != NULL)
    {
        strcpy(filenameWithPath, filename);
        return;
    }

    char path[1024];
    getcwd(path, sizeof(path));
    strcat(path, "/");
    strcat(path, filename);
    strcpy(filenameWithPath, path);
}

char *readMsg()
{
    char *buffer = NULL;
    size_t len = 0;
    ssize_t bytes_read = getdelim(&buffer, &len, '\0', input != NULL ? input : stdin);
    if (bytes_read == -1)
    {
        fprintf(stderr, "Failed to read data\n");
        exit(EXIT_FAILURE);
    }

    return buffer;
}

int getRndShift()
{
    srand(time(NULL));
    return rand() % (ALPHABET_LEN - 1) + 1;
}