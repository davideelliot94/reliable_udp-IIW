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
	int nread = 0;
	unsigned int tmp = 0;
	char buff[1024] = {};
	int fd ;
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
    
    int r = recvfrom(sockfd,buff,strlen(buff)*sizeof(char),0,serv_addr,sock_len);
    if(r == -1)
        funz_error("Dimensione non arrivata\n",0);
    dim = atoi(buff);
    printf("Ho ricevuto dim in char %s\n",buff);
    printf("dim file ricevuta %zu\n",dim);
    puts("entro nel while");
    printf("La dim del buffer è %d\n",sizeof(buff));
    
    while(tmp != dim)
    {	
		puts("sto per leggere\n");
        nread = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,sock_len);
        if(nread < 0)
			funz_error("Error in recvfrom\n",0);
	
        printf("Ho ricevuto cose %s\n",buff);
        printf("Ho letto nread = %d \n",nread);
        int w = write(fd,buff,nread);
        if(w < 0 )
			funz_error("Error writing on file\n",0);
		tmp += nread;
    }
    printf("File ricevuto\n");
    close(fd);
    exit(EXIT_SUCCESS);
	
}

void post_funz(char*command,char*filename,struct sockaddr* serv_addr,size_t dim_serv,int sockfd)
{
	char buff[1024] = {};
	int fd ;
	size_t fsize;
	struct stat stat_buf;
	int rcread = 0,rcsend = 0, bytes_sent = 0,totsend = 0;	
	unsigned int tmpread = 0;
	
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
	
	
	while(tmpread != fsize)
	{
		rcread = read(fd,buff,sizeof(buff));
		printf("ho letto %d bytes\n",rcread);
		if(rcread == -1)
			funz_error("Errore lettura file",1);
		tmpread += rcread;
		printf("contenuto %s\n",buff);
		/* scrivo su file locale */ 
		if(fsize - totsend > sizeof(buff))
			rcsend = sendto(sockfd,buff,sizeof(buff), 0,serv_addr,dim_serv);
		else
			rcsend = sendto(sockfd,buff,fsize-totsend, 0,serv_addr,dim_serv);				
		if(rcsend == -1)	
			funz_error("Errore durante l'invio del file\n",1);
		totsend += rcsend;
			
	}
	
	if(totsend != stat_buf.st_size)
		funz_error("Trasferimento incompleto\n",0);
	
	close(fd);
	exit(EXIT_SUCCESS);
}

void list_funz(char*command,struct sockaddr*serv_addr,socklen_t* sock_len,size_t dim_serv,int sockfd)
{
	char buff[1024] = {};
	int nread = 0,fd;
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
    fd = open("inServer",O_CREAT | O_WRONLY,0644);
    while(tmp != dim)
    {	
		puts("sto per leggere\n");
        nread = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,sock_len);
        if(nread < 0)
			funz_error("Error in recvfrom\n",0);
	
        printf("Ho ricevuto cose %s\n",buff);
        printf("Ho letto nread = %d \n",nread);
        int w = write(fd,buff,nread);
        if(w < 0 )
			funz_error("Error writing on file\n",0);
		tmp += nread;
    }
    printf("File ricevuto\n");
    close(fd);
	exit(EXIT_SUCCESS);
	
}


int main(int argc, char *argv[])
{
	int sockfd; /* file descriptor socket client e file a ricevere/inviare*/
	char line[1024];
	char command[1024];
   	char filename[1024]; /*nome file da ricevere/inviare*/
    struct sockaddr_in servaddr; /*indirizzo del server*/;
    socklen_t dimaddr = sizeof(sockfd);
	
	if(argc < 2)
		funz_error("Usage :./clientUDP.c <hostname> \n",0);
	
	
	
	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
		funz_error("Error in socket\n",1);
    
	memset((void*)&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port= htons(SERV_PORT);
	if(inet_pton(AF_INET,argv[1],&servaddr.sin_addr) <= 0)
		funz_error("Error , inet_pton failed\n",1);
	
	puts("inet_pton superata");
	
	
	while(!feof(stdin))
	{
		char* c = fgets(line,1024,stdin);
		printf("Ricevuto comando %s \n",line);
		if( c == NULL )
			funz_error("Error in fgets\n",1);
		int i = 0;
		while(1)
		{
			command[i] = line[i];
			i++;
			if(line[i] == '\n')
			{
				command[i] = '\0';
				break;
			}
			if(line[i] == ' ')
			{
				command[i] = '\0';
				int j = 0;
				i++;
				while(line[i] != '\n')
				{
					filename[j] = line[i];
					j++;
					i++;
				}
				break;	
			}	
		}
		puts("uscito dal while");
		printf("il comando è : %s \n" , command);
		if(strcasecmp(command,"GET") == 0)
		{
			pid_t pid = fork();
			if(pid == 0)
			{
				puts("CHILD PROCESS, EXEGUTING GET COMMAND!!!!");
				get_funz(command,filename,(struct sockaddr*)&servaddr,&dimaddr,sizeof(servaddr),sockfd);
			}
			if(pid < 0)
				funz_error("Error forking process\n",1);
			if(pid > 0 )
				continue;
		}
		if(strcasecmp(command,"POST") == 0)
		{
			pid_t pid = fork();
			if(pid == 0)
			{
				puts("CHILD PROCESS, EXEGUTING POST COMMAND!!!!");
				post_funz(command,filename,(struct sockaddr*)&servaddr,sizeof(servaddr),sockfd);
			}
			if(pid < 0)
				funz_error("Error forking process\n",1);
		}
		if(strcasecmp(command,"LIST") == 0)
		{
			puts("sono nel ciclo della list");
			pid_t pid = fork();
			if(pid == 0)
			{
				puts("CHILD PROCESS, EXEGUTING LIST COMMAND!!!!");
				list_funz(command,(struct sockaddr*)&servaddr,&dimaddr,sizeof(servaddr),sockfd);
			}
			if(pid < 0)
				funz_error("Error forking process\n",1);
		}
		if(strcasecmp(command,"CLOSE") == 0)
		{
			puts("Chiusura applicazione in corso\n");
			exit(EXIT_SUCCESS);
		}
		else
			puts("Comando sconosciuto, si prega di ridigitare\n");
	}
	return 0;
}
