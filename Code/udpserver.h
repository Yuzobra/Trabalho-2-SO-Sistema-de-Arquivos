typedef enum errors {
    No_errors,
    No_Permission,
    File_Does_Not_Exist,
    Out_of_Bounds,
    Dir_Already_Exists,
    Dir_Not_Empty 
} Errors;

typedef enum modes {
    Read,
    Write,
    Append,
    Dir_Create,
    Dir_Remove
} Modes;

typedef enum reqTypes {
    File_Write,
    File_Read
} reqTypes;


typedef struct filesPosition {
    int ini;
    int fim;
} FilesPosition;

Errors getFileDescriptor(char * path, FILE ** file, Modes mode);

Errors doCommand(char * req, int lenReq);