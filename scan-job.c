#include "pappl-private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <time.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <regex.h>

static const char* SCAN_NONE = "None";
static const char* SCAN_SERVICE_OFF_LINE = "ServiceOffLine";
static const char* SCAN_RESOURCES_ARE_NOT_READY = "ResourcesAreNotReady";
static const char* SCAN_JOB_QUEUED = "JobQueued";
static const char* SCAN_JOB_SCANNING = "JobScanning";
static const char* SCAN_JOB_SCANNING_AND_TRANSFERRING = "JobScanningAndTransferring";
static const char* SCAN_JOB_COMPLETED_SUCCESSFULLY = "JobCompletedSuccessfully";
static const char* SCAN_JOB_CANCELED_BY_USER = "JobCanceledByUser";
static const char* SCAN_INVALID_SCAN_TICKET = "InvalidScanTicket";
static const char* SCAN_UNSUPPORTED_DOCUMENT_FORMAT = "UnsupportedDocumentFormat";
static const char* SCAN_DOCUMENT_PERMISSION_ERROR = "DocumentPermissionError";
static const char* SCAN_ERRORS_DETECTED = "ErrorsDetected";

typedef enum State
{
  aborted,
  canceled,
  completed,
  pending,
  processing
} State;

typedef enum Kind
{
  single,
  adfBatch,
  adfSingle
} Kind;

typedef struct {
    char* xml;
} papplScanSettingsXML;

papplScanSettingsXML* new_ScanSettingsXml(const char* s) {
    papplScanSettingsXML* scanSettings = (papplScanSettingsXML*)malloc(sizeof(papplScanSettingsXML));

    if(scanSettings == NULL)
    {
        return NULL;
    }

    scanSettings->xml = strdup(s); 
    return scanSettings;
}

char* getString(papplScanSettingsXML* scanSettings, const char* name) {
    xmlDocPtr doc;
    xmlNodePtr cur;

    doc = xmlParseMemory(scanSettings->xml, strlen(scanSettings->xml));

    if (doc == NULL ) {
        fprintf(stderr,"Document not parsed successfully.\n");
        return NULL;
    }

    cur = xmlDocGetRootElement(doc);

    if (cur == NULL) {
        fprintf(stderr,"empty document\n");
        xmlFreeDoc(doc);
        return NULL;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)name))) {
            char* content = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            xmlFreeDoc(doc);
            return content;
        }
        cur = cur->next;
    }

    xmlFreeDoc(doc);
    return NULL;
}
