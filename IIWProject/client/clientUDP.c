#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>
#include <dirent.h>
#define SERV_PORT 5193

void funz_error(char*mexerr,int type)
{	
	if(type == 0 )
	{
		perror(mexerr);
		exit(1);
	}
	if(type == 1 )
	{
		perror(mexerr);
		exit(-1);	
	}
}

void get_funz(char*command,char*filename,struct sockaddr* serv_addr,socklen_t* sock_len,size_t dim_serv,int sockfd)
{
	char nread = 0, buff[1024] = {};
	int fd ;
	unsigned int tmp = 0;
	size_t dim;
	
	strcpy(buff,command);
	printf("invio il comando -%s-\n",buff);
	
    if(sendto(sockfd,buff,strlen(buff),0,serv_addr,dim_serv)<=0)
    {
        funz_error("Error in send\n",1);
    }
    puts("comando inviato...attendo");
    
    strcpy(buff,filename);
    if(sendto(sockfd,buff,strlen(buff),0,serv_addr,dim_serv)<0)
    {
        funz_error("Error in send\n",1);
    }
    puts("nome file inviato");
     
    fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if(fd < 0 )
    {
        funz_error("Error opening file\n",0);
    }
    
    int r = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,sock_len);
    if(r == -1)
        funz_error("Dimensione non arrivata\n",0);
    dim = atoi(buff);
    printf("Ho ricevuto dim in char %s\n",buff);
    printf("dim file ricevuta %zu\n",dim);
    puts("entro nel while");
    while(tmp != dim)
    {
        puts("attendo il server");
        while((nread = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,sock_len)) > 0 ){
            printf("Ho ricevuto cose %s\n",buff);
            int w = write(fd,buff,nread);
            if(w < 0 )
				funz_error("Error writing on file\n",0);
            tmp += nread;
           }
       
    }

    printf("File ricevuto\n");
    close(fd);
	
}

void post_funz(char*command,char*filename,struct sockaddr* serv_addr,size_t dim_serv,int sockfd)
{
	char buff[1024] = {};
	int fd ;
	size_t fsize;
	struct stat stat_buf;
	int rc, bytes_sent;	
	
	strcpy(buff,command);
	printf("invio il comando -%s-\n",buff);
	
    if(sendto(sockfd,buff,strlen(buff),0,serv_addr,dim_serv)<=0)
    {
        funz_error("Error in send\n",1);
    }
    puts("comando inviato...attendo");
    
    strcpy(buff,filename);
    if(sendto(sockfd,buff,strlen(buff),0,serv_addr,dim_serv)<0)
    {
        funz_error("Error in send\n",1);
    }
    puts("nome file inviato");
    fd = open(filename,O_RDONLY);
	if(fd == -1)
		funz_error("Impossibile aprire \n",0);	

	fstat(fd,&stat_buf);
	fsize = stat_buf.st_size;
	sprintf(buff,"%zu",fsize);
	 
	printf("dim file %zu\n",fsize);
	bytes_sent = sendto(sockfd, buff,sizeof(buff),0,serv_addr,dim_serv);/*invio la dim del file*/
	puts("dim file inviata\n");
	if(bytes_sent == -1)
		funz_error("Errore durante l'invio della dimensione del file\n",0);
	
	
	int controllo = read(fd,buff,fsize);
	printf("contenuto %s\n",buff);
	if(controllo == -1)
		funz_error("Errore lettura file\n",0);
	rc = sendto(sockfd,buff,strlen(buff), 0,serv_addr,dim_serv);
	if(rc == -1)	
		funz_error("Errore durante l'invio del file\n",0);
	if(rc != stat_buf.st_size)
		funz_error("Trasferimento incompleto\n",0);

}

void list_funz(char*command,struct sockaddr*serv_addr,socklen_t* sock_len,size_t dim_serv,int sockfd)
{
	char nread = 0, buff[1024] = {};
	unsigned int tmp = 0;
	size_t dim;
	
	puts("sono dentro funzione list");
	
	strcpy(buff,command);
	printf("invio il comando -%s-\n",buff);
	
    if(sendto(sockfd,buff,strlen(buff),0,serv_addr,dim_serv)<=0)
    {
        funz_error("Error in send\n",1);
    }
    puts("comando inviato...attendo");
	int r = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,sock_len);
    if(r == -1)
        funz_error("Dimensione non arrivata\n",0);
    dim = atoi(buff);
    printf("Ho ricevuto dim in char %s\n",buff);
    printf("dim file ricevuta %zu\n",dim);
    puts("entro nel while");
    while(tmp != dim)
    {
        puts("attendo il server");
        while((nread = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,sock_len)) > 0 ){
            printf("%s\n",buff);
            tmp += nread;
           }
       
    }

    printf("File ricevuto\n");

	
}


int main(int argc, char *argv[])
{
	int sockfd; /* file descriptor socket client e file a ricevere/inviare*/
	char*command = argv[2];
   	char *filename = argv[3]; /*nome file da ricevere/inviare*/
    struct sockaddr_in servaddr; /*indirizzo del server*/;
    socklen_t dimaddr = sizeof(sockfd);
	
	if(argc < 2)
		funz_error("Usage :./clientUDP.c <hostname> <command> <filename>\n",0);
	
	printf(">Comando ricevuto -- %s\n\n", command);
	
	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
		funz_error("Error in socket\n",1);
    
	memset((void*)&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port= htons(SERV_PORT);
	if(inet_pton(AF_INET,argv[1],&servaddr.sin_addr) <= 0)
		funz_error("Error , inet_pton failed\n",1);
	
	puts("inet_pton superata");
	
	if(strcasecmp(command,"GET") == 0)
		get_funz(command,filename,(struct sockaddr*)&servaddr,&dimaddr,sizeof(servaddr),sockfd);
	if(strcasecmp(command,"POST") == 0)
		post_funz(command,filename,(struct sockaddr*)&servaddr,sizeof(servaddr),sockfd);
	if(strcasecmp(command,"LIST") == 0)
		list_funz(command,(struct sockaddr*)&servaddr,&dimaddr,sizeof(servaddr),sockfd);
	
	return 0;
}
