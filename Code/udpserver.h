typedef enum errors {
    No_errors,
    No_Permission
} Errors;

int createFile(char * path, FILE ** file);

Errors doCommand(char * req, int lenReq);