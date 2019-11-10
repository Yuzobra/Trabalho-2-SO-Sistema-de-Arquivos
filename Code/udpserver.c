/*
 * udpserver.c - A simple UDP echo server
 * usage: udpserver <port>
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

#include "udpserver.h"

#define MAXFILESIZE 20009 // offset: 9999  && strlen: 9999 && METADASIZE
#define METADATASIZE 11
#define BUFSIZE 1024

char __base__[MAXPATHLEN]; 


// Prototipos de funcoes estaticas

static int file_select(const struct direct *entry);
static int parse (char *buf, int *cmd, char *name);


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

    parse(buf, &cmd, name);
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

    printf("%d\n",doCommand(buf, strlen(buf)));





    /*
     * sendto: echo the input back to the client
     */
    n = sendto(sockfd, buf, strlen(buf), 0,
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
      error("ERROR in sendto");
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
      // printf("%c\n", req[i]);
    }
    aux[k] = '\0';
    lenPath = atoi(aux);
    k = 0;
    for(; k < lenPath; i++) {
      path[k++] = req[i]; 
      // printf("%c\n", req[i]);
    
    }
    path[k] = '\0';
    // printf("PATH: %s\n", path);
    
    //Verificar se arquivo existe:
    if(createFile(path, &fp) == 1) /* Arquivo foi criado agora */ {
      
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
            // printf("ELSE: %c\n", aux[k - (5 - strlen(aux))]);
            tempBuf[k] = aux[k - (5-strlen(aux))];
          }
        }
        tempBuf[k] = '\0';
        strcpy(aux,tempBuf);
        printf("Tempbuf: %s\n",tempBuf);
      }
      printf("nrbytes: %d\nOffset: %d\nAux: %s\n",nrbytes,offset, aux);

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

      printf("ReqData: %s\n", reqData);
      printf("fileData: %s\n", fileData);


      // Verificar permissão de escrita
      if(strcmp(reqData, fileData) == 0) /* O request vem do dono do arquivo */ {
        printf("Dono do arquivo\n");

        if(aux[4] != 'W') {
          fclose(fp);
          return No_Permission;
        }
      }
      else /* O request vem de outra usuário */ {
        printf("Outro usuario\n");
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

      printf("Existing bytes: %d\nOffset: %d\nnrBytes: %d\n",nrExistingBytes,offset,nrbytes);

      // Se a nova string ultrapassar o tamanho atual do arquivo:
      if(offset + nrbytes > nrExistingBytes) {
        char totalBytes[5];
        printf("é maior\n");

        sprintf(totalBytes, "%d", offset+nrbytes);
        
        // Preencher os espaços vazios
        if(offset + nrbytes > nrExistingBytes) {
          fseek(fp, METADATASIZE + nrExistingBytes, SEEK_SET);
          for(i = 0; i < offset - nrExistingBytes + 1; i++) {
            fwrite(" ", sizeof(char), 1, fp);
          }
        }
// WR-REQ25/Pasta/Pasta2/NomeArq.txt0020WR00010002C
// WR-REQ25/Pasta/Pasta2/NomeArq.txt0020WR00180015ConteudoDoArquivo1

        // Alterar nos metadados
        fseek(fp, 6+(5-strlen(totalBytes)), SEEK_SET);
        fwrite(totalBytes, sizeof(char), strlen(totalBytes), fp);
      }

    }

    // Escrever no arquivo
    // printf("vou escrever\n");
    // fseek(fp, METADATASIZE+30, SEEK_SET);
    // printf("%d\n",fwrite("aaa", sizeof(char), 3, fp)); // JA ESTA SOBREESCREVENDO NA POSICAO DO FSEEK, SO DESCOBRIR O OFFSET PASSADO NO REQUEST
    fseek(fp, METADATASIZE + offset, SEEK_SET);
    fwrite(&req[contentIndex], sizeof(char), strlen(&req[contentIndex]), fp);



    fclose(fp);
  }
  return No_errors;
}



int createFile(char * path, FILE ** file) {
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
      if (count <= 0) /* Pasta vazia */{ 
        // printf("No files in this directory, creating new directory\n");
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
          strcat(cwd, "/");
          strcat(cwd, file_name);
          mkdir(cwd, 0777);
        }
      }
    }

    else {
      file_name[k++] = path[i];
    }

  }

  file_name[k] = 0;

  count = scandir( cwd, &files, file_select, alphasort);  
  if (count <= 0) /* Pasta vazia */{ 
    FILE * fp;
    // printf("No files in this directory, creating new file\n");
    strcat(cwd, "/");
    strcat(cwd, file_name);

    fp = fopen(&path[1], "w");

      //ESCREVER METADADOS DO ARQUIVO: DONO, PERMISSOES ETC

    *file = fp;
    return 1;
  }   

  else /* Pasta não está vazia, é preciso percorrer os arquivos */{
    FILE * fp;
    // printf("Pasta nao esta vazia\n");
    for (file_num=1;file_num<count+1;++file_num) {
      strcpy(curr_file, cwd);
      strcat(curr_file, "/");
      strcat(curr_file, files[file_num-1]->d_name);
      stat(curr_file, &file_info);
      
      if(S_ISREG(file_info.st_mode) && strcmp(files[file_num-1]->d_name, file_name) == 0) /* Achou o arquivo, retornar erro */ {
        fp = fopen(&path[1], "r+");
        *file = fp;
        
        return 0;
      }
    }

    // Criar arquivo e retornar true
    fp = fopen(&path[1], "w");
    *file = fp;
    return 1;

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


static int parse (char *buf, int *cmd, char *name) {
    char *cmdstr;

    cmdstr = strtok(buf," ");
        name = strtok(NULL,"\0");
    cmd = atoi(cmdstr);
}

