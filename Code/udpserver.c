/* 
* Nome      - Yuri Zoel Brasil
* Matricula - 1710730
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/dir.h> 
#include <sys/param.h>
#include <sys/stat.h> 
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>

#include "udpserver.h"

#define MAXFILESIZE 20009 // offset: 9999  && strlen: 9999 && METADASIZE
#define METADATASIZE 11
#define BUFSIZE 1024

char pathCpy[MAXPATHLEN];
char __base__[MAXPATHLEN]; 
char clientResponse[100];


// Prototipos de funcoes estaticas

static int file_select(const struct direct *entry);
static int parse (char *buf, int *cmd, char *name);
int VerificaPermRec(char * path, char * idUser); /* Retorna 0 se NAO existirem arquivo que o usuario nao puder deletar, senao retorna um valor maior que 0 */ 
void deleteDir(char * path); // Deletes directory
void deleteAllUnderDir(char * path); /* Deletes everything under path, without looking at permissions */
/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}


int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  char name[BUFSIZE];   // name of the file received from client
  int cmd;              // cmd received from client
  Errors status;        // Return of doCommand function
  FilesPosition positions[40];

  if (getcwd(__base__, MAXPATHLEN) == NULL )  {  
      printf("Error getting base directory\n"); 
      exit(0);   
  }


  /*
   * check command line arguments
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /*
   * socket: create the parent socket
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    error("ERROR on binding");




  /*
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
    (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");


    // parse(buf, &cmd, name);

    /*
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n",
	   hostp->h_name, hostaddrp);

    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);


    // INTERPRETA COMANDO RECEBIDO AQUI

    status = doCommand(buf, strlen(buf));
    if(status == No_errors) {
      n = sendto(sockfd, clientResponse, strlen(clientResponse), 0,
          (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0)
        error("ERROR in sendto");
      printf("Number of sent bytes: %d\n", n);
    }
    else if(status == No_Permission) {
      strcpy(clientResponse, "No Permission to perform this action");
      n = sendto(sockfd, clientResponse, strlen(clientResponse), 0,
                 (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0)
        error("ERROR in sendto");
    }

    else if (status == Out_of_Bounds) {
      strcpy(clientResponse, "Request exceeds file size");
      n = sendto(sockfd, clientResponse, strlen(clientResponse), 0,
                 (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0)
        error("ERROR in sendto");
    }
    
    else if(status == File_Does_Not_Exist) {
      strcpy(clientResponse, "File does not exist");
      n = sendto(sockfd, clientResponse, strlen(clientResponse), 0,
                 (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0)
        error("ERROR in sendto");
    }

    else if(status == Dir_Already_Exists) {
      strcpy(clientResponse, "Directory already exists");
      n = sendto(sockfd, clientResponse, strlen(clientResponse), 0,
                 (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0)
        error("ERROR in sendto");
    }

    else if(status == Dir_Not_Empty){
      strcpy(clientResponse, "Directory not empty");
      n = sendto(sockfd, clientResponse, strlen(clientResponse), 0,
                 (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0)
        error("ERROR in sendto");
    }
    
    else if(status == Dir_Empty) {
      strcpy(clientResponse, "Directory empty");
      n = sendto(sockfd, clientResponse, strlen(clientResponse), 0,
                 (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0)
        error("ERROR in sendto");
    }
    strcpy(clientResponse, "");
  }
}


/*
*
* Funçoes externadas
*
*/

Errors doCommand(char * req, int lenReq) {
  char aux[BUFSIZE];
  char fileContents[MAXFILESIZE];
  int i, k = 0;
  int offset, nrbytes, contentIndex;



  for(i = 0; i < 6; i++) {
    aux[i] = req[i];
  }
  aux[i] = '\0';

  if(strcmp(aux, "WR-REQ") == 0)  /* Escrever arquivo */  {
    printf("Write Request\n");

    FILE * fp;
    int lenPath;
    char path[MAXPATHLEN];
    // Pegar len do path
    for(i = 6; req[i] != '/'; i++ ) {
      aux[k++] = req[i];
    }
    aux[k] = '\0';
    lenPath = atoi(aux);
    k = 0;
    for(; k < lenPath; i++) {
      path[k++] = req[i]; 
    
    }
    path[k] = '\0';
    
    
    //Verificar se arquivo existe:
    if(getFileDescriptor(path, &fp, Write) == File_Does_Not_Exist) /* Arquivo foi criado agora */ {
      
      char IDPERMBUF[7] = { 0 };
      /* Metadados:
      * 4 bytes: ID Usuario
      * 2 bytes: Permissoes
      * 5 bytes: Numero de bytes já escritos no arquivo
      */  
      
      for(k=0; k < 6; k++) /* ID + Permissoes */ {
        IDPERMBUF[k] = req[i];
        aux[k] = req[i++];

      }
      aux[k] = '\0';
      IDPERMBUF[6] = '\0';
      fwrite(aux, sizeof(char), 6, fp);  
      
      for(k=0; k < 4; i++) /* NRBytes */ {
        aux[k++] = req[i];
      }
      aux[k] = '\0';
      nrbytes = atoi(aux);

      for(k=0; k < 4; i++) /* Offset */ {
        aux[k++] = req[i];
      }
      aux[k] = '\0';
      offset = atoi(aux);
      contentIndex = i; // Pegar index de inicio da string


      sprintf(aux, "%d", nrbytes + offset);

      // Preencher de 0 no inicio até conter 5 bytes:
      if(strlen(aux) < 5) {
        char tempBuf[6];
        for(k = 0; k < 5 ; k++) {
          if(k < 5 - strlen(aux)){
            tempBuf[k] = '0';
          }
          else {
            tempBuf[k] = aux[k - (5-strlen(aux))];
          }
        }
        tempBuf[k] = '\0';
        strcpy(aux,tempBuf);
      }

      fwrite(aux, sizeof(char), 5, fp);

      // Salvar metadados em AUX:
      k = 0;
      for(i = METADATASIZE - 5; i < METADATASIZE; i++) {
        aux[i] = aux[k++]; 
      }
      

      for(i = 0; i < 6; i++) {
        aux[i] = IDPERMBUF[i];
      }

      aux[METADATASIZE] = '\0';


      // Preencher o espaco ate o offset
      if(offset != 0) {
        fseek(fp, METADATASIZE, SEEK_SET);
        for(i = 0; i < offset; i++) {
          fwrite(" ", sizeof(char), 1, fp);
        }
      }

   
    }

    else /*  Arquivo já existe  */ {
      char reqData[5];
      char fileData[5];
      int nrExistingBytes;
      fread(aux,sizeof(char), METADATASIZE, fp);
      aux[METADATASIZE] = '\0';
      printf("MetaData: %s\n", aux);


      // Pegar o usuáriodo request e do arquivo
      for(k = 0; k < 4; k++) {
        reqData[k] = req[i++];
        fileData[k] = aux[k];
      }
      reqData[k] = '\0';
      fileData[k] = '\0';

      // Verificar permissão de escrita
      if(strcmp(reqData, fileData) == 0) /* O request vem do dono do arquivo */ {
        if(aux[4] != 'W') {
          fclose(fp);
          return No_Permission;
        }
      }
      else /* O request vem de outra usuário */ {
        if(aux[5] != 'W') {
          fclose(fp);
          return No_Permission;
        }
      }

      // Pegar a quantidade de bytes ja escritos no arquivo
      nrExistingBytes = atoi(&aux[strlen(aux) - 5]);

      i += 2;
      // Pegar o Offset e o nrbytes
      for(k=0; k < 4; k++) /* NRBytes */ {
        aux[k] = req[i++];
      }
      aux[k] = '\0';
      nrbytes = atoi(aux);

      for(k=0; k < 4; i++) /* Offset */ {
        aux[k++] = req[i];
      }
      aux[k] = '\0';
      offset = atoi(aux);
      contentIndex = i; // Pegar o index de inicio da string

      // Se a nova string ultrapassar o tamanho atual do arquivo:
      if(offset + nrbytes > nrExistingBytes) {
        char totalBytes[5];

        sprintf(totalBytes, "%d", offset+nrbytes);
        
        // Preencher os espaços vazios
        if(offset + nrbytes > nrExistingBytes) {
          fseek(fp, METADATASIZE + nrExistingBytes, SEEK_SET);
          for(i = 0; i < offset - nrExistingBytes + 1; i++) {
            fwrite(" ", sizeof(char), 1, fp);
          }
        }


        // Alterar nos metadados
        fseek(fp, 6+(5-strlen(totalBytes)), SEEK_SET);
        fwrite(totalBytes, sizeof(char), strlen(totalBytes), fp);
      }
    }
    
    // Escrever no arquivo
    fseek(fp, METADATASIZE + offset, SEEK_SET);

    fwrite(&req[contentIndex], sizeof(char), strlen(&req[contentIndex]), fp);
    fclose(fp);
    
    strcpy(clientResponse, "Written sucessfully");
  // WR-REQ25/Pasta/Pasta2/NomeArq.txt0020WR00010002C
  // WR-REQ25/Pasta/Pasta2/NomeArq.txt0020WR00180015ConteudoDoArquivo1
  // WR-REQ37/Pasta/Pasta2/NMDirectory/NomeArq.txt0020WR00180015ConteudoDoArquivo1
  // WR-REQ37/Pasta/Pasta2/NMDirectory/NomeArq.txt0021WR00180015ConteudoDoArquivo1
  }

  else if(strcmp(aux, "RD-REQ") == 0) {
    printf("Read Request\n");

    FILE * fp;
    int lenPath;
    char path[MAXPATHLEN];
    // Pegar len do path
    for(i = 6; req[i] != '/'; i++ ) {
      aux[k++] = req[i];
    }
    aux[k] = '\0';
    lenPath = atoi(aux);
    k = 0;
    for(; k < lenPath; i++) {
      path[k++] = req[i]; 
    
    }
    path[k] = '\0';
    
    //Verificar se arquivo existe:
    if(getFileDescriptor(path, &fp, Read) == File_Does_Not_Exist) /* Arquivo não existe */ {
      printf("Arquivo nao existe\n");

      return File_Does_Not_Exist;
    }

    else /* Encontrou o arquivo */ {
      printf("Arquivo existe\n");
      char aux[METADATASIZE];
      char reqData[5];
      char fileData[5];
      int nrExistingBytes;
      fread(aux,sizeof(char), METADATASIZE, fp);
      aux[METADATASIZE] = '\0';
      printf("MetaData: %s\n", aux);


      // Pegar o usuáriodo request e do arquivo
      for(k = 0; k < 4; k++) {
        reqData[k] = req[i++];
        fileData[k] = aux[k];
      }
      reqData[k] = '\0';
      fileData[k] = '\0';
      // Verificar permissão de leitura
      if(strcmp(reqData, fileData) == 0) /* O request vem do dono do arquivo */ {
        if(aux[4] == '0') {
          fclose(fp);
          return No_Permission;
        }
      }
      else /* O request vem de outra usuário */ {
        if(aux[5] == '0') {
          fclose(fp);
          return No_Permission;
        }
      }
      // Pegar a quantidade de bytes ja escritos no arquivo
      nrExistingBytes = atoi(&aux[strlen(aux) - 5]);

      // Pegar o Offset e o nrbytes
      for(k=0; k < 4; k++) /* NRBytes */ {
        aux[k] = req[i++];
      }
      aux[k] = '\0';
      nrbytes = atoi(aux);

      for(k=0; k < 4; i++) /* Offset */ {
        aux[k++] = req[i];
      }
      aux[k] = '\0';
      offset = atoi(aux);

      printf("NRBytes: %d\nOffset: %d \nnrExistingBytes: %d\n", nrbytes, offset, nrExistingBytes);
      fseek(fp, METADATASIZE, SEEK_SET);

      // Verificar se o que ele quer ler esta dentro do limite do arquivo:
      if(offset > nrExistingBytes) /* O comeco da leitura vem depois do final do arquivo */ {
        printf("Out of bounds\n");
        fclose(fp);
        return Out_of_Bounds; 
      }

      else if(offset + nrbytes > nrExistingBytes) /* O comeco da leitura existe, mas o usuario requisita mais bytes do que existem */ {
        char fileContents[MAXFILESIZE];
        fseek(fp, offset, SEEK_CUR);
        fread(fileContents, sizeof(char) ,nrExistingBytes - offset, fp);

        printf("Conteudo: %s\n", fileContents);
        strcpy(clientResponse, fileContents);
      }

      else /* A leitura pode ocorrer normalmente */ {
        char fileContents[MAXFILESIZE];
        fseek(fp, offset, SEEK_CUR);
        fread(fileContents, sizeof(char) ,nrbytes, fp);

        printf("Conteudo: %s\n", fileContents);
        strcpy(clientResponse, fileContents);
      }

      fclose(fp);
    }
  // RD-REQ25/Pasta/Pasta2/NomeArq.txt002000100015
  }

  else if(strcmp(aux, "FD-REQ") == 0) {
    printf("File Delete Request\n");

    FILE * fp;
    int lenPath;
    char path[MAXPATHLEN];
    // Pegar len do path
    for(i = 6; req[i] != '/'; i++ ) {
      aux[k++] = req[i];
    }
    aux[k] = '\0';
    lenPath = atoi(aux);
    k = 0;
    for(; k < lenPath; i++) {
      path[k++] = req[i]; 
    
    }
    path[k] = '\0';
    strcpy(pathCpy, path);

    //Verificar se arquivo existe:
    if(getFileDescriptor(path, &fp, Read) == File_Does_Not_Exist) /* Arquivo não existe */ {
      printf("Arquivo nao existe\n");

      return File_Does_Not_Exist;
    }

    else /* Encontrou o arquivo */ {
      printf("Arquivo existe\n");
      char aux[MAXPATHLEN];
      char reqData[5];
      char fileData[5];
      fread(aux,sizeof(char), METADATASIZE, fp);
      aux[METADATASIZE] = '\0';
      printf("MetaData: %s\n", aux);


      // Pegar o usuáriodo request e do arquivo
      for(k = 0; k < 4; k++) {
        reqData[k] = req[i++];
        fileData[k] = aux[k];
      }
      reqData[k] = '\0';
      fileData[k] = '\0';
      // Verificar permissão de escrita
      if(strcmp(reqData, fileData) == 0) /* O request vem do dono do arquivo */ {
        if(aux[4] != 'W') {
          fclose(fp);
          return No_Permission;
        }
      }
      else /* O request vem de outra usuário */ {
        if(aux[5] != 'W') {
          fclose(fp);
          return No_Permission;
        }
      }

      strcpy(aux, __base__);
      strcat(aux, pathCpy);

      if(remove(aux) == 0)
        printf("arquivo deletado\n");
      else
        printf("erro ao deletar\n");
      strcpy(clientResponse, "Arquivo deletado");
      fclose(fp);
    }
  // RD-REQ25/Pasta/Pasta2/NomeArq.txt002000100015
  }

  else if(strcmp(aux, "FI-REQ") == 0) {
    printf("Information Request\n");

    FILE * fp;
    int lenPath;
    char path[MAXPATHLEN];
    // Pegar len do path
    for(i = 6; req[i] != '/'; i++ ) {
      aux[k++] = req[i];
    }
    aux[k] = '\0';
    lenPath = atoi(aux);
    k = 0;
    for(; k < lenPath; i++) {
      path[k++] = req[i]; 
    
    }
    path[k] = '\0';
    
    //Verificar se arquivo existe:
    if(getFileDescriptor(path, &fp, Read) == File_Does_Not_Exist) /* Arquivo não existe */ {
      printf("Arquivo nao existe\n");

      return File_Does_Not_Exist;
    }
    else /* Encontrou o arquivo */ {
      printf("Arquivo existe\n");
      char aux[METADATASIZE];
      char owner[5];
      char permissions[3];
      char fileData[5];
      char nrExistingBytes[6];
      fread(aux,sizeof(char), METADATASIZE, fp);
      aux[METADATASIZE] = '\0';
      printf("MetaData: %s\n", aux);


      // Pegar o usuáriodo do arquivo
      for(k = 0; k < 4; k++) {
        fileData[k] = aux[k];
      }
      fileData[k] = '\0';
      strcpy(owner,fileData); // Pegar o dono do arquivo
      i = 0;
      for(k = 4; k < 6; k++) {
        fileData[i++] = aux[k];
      }
      fileData[i] = '\0';
      strcpy(permissions, fileData);


      // Pegar a quantidade de bytes ja escritos no arquivo
      strcpy(nrExistingBytes,&aux[strlen(aux) - 5]);

      strcpy(clientResponse, "Id do dono: ");
      strcat(clientResponse, owner);
      strcat(clientResponse, "\nPermissoes: ");
      strcat(clientResponse,permissions);
      strcat(clientResponse, "\nQuantidade de bytes escritos no arquivo: ");
      strcat(clientResponse, nrExistingBytes);
      fclose(fp);
    }
  // FI-REQ25/Pasta/Pasta2/NomeArq.txt
  // FI-REQ37/Pasta/Pasta2/NMDirectory/NomeArq.txt
  }

  else if(strcmp(aux, "DC-REQ") == 0) /* Criar um diretório */ {
    printf("Directory create request\n");

    FILE * fp;
    int lenPath;
    char path[MAXPATHLEN];
    // Pegar len do path
    for(i = 6; req[i] != '/'; i++ ) {
      aux[k++] = req[i];
    }
    aux[k] = '\0';
    lenPath = atoi(aux);
    k = 0;
    for(; k < lenPath; i++) {
      path[k++] = req[i]; 
    
    }
    path[k] = '\0';
    
    //Verificar se arquivo existe:
    if(getFileDescriptor(path, &fp, Dir_Create) == No_errors) /* Diretorio foi criado com sucesso */ {
      strcpy(clientResponse, "Directory created with sucess\n");
      printf("Diretorio criado\n");
    }

    else /* Directory name already in use */ {
      return Dir_Already_Exists;
    }
  //DC-REQ25/Pasta/Pasta2/NMDirectory
  }

  else if(strcmp(aux, "DR-REQ") == 0) /* Remover um diretorio (Somente se houver arquivos criados pelo usuario) */ {
    printf("Directory remove request\n");
    Errors status;
    FILE * fp;
    int lenPath;
    char path[MAXPATHLEN];
    // Pegar len do path
    for(i = 6; req[i] != '/'; i++ ) {
      aux[k++] = req[i];
    }
    aux[k] = '\0';
    lenPath = atoi(aux);
    k = 0;
    for(; k < lenPath; i++) {
      path[k++] = req[i]; 
    
    }
    path[k] = '\0';
    
    
    status = getFileDescriptor(path, &fp, Dir_Remove);
    if(status == No_errors) {
      char idReq[5];
      idReq[0] = req[i++];
      idReq[1] = req[i++];
      idReq[2] = req[i++];
      idReq[3] = req[i++];
      idReq[4] = '\0';

      // printf("idReq: %s\n", idReq);

      if(VerificaPermRec(path, idReq) == 0) /* Diretorio pode ser removido com sucesso */ {
        
        deleteDir(path);

        strcpy(clientResponse, "Directory removed with sucess\n");
        printf("Diretorio deletado\n");
      } 

      else {
        strcpy(clientResponse, "Permission denied: You don't have ownership over some file under this directory\n");
        printf("Permissao negada\n");
      }
    }

    else if(status == Dir_Not_Empty)/* Directory not empty */ {
      return Dir_Not_Empty;
    }

    else /* Directory does not exist */ {
      return File_Does_Not_Exist;
    }
  //DR-REQ25/Pasta/Pasta2/NMDirectory0020
  //DR-REQ13/Pasta/Pasta20020
  }

  else if(strcmp(aux, "DL-REQ") == 0) /* Listar diretorio */ {
    printf("Directory list request\n");
    Errors status;
    FILE * fp;
    int lenPath;
    char path[MAXPATHLEN];
    char bufNames[BUFSIZE];
    char aux[5];
    FilesPosition positions[40];
    DIR *dir;
    struct dirent *entry;

    struct stat buf;
    
    // Pegar len do path
    for(i = 6; req[i] != '/'; i++ ) {
      aux[k++] = req[i];
    }
    aux[k] = '\0';
    lenPath = atoi(aux);
    k = 0;
    for(; k < lenPath; i++) {
      path[k++] = req[i]; 
    
    }
    path[k] = '\0';

    if(!(dir = opendir(&path[1]))) /* Diretorio nao existe */ {
      return File_Does_Not_Exist;
    }
    if(!(entry = readdir(dir))) /* O diretorio esta vazio */ {

      return Dir_Empty;
    }


    i=0;
    k=0;
    memset(bufNames, 0, BUFSIZE);

    do {
        if(entry->d_name[0] != '.') {
          positions[i++].ini = k;
          k += strlen(entry->d_name);
          if(i == 0) {
            strcpy(bufNames, entry->d_name);
            strcat(bufNames, "\n");
          }
          else {
            strcat(bufNames, entry->d_name);
            strcat(bufNames, "\n");
          }
        }
      } while(entry = readdir(dir));

      // Get number of files in directory
      // sprintf(clientResponse, "%d", i);
      
      // // Begin appending index of file names
      // strcat(clientResponse, "/");
      // for(k = 0; k < i; k++) /* i possui o numero de arquivos */ {
      //   sprintf(aux, "%d", positions[k].ini);
      //   strcat(clientResponse, aux);
      //   strcat(clientResponse, ",");
      // }

      strcat(clientResponse, bufNames);
  //DL-REQ25/Pasta/Pasta2/NMDirectory
  //DL-REQ13/Pasta/Pasta2
  }

  return No_errors;
}



Errors getFileDescriptor(char * path, FILE ** file, Modes mode) {
  char file_name[MAXPATHLEN] = { 0 };
  char cwd[MAXPATHLEN]; //Current working directory
  char curr_file[MAXPATHLEN];
  struct stat file_info;
	struct direct **files;

  stpcpy( cwd, __base__);

  int count, i, k = 0, file_num;

  for(i = 1; path[i]; i++) /* Navegar até(ou criar) a pasta */ {

    if(path[i] == '/') /* Pasta */{
      k = 0;
      count = scandir( cwd, &files, file_select, alphasort);  
      if (count <= 0) /* Pasta vazia */ { 
        // printf("No files in this directory, creating new directory\n");

        if(mode == Read || mode == Dir_Remove ) {
          printf("Erro ao tentar criar caminho ate path\n");
          return File_Does_Not_Exist;
        }
        strcat(cwd, "/");
        strcat(cwd, file_name);

        mkdir(cwd, 0777);
      }   

      else /* Pasta não está vazia, é preciso percorrer os arquivos */ {
        for (file_num=1;file_num<count+1;++file_num) {
          strcpy(curr_file, cwd);
          strcat(curr_file, "/");
          strcat(curr_file, files[file_num-1]->d_name);
          stat(curr_file, &file_info);
          
          if(S_ISDIR(file_info.st_mode) && strcmp(files[file_num-1]->d_name, file_name) == 0) /* Achou o diretorio */ {
            strcat(cwd, "/");
            strcat(cwd, files[file_num-1]->d_name);
            break;
          }
        }

        if(file_num == count+1) /* Não encontrou a pasta */{
          if(mode == Read || mode == Dir_Remove){
            printf("Erro ao tentar achar caminho\n");
            return File_Does_Not_Exist;
          }
          strcat(cwd, "/");
          strcat(cwd, file_name);
          mkdir(cwd, 0777);
        }
      }

      memset(file_name, 0, MAXPATHLEN);
    }

    else {
      file_name[k++] = path[i];
    }

  }

  file_name[k] = 0;

  count = scandir( cwd, &files, file_select, alphasort);  
  if (count <= 0) /* Pasta vazia */{ 
    FILE * fp;

    if(mode == Read || mode == Dir_Remove) {
      printf("Pasta vazia sem permissão de criação\n");
      return File_Does_Not_Exist;
    }

    if(mode == Write){
      printf("No files in this directory, creating new file\n");
      strcat(cwd, "/");
      strcat(cwd, file_name);
      fp = fopen(&path[1], "w+");
      *file = fp;
      return File_Does_Not_Exist;
    }
    
    else if (mode == Dir_Create) {
      printf("No files in this directory. Creating new directory\n");
      strcat(cwd, "/");
      strcat(cwd, file_name);
      mkdir(cwd, 0777);
      return No_errors;
    }
  }   

  else /* Pasta não está vazia, é preciso percorrer os arquivos */{
    FILE * fp;

    // printf("Pasta nao esta vazia\n");
    for (file_num=1;file_num<count+1;++file_num) {
      strcpy(curr_file, cwd);
      strcat(curr_file, "/");
      strcat(curr_file, files[file_num-1]->d_name);
      stat(curr_file, &file_info);
      
      if(mode == Write) {
        if(S_ISREG(file_info.st_mode) && strcmp(files[file_num-1]->d_name, file_name) == 0) /* Achou o arquivo */ {
          fp = fopen(&path[1], "r+");
          *file = fp;
          
          return No_errors;
        }
      }
      else if(mode == Dir_Create) {
        if(S_ISDIR(file_info.st_mode) && strcmp(files[file_num-1]->d_name, file_name) == 0) /* Achou o diretorio */ {
          return Dir_Already_Exists;
        }
      }
      else if(mode == Dir_Remove) {
        if(S_ISDIR(file_info.st_mode) && strcmp(files[file_num-1]->d_name, file_name) == 0) /* Achou o diretorio a ser removido */ {
          return No_errors;
        }
      }
      else if(mode == Read) {
        if(S_ISREG(file_info.st_mode) && strcmp(files[file_num-1]->d_name, file_name) == 0) /* Achou o arquivo */ {
          fp = fopen(&path[1], "r+");
          *file = fp;
          
          return No_errors;
        }
      }
    }

    // File of directory does not exist, create it if necessary
    if(mode == Read || mode == Dir_Remove) {
      printf("Nao foi possivel achar path\n");
      return File_Does_Not_Exist;
    }

    
    else if(mode == Dir_Create) /* Criar diretorio */ {
      mkdir(&path[1], 0777);
      return No_errors;
    }

    else /* Criar arquivo e retornar que foi criado agora */ {
      fp = fopen(&path[1], "w");
      *file = fp;
      return File_Does_Not_Exist;
    }
  }
}


/*
*
* Funçoes estaticas
*
*/


static int file_select(const struct direct *entry) {     
	if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) return (0); // false     
		else return (1); //true
}

int VerificaPermRec(char * path, char * idUser) /* Retorna 0 se NAO existirem arquivo que o usuario nao puder deletar, senao retorna um valor maior que 0 */ {
	char pathname[MAXPATHLEN]; //Isto pode ser muito perigoso e pode facilmente estorar a memoria, estou utilizando aqui por questoes de simplicidade
	DIR *dir;
	struct direct **files;
	struct dirent *entry;

	struct stat buf;

  int ver = 0;

	if(!(dir = opendir(&path[1]))) return 0;
	if(!(entry = readdir(dir))) return 0;
	
	do {
		if(entry->d_type == DT_DIR && strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..") != 0 ){
			strcpy(pathname, path);
			strcat(pathname,"/");
			strcat(pathname,entry->d_name);
      
      // printf("Pasta: %s\n", pathname);

			ver += VerificaPermRec(pathname, idUser);
		}
		
		else if(entry->d_type == DT_REG && entry->d_name[0] != '.')/* Verificar id de usuario do arquivo */{
			FILE * fp;
      char idArq[5];
      strcpy(pathname, &path[1]);
			strcat(pathname,"/");
			strcat(pathname,entry->d_name);
			
      // printf("Arquivo: %s\n", pathname);
      fp = fopen(pathname, "r+");      

      fread(idArq, sizeof(char), strlen(idUser), fp);
      idArq[4] = '\0';

      // printf("idArq: %s\n", idArq);
      if(strcmp(idArq, idUser) != 0) /* O usuario nao tem permissao para deletar este arquivo */ {
        return 1;
      }
      fclose(fp);
		}
	} while(entry = readdir(dir));
	
	return ver;	
}

void deleteDir(char * path) {
  char pathname[MAXPATHLEN];
  char aux[MAXPATHLEN];
  deleteAllUnderDir(path);

  strcpy(pathname, path);
  strcpy(aux, __base__);
  strcat(aux, pathname);
  if(rmdir(aux) == -1){   
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error deleting directory: %s\n", strerror( errnum ));
      }
}

void deleteAllUnderDir(char * path) /* Deletes everything under path, without looking at permissions */ {
  char pathname[MAXPATHLEN]; //Isto pode ser muito perigoso e pode facilmente estorar a memoria, estou utilizando aqui por questoes de simplicidade
	char aux[MAXPATHLEN];
  DIR *dir;
	struct direct **files;
	struct dirent *entry;

	struct stat buf;

  int ver = 0;

	if(!(dir = opendir(&path[1]))) return;
	if(!(entry = readdir(dir))) return;
	
	do {
		if(entry->d_type == DT_DIR && strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..") != 0 ){
			
      strcpy(pathname, path);
			strcat(pathname,"/");
			strcat(pathname,entry->d_name);
      
			deleteAllUnderDir(pathname);
      strcpy(aux, __base__);
      strcat(aux, pathname);
      strcpy(pathname,aux);
      
      printf("Pasta: %s\n", pathname);
      if(rmdir(pathname) == -1){   
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error deleting directory: %s\n", strerror( errnum ));
      }
    }
		
		else if(entry->d_type == DT_REG)/* Verificar id de usuario do arquivo */{
			FILE * fp;
      
      strcpy(pathname, &path[1]);
			strcat(pathname,"/");
			strcat(pathname,entry->d_name);
			
      printf("Arquivo: %s\n", pathname);
      remove(pathname);
      
		}
	} while(entry = readdir(dir));
	
}

static int parse (char *buf, int *cmd, char *name) {
    char *cmdstr;

    cmdstr = strtok(buf," ");
    name = strtok(NULL,"\0");
    cmd = atoi(cmdstr);
}