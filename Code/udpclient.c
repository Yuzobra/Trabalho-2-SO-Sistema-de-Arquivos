/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/param.h>

#define BUFSIZE 1024


// Prototipo das funcoes estaticas
void interpretInput(char * resBuf);


/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    interpretInput(buf);

 

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    /* print the server's reply */
    
    n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");
    
    printf("Message from server: ");
    for(int i = 0; i < n; i++) {
      printf("%c",buf[i]);
    }
    printf("\n");
    return 0;
}




void interpretInput(char * resBuf) {
  char aux[BUFSIZE];
  char idUser[5];
  char offset[5];
  char lenText[5];
  int inputType, len;
  char path[MAXPATHLEN];
  int k, c;
    
  bzero(resBuf, BUFSIZE);
  
  printf("Please choose type:\n1-File Write\n2-File Read\n3-File Description\n4-Create directory\n5-Directory remove\n6-Directory List\n");
  scanf("%d", &inputType);
  switch (inputType)
  {
  case 1:
    strcpy(resBuf,"WR-REQ");

    // Tratar input do usuario
    //Id usuário:
    printf("Insira o seu ID:");
    scanf("%s", idUser); 


    // Preencher de 0 no inicio até conter 4 bytes:
    if(strlen(idUser) < 4) {
      char tempBuf[5];
      for(k = 0; k < 4 ; k++) {
        if(k < 4 - strlen(idUser)){
          tempBuf[k] = '0';
        }
        else {
          tempBuf[k] = idUser[k - (4-strlen(idUser))];
        }
      }
      tempBuf[k] = '\0';
      strcpy(idUser,tempBuf); // Armazenar para adicionar ao payload na hora certa
    }

    // Path:
    printf("Insira o path do arquivo: ");
    scanf("%s", path);
    if(path[0] != '/') /* Verificar se o path comeca com '/' */ {
      strcpy(aux, path);
      strcpy(path, "/");
      strcat(path, aux);
    }
    if(path[strlen(path)-1] != '/') /* Verificar se o path termina com '/' */ {
      len = strlen(path);
      path[len] = '/';
      path[len+1] = '\0';
    }
    len = strlen(path);
 
    // printf("resbuf em path: %s\n", resBuf);

    //Nome do arquivo:
    printf("Insira o nome do arquivo: ");
    scanf("%s", aux);
    len += strlen(aux);
    strcat(path, aux);

    sprintf(aux, "%d", len);

    strcat(resBuf, aux);
    strcat(resBuf, path);
 
    // printf("resbuf em nome: %s\n", resBuf);

    //Adicionar id ao buf
    strcat(resBuf, idUser);

    //Permissoes
    printf("Escolha a sua permissao(0, R ou W): ");
    scanf("%s", aux);
    while(strcmp(aux, "0") != 0 && strcmp(aux, "R") != 0 && strcmp(aux, "W") != 0) {
      printf("Opcao invalida, escolha a sua permissao(0, R ou W): ");
      scanf("%s", aux);
    }
    strcat(resBuf, aux);


    printf("Escolha a permissao dos outros usuarios(0, R ou W): ");
    scanf("%s", aux);
    while(strcmp(aux, "0") != 0 && strcmp(aux, "R") != 0 && strcmp(aux, "W") != 0) {
      printf("Opcao invalida, escolha a permissao dos outros usuarios(0, R ou W): ");
      scanf("%s", aux);
    }
    strcat(resBuf, aux);


    //Offset:
    printf("Insira o offset do texto:");
    scanf("%s", offset); 


    // Preencher de 0 no inicio até conter 4 bytes:
    if(strlen(offset) < 4) {
      char tempBuf[5];
      for(k = 0; k < 4 ; k++) {
        if(k < 4 - strlen(offset)){
          tempBuf[k] = '0';
        }
        else {
          tempBuf[k] = offset[k - (4-strlen(offset))];
        }
      }
      tempBuf[k] = '\0';
      strcpy(offset,tempBuf); // Armazenar para adicionar ao payload na hora certa
    }

    
    // Texto:
    printf("Insira o texto a ser escrito:\n\n");
    
    //flush stdin
    while ((c = getchar()) != '\n' && c != EOF);

    fgets(aux, BUFSIZE, stdin);
    if ((strlen(aux) > 0) && (aux[strlen (aux) - 1] == '\n'))
        aux[strlen (aux) - 1] = '\0';
    // printf("LEN: %d\nTEXTO: %s\n", strlen(aux), aux);
    sprintf(lenText, "%d", strlen(aux));
    // Preencher de 0 no inicio até conter 4 bytes:
    if(strlen(lenText) < 4) {
      char tempBuf[5];
      for(k = 0; k < 4 ; k++) {
        if(k < 4 - strlen(lenText)){
          tempBuf[k] = '0';
        }
        else {
          tempBuf[k] = lenText[k - (4-strlen(lenText))];
        }
      }
      tempBuf[k] = '\0';
      strcpy(lenText,tempBuf); // Armazenar para adicionar ao payload na hora certa
    }

    //Adicionar tamanho do texto:
    strcat(resBuf, lenText);
    
    //Adicionar offset
    strcat(resBuf, offset);

    //adicionar text
    strcat(resBuf, aux);

    printf("Payload: %s\n", resBuf);
    break;
  case 2:
    /* code */
    break;
  case 3:
    /* code */
    break;
  case 4:
    /* code */
    break;
  case 5:
    /* code */
    break;
  case 6:
    /* code */
    break;
  
  default:
    printf("Acao invalida\n");
    break;
  }
  
  
  // fgets(resBuf, BUFSIZE, stdin);


}