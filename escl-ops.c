#include "pappl-private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <regex.h>

char* readXmlContent(const char* filePath) {
    char* xmlContent = NULL;
    long length;
    FILE* file = fopen(filePath, "rb");

    if (file) {
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);
        xmlContent = (char*)malloc(length + 1);
        if (xmlContent) {
            fread(xmlContent, 1, length, file);
            xmlContent[length] = '\0';  
        }
        fclose(file);
    }

    return xmlContent;
}

typedef struct ScanSettingsXml {
    char* xml;
} ScanSettingsXml;

void initScanSettingsXml(ScanSettingsXml* settings, const char* s) {
    settings->xml = (char*)malloc(strlen(s) + 1);
    strcpy(settings->xml, s);
}

char* getString(const ScanSettingsXml* settings, const char* name, const char* pattern) {
    const int max_matches = 2;
    regex_t regex;
    regmatch_t matches[max_matches];

    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    if (regexec(&regex, settings->xml, max_matches, matches, 0) == 0) {
        size_t match_length = matches[1].rm_eo - matches[1].rm_so;
        char* result = (char*)malloc(match_length + 1);
        strncpy(result, settings->xml + matches[1].rm_so, match_length);
        result[match_length] = '\0';

        regfree(&regex);
        return result;
    }

    regfree(&regex);
    char* empty = (char*)malloc(1);
    empty[0] = '\0';
    return empty;
}

double getNumber(const ScanSettingsXml* settings, const char* name, const char* pattern) {
    char* string_value = getString(settings, name, pattern);
    double result = strtod(string_value, NULL);
    free(string_value);
    return result;
}

bool ClientAlreadyAirScan(pappl_client_t *client) {
    const char* airscan_string = "AirScanScanner";
    size_t airscan_length = strlen(airscan_string);

    const char* user_agent = httpGetField(client->http, HTTP_FIELD_USER_AGENT);
    const char* result = strstr(user_agent, airscan_string);

    if (result != NULL) {
        size_t substring_length = strlen(result);
        if (substring_length > airscan_length) {
            char next_char = *(result + airscan_length);
            if (next_char != ' ' && next_char != '\t' && next_char != '\r' && next_char != '\n') {
                return false;
            }
        }

        return true;
    }

    return false;
}

void ScanSettingsFromXML(const char* xmlString, pappl_client_t *client)
{
ScanSettingsXml scanSettings;
initScanSettingsXml(&scanSettings, xmlString);

char* versionPattern = "<pwg:Version>([^<]*)</pwg:Version>";
char* version = getString(&scanSettings, "Version", versionPattern);

char* intentPattern = "<scan:Intent>([^<]*)</scan:Intent>";
char* intent = getString(&scanSettings, "Intent", intentPattern);

char* heightPattern = "<pwg:Height>([^<]*)</pwg:Height>";
char* height = getString(&scanSettings, "Height", heightPattern);

char* contentRegionUnitsPattern = "<pwg:ContentRegionUnits>([^<]*)</pwg:ContentRegionUnits>";
char* contentRegionUnits = getString(&scanSettings, "ContentRegionUnits", contentRegionUnitsPattern);

char* widthPattern = "<pwg:Width>([^<]*)</pwg:Width>";
double width = getNumber(&scanSettings, "Width", widthPattern);

char* xOffsetPattern = "<pwg:XOffset>([^<]*)</pwg:XOffset>";
double xOffset = getNumber(&scanSettings, "XOffset", xOffsetPattern);

char* yOffsetPattern = "<pwg:YOffset>([^<]*)</pwg:YOffset>";
double yOffset = getNumber(&scanSettings, "YOffset", yOffsetPattern);

char* inputSourcePattern = "<pwg:InputSource>([^<]*)</pwg:InputSource>";
char* inputSource = getString(&scanSettings, "InputSource", inputSourcePattern);

char* colorModePattern = "<scan:ColorMode>([^<]*)</scan:ColorMode>";
char* colorMode = getString(&scanSettings, "ColorMode", colorModePattern);

char* blankPageDetectionPattern = "<scan:BlankPageDetection>([^<]*)</scan:BlankPageDetection>";
char* blankPageDetection = getString(&scanSettings, "BlankPageDetection", blankPageDetectionPattern);

free(version);
free(intent);
free(height);
free(contentRegionUnits);
free(inputSource);
free(colorMode);
free(blankPageDetection);
free(scanSettings.xml);
}


