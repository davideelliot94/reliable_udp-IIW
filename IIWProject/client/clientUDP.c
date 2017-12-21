#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>
#include <dirent.h>

#define MAXWIN 90
#define SERV_PORT 5193
#define MAXLINE 1024

void funz_error(char*mexerr,int type)
{	
	if(type == 0 )
	{
		puts("###########################################################");
		perror(mexerr);
		puts("###########################################################");
		exit(1);
	}
	if(type == 1 )
	{
		puts("###########################################################");
		perror(mexerr);
		puts("###########################################################");
		exit(-1);	
	}
}

void get_funz(char*command,char*filename,struct sockaddr* serv_addr,socklen_t* sock_len,size_t dim_serv,int sockfd)
{
	int nread = 0;
	int tmp = 0;
	char buff[MAXLINE] = {};
	int fd ;
	long int dim;
	struct flock lock;
	
	
	memset(&lock,0,sizeof(lock));
	strcpy(buff,command);
	printf("invio il comando -%s-\n",buff);
	
    if(sendto(sockfd,buff,sizeof(buff),0,serv_addr,dim_serv)<=0)
    {
        funz_error("Error in send\n",1);
    }
    puts("comando inviato...attendo");
    
    int r = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,sock_len);
    if(r == -1)
        funz_error("Dimensione non arrivata\n",0);
    if(strcasecmp(buff,"BAD_REQUEST")==0)
    {
		puts("###########################################################");
		printf("BAD_REQUEST il file richiesto non è presente sul server\n");
		puts("###########################################################");
		exit(EXIT_FAILURE);
	}  
          
    fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if(fd < 0 )
    {
        funz_error("Error opening file\n",0);
    }   
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLKW,&lock);
       
    char*ptr;
    dim = strtol(buff,&ptr,10);
    printf("Ho ricevuto dim in char %s\n",buff);
    printf("dim file ricevuta %zu\n",dim);
    puts("entro nel while");
    printf("La dim del buffer è %ld\n",sizeof(buff));
    

    while(tmp != dim)
    {	
		
		puts("sto per leggere\n");
        nread = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,sock_len);
        if(nread < 0)
        {
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Error in recvfrom\n",0);
		}
        printf("Ho ricevuto cose %s\n",buff);
        printf("Ho letto nread = %d \n",nread);
        int w = write(fd,buff,nread);
        if(w < 0 )
		{
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Error writing on file\n",0);
		}
		tmp += nread;
		sendto(sockfd,"ACK_RECEIVED",sizeof("ACK_RECEIVED"),0,serv_addr,dim_serv);
    }
    puts("###########################################################");
    printf("File ricevuto\n");
    puts("###########################################################");
    lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&lock);
    close(fd);
    exit(EXIT_SUCCESS);
	
}

void post_funz(char*command,char*filename,struct sockaddr* serv_addr,size_t dim_serv,int sockfd,socklen_t*dimaddr)
{
	char buff[MAXLINE] = {};
	int fd ;
	size_t fsize;
	struct stat stat_buf;
	int rcread = 0,rcsend = 0, bytes_sent = 0;
	unsigned int tmpread = 0,totsend = 0;
	struct flock lock;
	int nsent = 0;
	
	memset(&lock,0,sizeof(lock));
	strcpy(buff,command);
	printf("invio il comando -%s-\n",buff);
	
    if(sendto(sockfd,buff,sizeof(buff),0,serv_addr,dim_serv)<=0)
    {
        funz_error("Error in send\n",1);
    }
    puts("comando inviato...attendo");
    
    fd = open(filename,O_RDONLY);
	if(fd == -1)
		funz_error("Impossibile aprire \n",0);	
	
	lock.l_type = F_RDLCK;
	fcntl(fd,F_SETLKW,&lock);
	
	fstat(fd,&stat_buf);
	fsize = stat_buf.st_size;
	sprintf(buff,"%zu",fsize);
	 
	printf("dim file %zu\n",fsize);
	bytes_sent = sendto(sockfd, buff,sizeof(buff),0,serv_addr,dim_serv);/*invio la dim del file*/
	puts("dim file inviata\n");
	if(bytes_sent == -1)
	{
		lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&lock);
		funz_error("Errore durante l'invio della dimensione del file\n",0);
	}
	
	int count = 0;
	while(totsend != fsize)
	{
		if(count%MAXWIN == 0 && count != 0)
		{
				int reccount = 0;
				while(reccount != nsent)
				{
					int ack = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,dimaddr);
					if(ack == 0)
						puts("Ack non arrivato");
					else
						printf("Ack -> %d ricevuto \n",reccount);
					reccount++;
				}	
				nsent = 0;
					
		}
		
		rcread = read(fd,buff,sizeof(buff));
		printf("ho letto %d bytes\n",rcread);
		if(rcread == -1)
		{
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Errore lettura file",1);
		}
		tmpread += rcread;
		printf("contenuto %s\n",buff);
		/* scrivo su file locale */ 
		if(fsize - totsend > sizeof(buff))
			rcsend = sendto(sockfd,buff,sizeof(buff), 0,serv_addr,dim_serv);
		else
			rcsend = sendto(sockfd,buff,fsize-totsend, 0,serv_addr,dim_serv);				
		if(rcsend == -1)
		{	
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Errore durante l'invio del file\n",1);
		}
		totsend += rcsend;	
		count++;
		nsent++;
	}
	
	int reccount = 0;
	while(reccount != nsent)
	{
		int ack = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,dimaddr);
		if(ack == 0)
			puts("Ack non arrivato");
		else
			printf("Ack -> %d ricevuto \n",reccount);
		reccount++;
	}	
	nsent = 0;
	
	if(totsend != stat_buf.st_size)
	{
		lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&lock);
		funz_error("\t>Trasferimento incompleto\n",0);
		
	}
	puts("###########################################################");
	puts("\t>File inviato correttamente\n");
	puts("###########################################################");
	lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&lock);
	close(fd);
	exit(EXIT_SUCCESS);
}

void list_funz(char*command,struct sockaddr*serv_addr,socklen_t* sock_len,size_t dim_serv,int sockfd)
{
	char buff[MAXLINE] = {};
	int nread = 0,fd;
	unsigned int tmp = 0;
	size_t dim;
	struct flock lock;
	
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
    if(strcasecmp(buff,"NO_ELEM") == 0)
    {
		puts("###########################################################");
		printf("NO_ELEM, il server attualmente è vuoto\n");
		puts("###########################################################");
		exit(EXIT_SUCCESS);	
	}
	
    dim = atoi(buff);
    printf("Ho ricevuto dim in char %s\n",buff);
    printf("dim file ricevuta %zu\n",dim);
    puts("entro nel while");
    fd = open("inServer",O_CREAT | O_WRONLY,0644);
    if(fd == -1)
		funz_error("Error opening file\n",1);
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLKW,&lock);
    while(tmp != dim)
    {	
		puts("sto per leggere\n");
        nread = recvfrom(sockfd,buff,sizeof(buff),0,serv_addr,sock_len);
        if(nread < 0)
        {
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Error in recvfrom\n",0);
		}
        printf("Ho ricevuto cose %s\n",buff);
        printf("Ho letto nread = %d \n",nread);
        int w = write(fd,buff,nread);
        if(w < 0 )
        {
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Error writing on file\n",0);
		}
		tmp += nread;
    }
    printf("File ricevuto con successo\n");
    lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&lock);
	close(fd);
	
	FILE*fil = fopen("inServer","r");
    puts("###########################################################");
	while(!feof(fil))
	{
		char buff[MAXLINE];
		char*cc = fgets(buff,sizeof(buff),fil);
		if(cc == NULL)
			funz_error("Error reading file\n",0);
		printf("\t> %s \n",buff);
	}
    puts("###########################################################");
    
    fclose(fil);
	exit(EXIT_SUCCESS);
	
}


int main(int argc, char *argv[])
{
	int sockfd; /* file descriptor socket client e file a ricevere/inviare*/ /*nome file da ricevere/inviare*/
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
		
		int i = 0;
		char line[MAXLINE] = {};
		char command[MAXLINE] = {};
		char filename[MAXLINE] = {};
		char* c = fgets(line,1024,stdin);
		pid_t pid = fork();
		if(pid == 0)
		{	
			printf("Ricevuto comando %s \n",line);
			if( c == NULL )
				funz_error("Error in fgets\n",1);
			
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
					puts("CHILD PROCESS, EXEGUTING GET COMMAND!!!!");
					get_funz(line,filename,(struct sockaddr*)&servaddr,&dimaddr,sizeof(servaddr),sockfd);
			}
			if(strcasecmp(command,"POST") == 0)
			{
					puts("CHILD PROCESS, EXEGUTING POST COMMAND!!!!");
					post_funz(line,filename,(struct sockaddr*)&servaddr,sizeof(servaddr),sockfd,&dimaddr);

			}
			if(strcasecmp(command,"LIST") == 0)
			{

					puts("CHILD PROCESS, EXEGUTING LIST COMMAND!!!!");
					list_funz(line,(struct sockaddr*)&servaddr,&dimaddr,sizeof(servaddr),sockfd);
			}
			if(strcasecmp(command,"CLOSE") == 0)
			{
				puts("Chiusura applicazione in corso\n");
				exit(EXIT_SUCCESS);
			}
			else
				puts("Comando sconosciuto, si prega di ridigitare\n");
		}
		if(pid < 0 )
			funz_error("Error spawning process\n",1);
	}
	return 0;
}
