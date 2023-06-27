#include "pappl-private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct {
    char* key;
    char* value;
} KeyValuePair;

typedef struct {
    KeyValuePair* pairs;
    size_t count;
} RawOptions;

typedef struct {
    char* icon;
    char* note;
    char* location;
    double gray_gamma;
    double color_gamma;
    bool synthesize_gray;
    RawOptions sane_options;
} Options;

typedef struct {
    char* device_name;
    RawOptions options;
} DeviceOptions;

typedef struct {
    char* fileName;
    RawOptions globalOptions;
    DeviceOptions* deviceOptions;
    size_t deviceOptionsCount;
} OptionsFile;

void delete_KeyValuePair(KeyValuePair* pair) {
    free(pair->key);
    free(pair->value);
}

RawOptions new_RawOptions() {
    RawOptions options;
    options.pairs = NULL;
    options.count = 0;
    return options;
}

void delete_RawOptions(RawOptions* options) {
    for (size_t i = 0; i < options->count; ++i) {
        delete_KeyValuePair(&options->pairs[i]);
    }
    free(options->pairs);
    options->pairs = NULL;
    options->count = 0;
}

Options new_Options() {
    Options options;
    options.icon = NULL;
    options.note = NULL;
    options.location = NULL;  
    options.gray_gamma = 1.0;
    options.color_gamma = 1.0;
    options.synthesize_gray = false;
    options.sane_options = new_RawOptions();
    return options;
}

void delete_Options(Options* options) {
    free(options->icon);
    free(options->note);
    free(options->location);  
    delete_RawOptions(&options->sane_options);
}

DeviceOptions new_DeviceOptions() {
    DeviceOptions options;
    options.device_name = NULL;
    options.options = new_RawOptions();
    return options;
}

void delete_DeviceOptions(DeviceOptions* options) {
    free(options->device_name);
    delete_RawOptions(&options->options);
}

void delete_OptionsFile(OptionsFile* instance) {
    free(instance->fileName);
    delete_RawOptions(&instance->globalOptions);
    for (size_t i = 0; i < instance->deviceOptionsCount; ++i) {
        delete_DeviceOptions(&instance->deviceOptions[i]);
    }
    free(instance->deviceOptions);
    free(instance);
}

OptionsFile* new_OptionsFile(const char* fileName) {
    OptionsFile* file = (OptionsFile*)malloc(sizeof(OptionsFile));
    file->fileName = strdup(fileName);
    file->deviceOptions = NULL;
    file->deviceOptionsCount = 0;
    file->globalOptions = new_RawOptions();

    FILE* fp = fopen(fileName, "r");
    if (fp == NULL) {
        printf("no device options at '%s'\n", fileName);
        return file;
    }
    printf("reading device options from '%s'\n", fileName);

    char line[256];
    RawOptions* pDeviceSection = NULL;

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '#')
            continue;

        char* name = strtok(line, " \t\n");
        char* value = strtok(NULL, "\n");
        if (value) {
            char* end = value + strlen(value) - 1;
            while (end > value && isspace((unsigned char)*end))
                end--;
            end[1] = '\0';
        }

        if (strcmp(name, "device") == 0) {
            file->deviceOptions = (DeviceOptions*)realloc(file->deviceOptions, sizeof(DeviceOptions) * (file->deviceOptionsCount + 1));
            DeviceOptions* devOpt = &file->deviceOptions[file->deviceOptionsCount];
            devOpt->device_name = strdup(value);
            devOpt->options = new_RawOptions();
            pDeviceSection = &devOpt->options;
            file->deviceOptionsCount++;
        } else {
            RawOptions* target = pDeviceSection ? pDeviceSection : &file->globalOptions;
            target->pairs = (KeyValuePair*)realloc(target->pairs, sizeof(KeyValuePair) * (target->count + 1));
            target->pairs[target->count].key = strdup(name);
            target->pairs[target->count].value = strdup(value ? value : "");
            target->count++;
        }
    }

    fclose(fp);
    return file;
}

char* path(const char* fileName) {
    char* path = NULL;
    const char* pos = strrchr(fileName, '/');

    if (pos != NULL) {
        size_t length = pos - fileName + 1;
        path = (char*)malloc(length + 1);

        if (path != NULL) {
            strncpy(path, fileName, length);
            path[length] = '\0'; 
        }
    } else {
        path = strdup(""); 
    }

    return path;
}

Options scannerOptions(const OptionsFile* optionsFile, pappl_scanner_t * scanner) {
    RawOptions rawOptions = optionsFile->globalOptions;
    for (size_t i = 0; i < optionsFile->deviceOptionsCount; ++i) {
        DeviceOptions* section = &optionsFile->deviceOptions[i];
        bool match = false;
        if (strcmp(scanner->sane_name, section->device_name) == 0) {
            fprintf(stderr, "%s: device name '%s' matches device name '%s'\n",
                    optionsFile->fileName, section->device_name, scanner->sane_name);
            match = true;
        } else if (strcmp(scanner->make_and_model, section->device_name) == 0) {
            fprintf(stderr, "%s: device make and model '%s' matches device name '%s'\n",
                    optionsFile->fileName, scanner->make_and_model, section->device_name);
            match = true;
        }
        if (match) {
            
            rawOptions.pairs = realloc(rawOptions.pairs, sizeof(KeyValuePair) * (rawOptions.count + section->options.count));
            memcpy(rawOptions.pairs + rawOptions.count, section->options.pairs, sizeof(KeyValuePair) * section->options.count);
            rawOptions.count += section->options.count;
        }
    }

    Options processedOptions = new_Options();
    for (size_t i = 0; i < rawOptions.count; ++i) {
        KeyValuePair* option = &rawOptions.pairs[i];
        if (strcmp(option->key, "icon") == 0) {
            processedOptions.icon = strdup(option->value);
            if (processedOptions.icon[0] != '/') {
                char* filePath = path(optionsFile->fileName);
                size_t len = strlen(filePath) + strlen(processedOptions.icon) + 1;
                processedOptions.icon = realloc(processedOptions.icon, len);
                memmove(processedOptions.icon + strlen(filePath), processedOptions.icon, strlen(processedOptions.icon) + 1);
                memcpy(processedOptions.icon, filePath, strlen(filePath));
                free(filePath);
            }
        } else if (strcmp(option->key, "note") == 0) {
            processedOptions.note = strdup(option->value);
        } else if (strcmp(option->key, "location") == 0) { 
            processedOptions.location = strdup(option->value);
        } else if (strcmp(option->key, "gray-gamma") == 0) {
            processedOptions.gray_gamma = atof(option->value);
        } else if (strcmp(option->key, "color-gamma") == 0) {
            processedOptions.color_gamma = atof(option->value);
        } else if (strcmp(option->key, "synthesize-gray") == 0) {
            processedOptions.synthesize_gray = (bool)atoi(option->value);
        } else {
            processedOptions.sane_options.pairs = realloc(processedOptions.sane_options.pairs, sizeof(KeyValuePair) * (processedOptions.sane_options.count + 1));
            processedOptions.sane_options.pairs[processedOptions.sane_options.count].key = strdup(option->key);
            processedOptions.sane_options.pairs[processedOptions.sane_options.count].value = strdup(option->value);
            processedOptions.sane_options.count++;
        }
    }

    delete_RawOptions(&rawOptions);
    return processedOptions;
}
