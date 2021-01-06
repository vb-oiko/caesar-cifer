#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

#define ALPHABET_LEN 26
const char *ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

#define MULTILINE(...) #__VA_ARGS__
const char *DEFAULT_FREQUENCIES = MULTILINE(
    {
        "A" : 0.084883,
        "B" : 0.015791,
        "C" : 0.032149,
        "D" : 0.038351,
        "E" : 0.121006,
        "F" : 0.021306,
        "G" : 0.020925,
        "H" : 0.047875,
        "I" : 0.072240,
        "J" : 0.002258,
        "K" : 0.008187,
        "L" : 0.042097,
        "M" : 0.025545,
        "N" : 0.071593,
        "O" : 0.075920,
        "P" : 0.021200,
        "Q" : 0.000866,
        "R" : 0.063569,
        "S" : 0.066581,
        "T" : 0.090040,
        "U" : 0.027324,
        "V" : 0.010819,
        "W" : 0.018478,
        "X" : 0.001888,
        "Y" : 0.017939,
        "Z" : 0.001168
    });

void printHelpMsg();
void printWrongCommandMsg(char *str);
void printSeeHelp();
int isDecodeCommand(char *str);
int isEncodeCommand(char *str);
int isFrequencyCommand(char *str);
void parseFileNames(int argc, char *argv[], int fileNameArgInd);
FILE *openFile(const char *filename, const char *mode);
void closeFiles();
void getFilenameWithPath(const char *filename, char *filenameWithPath);
char *readStreamToString(FILE *fileptr);
int getRndShift();
char *encode(const char *msg, const int shift);
float *getFrequencies(const char *text, const int shift);
char *jsonEncodeFrequencies(float *frequencies);
void parseShiftOptionValue(char *arg);
void parseFrequencyOptionValue(char *arg);

int shift = 0;
int commandArgInd = 1;
FILE *input = NULL;
FILE *output = NULL;
float *frequencies;

int main(int argc, char *argv[])
{
    int opt;

    while (1)
    {
        static struct option long_options[] =
            {
                {"help", no_argument, 0, 'h'},
                {"shift", required_argument, 0, 's'},
                {"frequency", required_argument, 0, 'f'},
                {0, 0, 0, 0}};

        int option_index = 0;

        opt = getopt_long(argc, argv, "hs:f:",
                          long_options, &option_index);

        if (opt == -1)
            break;

        switch (opt)
        {

        case 'h':
            printHelpMsg();
            exit(EXIT_SUCCESS);
            break;

        case 's':
            parseShiftOptionValue(optarg);
            break;

        case 'f':
            parseFrequencyOptionValue(optarg);
            break;

        case '?':
            break;

        default:
            printHelpMsg();
            fprintf(stderr, "\n\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        if (argc > 1)
        {

            fprintf(stderr, "caesar-cifer: no command specified\n");
            printf("\n");
            printHelpMsg();
            exit(EXIT_FAILURE);
        }
        printHelpMsg();
        exit(EXIT_SUCCESS);
    }

    if (isEncodeCommand(argv[optind]))
    {
        if (shift == 0)
        {
            shift = getRndShift();
            fprintf(stderr, "caesar-cifer: shift value not specified, random value=%d will be used\n", shift);
        }

        parseFileNames(argc, argv, optind + 1);

        char *msg = readStreamToString(input);

        char *encodedMsg = encode(msg, shift);
        fprintf(output != NULL ? output : stdout, "%s", encodedMsg);
        fflush(stdout);

        free(encodedMsg);
        free(msg);
        closeFiles();
        exit(EXIT_SUCCESS);
    }

    if (isDecodeCommand(argv[optind]))
    {
        if (shift != 0)
        {
            fprintf(stderr, "caesar-cifer: shift value ignored for decoding, trying all possible shift values\n");
        }

        fprintf(stderr, "decoding...\n");
        parseFileNames(argc, argv, optind + 1);
        closeFiles();
        exit(EXIT_SUCCESS);
    }

    if (isFrequencyCommand(argv[optind]))
    {
        if (shift != 0)
        {
            fprintf(stderr, "caesar-cifer: shift value ignored for computing frequencies\n");
        }

        parseFileNames(argc, argv, optind + 1);
        char *text = readStreamToString(input);

        float *frequencies = getFrequencies(text, 0);
        char *json = jsonEncodeFrequencies(frequencies);

        fprintf(output != NULL ? output : stdout, "%s", json);
        fflush(stdout);

        free(frequencies);
        free(json);
        closeFiles();
        exit(EXIT_SUCCESS);
    }

    printWrongCommandMsg(argv[optind]);
    return EXIT_FAILURE;
}

void parseFrequencyOptionValue(char *arg)
{
    FILE *freqFile = openFile(arg, "r");
}

void parseShiftOptionValue(char *arg)
{
    char *n_endptr;
    int n = strtol(arg, &n_endptr, 10);

    if (n_endptr == arg || *n_endptr != '\0')
    {
        fprintf(stderr, "caesar-cifer: shift value specified is not a decimal number\n");
        exit(EXIT_FAILURE);
    }

    if (n < 1 || n > 25)
    {
        fprintf(stderr, "caesar-cifer: shift value must be in range 1 .. %d\n", ALPHABET_LEN - 1);
        exit(EXIT_FAILURE);
    }

    shift = n;
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
int isFrequencyCommand(char *str)
{
    static char *command = "frequency";
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
    fprintf(stderr, "  -h, --help               Display current manual\n");
    fprintf(stderr, "  -s, --shift integer      Use specified shift value when encoding\n");
    fprintf(stderr, "  -f, --frequency string   Use specified file with letters frrquencies\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  encode      Encode stream\n");
    fprintf(stderr, "  decode      Decode stream\n");
    fprintf(stderr, "  frequency   Compute frequencies of the letters in stream\n");
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

    fprintf(stderr, "caesar-cifer: Failed to open file '%s'\n\n", filename);
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

char *readStreamToString(FILE *fileptr)
{
    char *buffer = NULL;
    size_t len = 0;
    ssize_t bytes_read = getdelim(&buffer, &len, '\0', fileptr != NULL ? fileptr : stdin);
    if (bytes_read == -1)
    {
        fprintf(stderr, "caesar-cifer: failed to read data\n");
        exit(EXIT_FAILURE);
    }

    return buffer;
}

int getRndShift()
{
    srand(time(NULL));
    return rand() % (ALPHABET_LEN - 1) + 1;
}

char *encode(const char *msg, const int shift)
{
    size_t len = strlen(msg);
    char *encodedMsg = malloc(len * sizeof(char));
    if (encodedMsg == NULL)
    {
        fprintf(stderr, "caesar-cifer: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < len; i++)
    {
        char *offset = strchr(ALPHABET, msg[i]);

        if (offset == NULL)
        {
            encodedMsg[i] = msg[i];
        }
        else
        {
            size_t pos = offset - ALPHABET;
            size_t encodedPos = (pos + shift) % ALPHABET_LEN;
            encodedMsg[i] = ALPHABET[encodedPos];
        }
    }

    return encodedMsg;
}

float *getFrequencies(const char *text, const int shift)
{
    size_t *counts = malloc(ALPHABET_LEN * sizeof(size_t));
    if (counts == NULL)
    {
        fprintf(stderr, "caesar-cifer: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < ALPHABET_LEN; i++)
    {
        counts[i] = 0;
    }

    size_t len = strlen(text);

    for (size_t i = 0; i < len; i++)
    {
        char *offset = strchr(ALPHABET, text[i]);

        if (offset != NULL)
        {
            size_t pos = offset - ALPHABET;
            size_t decodedPos = (pos + ALPHABET_LEN - shift) % ALPHABET_LEN;
            counts[decodedPos]++;
        }
    }

    size_t total = 0;
    for (size_t i = 0; i < ALPHABET_LEN; i++)
    {
        total += counts[i];
    }

    float *frequencies = malloc(ALPHABET_LEN * sizeof(size_t));
    if (frequencies == NULL)
    {
        fprintf(stderr, "caesar-cifer: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < ALPHABET_LEN; i++)
    {
        frequencies[i] = (float)counts[i] / (float)total;
    }

    free(counts);
    return frequencies;
}

char *jsonEncodeFrequencies(float *frequencies)
{
    char tempStr[1024];
    char *json = malloc(2048 * sizeof(char));
    if (json == NULL)
    {
        fprintf(stderr, "caesar-cifer: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    json[0] = '\0';
    strcat(json, "{\n");

    for (size_t i = 0; i < ALPHABET_LEN; i++)
    {
        sprintf(tempStr, "  \"%c\": %f%s", ALPHABET[i], frequencies[i], (i == ALPHABET_LEN - 1) ? "\n" : ",\n");
        strcat(json, tempStr);
    }

    strcat(json, "}\n\0");

    return json;
}