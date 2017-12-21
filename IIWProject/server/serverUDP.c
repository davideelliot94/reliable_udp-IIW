/*
 * serverUDP.c
 * 
 * Copyright 2017 Davide Salvadore <davide@parrot>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <signal.h>

#define MAXWIN 90
#define SERV_PORT 5193
#define MAXLINE 1024

struct sigaction act;
 
static void pSigHandler(int signo){
	
    switch (signo) {
            case SIGUSR1:
            printf("Segnale ricevuto\n");
            fflush(stdout);
            break;
    }
}

void funz_error(char*mexerr)
{	
	puts("###########################################################");
	perror(mexerr);
	puts("###########################################################");
	exit(-1);	
}

void get_funz(char*filename,struct sockaddr_in* addr,int sockfds,size_t addrsize,socklen_t * dimaddr)
{
		char buff[MAXLINE] = {};
		struct stat stat_buf;
		int fd,bytes_sent = 0,rcsend = 0,rcread = 0;
		unsigned int tmpread = 0,totsend = 0;
		struct flock lock;
		size_t fsize;
		memset(&lock,0,sizeof(lock));
		lock.l_type = F_RDLCK;
		int nsent = 0;
		
		fprintf(stderr,"Rievuta richiesta di inviare il file: '%s'\n",filename);

		/*apro il file da mandare*/
		
		fd = open(filename,O_RDONLY);
		if(fd == -1)
		{
			char*err = "BAD_REQUEST";
			sendto(sockfds,err,sizeof(err),0,(struct sockaddr*)addr,addrsize);
			funz_error("Impossibile aprire \n");	
		}
		fcntl(fd,F_SETLKW,&lock);

		fstat(fd,&stat_buf);
		fsize = stat_buf.st_size;
		sprintf(buff,"%zu",fsize);
	 
		printf("dim file %zu\n",fsize);
		bytes_sent = sendto(sockfds, buff,sizeof(buff),0,(struct sockaddr*)addr,addrsize);/*invio la dim del file*/
		puts("dim file inviata\n");
		if(bytes_sent == -1)
		{	
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW, &lock);
			funz_error("Errore durante l'invio della dimensione del file\n");
		}
		
		
		int count = 0;
		while(totsend != fsize)
		{	
			
			if(count%MAXWIN == 0 && count != 0)
			{
				int reccount = 0;
				while(reccount != nsent)
				{
					int ack = recvfrom(sockfds,buff,sizeof(buff),0,(struct sockaddr*)addr,dimaddr);
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
				fcntl(fd,F_SETLKW, &lock);
				funz_error("Errore lettura file");
			}
			tmpread += rcread;
			printf("contenuto %s\n",buff);			
			if(fsize - totsend > sizeof(buff))
				rcsend = sendto(sockfds,buff,sizeof(buff), 0,(struct sockaddr*)addr,addrsize);
			else
				rcsend = sendto(sockfds,buff,fsize-totsend, 0,(struct sockaddr*)addr,addrsize);				
			if(rcsend == -1)
			{	
				lock.l_type = F_UNLCK;
				fcntl(fd,F_SETLKW, &lock);
				funz_error("Errore durante l'invio del file\n");
			}
			totsend += rcsend;
			count++;
			nsent++;		
		
		}
		
		int reccount = 0;
		while(reccount != nsent)
		{
			int ack = recvfrom(sockfds,buff,sizeof(buff),0,(struct sockaddr*)addr,dimaddr);
			if(ack == 0)
				puts("Ack non arrivato");
			else
				printf("Ack -> %d ricevuto \n",reccount);
			reccount++;
		}	
		nsent = 0;
		
		printf("\n\ntnpread è %d\n\n mentre st_size è %zu\n\n ed totsend è %d \n\n",tmpread,stat_buf.st_size,totsend);	
		if(totsend != stat_buf.st_size)
		{
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW, &lock);
			puts("###########################################################");
			funz_error("\t>Trasferimento incompleto\n");
			puts("###########################################################");
		}
		
		
		
		lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&lock);
		close(fd);
		kill(getppid(),SIGUSR1);
		puts("###########################################################");
		puts("\t>File trasferito con successo\n");
		puts("###########################################################");
		exit(EXIT_SUCCESS);
}
		



void list_funz(struct sockaddr_in* addr,int sockfds,size_t addrsize)
{
	
	char buff[MAXLINE] = {};
	struct stat stat_buf;
	struct dirent *dir_p;
	struct flock lock;
	int fd,bytes_sent = 0,rcread = 0, totsend = 0, rcsend = 0;
	unsigned int tmpread = 0;
	int count_elem = 0;
	size_t fsize;
	puts("ricevuto comando LIST");

	memset(&lock,0,sizeof(lock));
	DIR *dp;
	dp = opendir("./");
	
	if ( dp == NULL )
		exit(1);
	fd = open("listafile",O_CREAT | O_WRONLY, 0644);
	if(fd == -1)
		funz_error("Error opening file\n");
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLKW,&lock);
	
	while( ( dir_p = readdir(dp) ) != NULL )
	{
		if(strcasecmp(dir_p -> d_name,".") == 0 || strcasecmp(dir_p -> d_name,"..") == 0 || strcasecmp(dir_p -> d_name,"listafile") == 0)
		{
			continue;
		}
		count_elem++;
		printf("\t%s \n",dir_p -> d_name);
		int w = write(fd,dir_p -> d_name,strlen(dir_p -> d_name));
		if(w < 0 )
		{
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("error writing on file\n");
		}
		int u = write(fd,"\n",strlen("\n"));
		if( u < 0 )
		{
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Error writing on file\n");
		}
			
	}
	closedir(dp);
	
	if(count_elem == 0)
	{
		lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&lock);
		close(fd);
		char*err = "NO_ELEM";
		sendto(sockfds,err,sizeof(err),0,(struct sockaddr*)addr,addrsize);
		puts("###########################################################");
		printf("\t>NO_ELEM, il server attualmente è vuoto\n");
		puts("###########################################################");
		exit(EXIT_SUCCESS);
	}
	
	fstat(fd,&stat_buf);
	fsize = stat_buf.st_size;
	sprintf(buff,"%zu",fsize);
 
	printf("dim file %zu\n",fsize);
	bytes_sent = sendto(sockfds, buff,sizeof(buff),0,(struct sockaddr*)addr,addrsize);/*invio la dim del file*/
	puts("dim file inviata\n");
	if(bytes_sent == -1)
	{
		lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&lock);
		funz_error("Errore durante l'invio della dimensione del file\n");
	}
	lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&lock);
	close(fd);
	fd = open("listafile",O_RDONLY);
	if(fd == -1)
		funz_error("Error opening file\n");
	lock.l_type = F_RDLCK;
	fcntl(fd,F_SETLKW,&lock);
	
	while(tmpread != fsize)
	{
		rcread = read(fd,buff,sizeof(buff));
		printf("ho letto %d bytes\n",rcread);
		if(rcread == -1)
		{
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Errore lettura file");
		}
		tmpread += rcread;
		printf("contenuto %s\n",buff);			
		if(fsize - totsend > sizeof(buff))
			rcsend = sendto(sockfds,buff,sizeof(buff), 0,(struct sockaddr*)addr,addrsize);
		else
			rcsend = sendto(sockfds,buff,fsize-totsend, 0,(struct sockaddr*)addr,addrsize);				
		if(rcsend == -1)	
		{
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Errore durante l'invio del file\n");
		}
		totsend += rcsend;
		
	}
	if(totsend != stat_buf.st_size)
	{
		lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&lock);
		funz_error("Trasferimento incompleto\n");
	}
	lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&lock);
	close(fd);	
	puts("###########################################################");
	puts("\t>Comando eseguito con successo\n");
	puts("###########################################################");
	exit(EXIT_SUCCESS);
		
}


void post_funz(char*filename,struct sockaddr_in* addr,socklen_t * dimaddr,int sockfds,size_t addrsize)
{
		char buff[MAXLINE] = {};
		int fd,nread = 0;
		int tmp = 0;
		long int dim;
		struct flock lock;
		memset(&lock,0,sizeof(lock));
			
		fd = open(filename, O_CREAT | O_WRONLY, 0644);
		if(fd < 0 )
			funz_error("Error opening file\n");
		
		lock.l_type = F_WRLCK;
		fcntl(fd,F_SETLKW,&lock);
		int r = recvfrom(sockfds,buff,sizeof(buff),0,(struct sockaddr*)addr,dimaddr);
		if(r == -1)
		{
			lock.l_type = F_UNLCK;
			fcntl(fd,F_SETLKW,&lock);
			funz_error("Dimensione non arrivata\n");
		}
		char*ptr;
		dim = strtol(buff,&ptr,10);
		printf("Ho ricevuto dim in char %s\n",buff);
		printf("dim file ricevuta %zu\n",dim);
		puts("entro nel while");
		
		while(tmp != dim)
		{	
			puts("sto per leggere\n");
			nread = recvfrom(sockfds,buff,sizeof(buff),0,(struct sockaddr*)addr,dimaddr);
			if(nread < 0)
			{
				lock.l_type = F_UNLCK;
				fcntl(fd,F_SETLKW,&lock);
				funz_error("Error in recvfrom\n");
			}
			printf("Ho ricevuto cose %s\n",buff);
			printf("Ho letto nread = %d \n",nread);
			int w = write(fd,buff,nread);
			if(w < 0 )
			{
				lock.l_type = F_UNLCK;
				fcntl(fd,F_SETLKW,&lock);
				funz_error("Error writing on file\n");
			}
			tmp += nread;
			sendto(sockfds,"ACK_RECEIVED",sizeof("ACK_RECEIVED"),0,(struct sockaddr*)addr,addrsize);
		}
		puts("###########################################################");
		printf("\t>File ricevuto\n");
		puts("###########################################################");
		kill(getppid(),SIGUSR1);
		lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLKW,&lock);
		close(fd);
		exit(EXIT_SUCCESS);
}


int main(int argc, char **argv)
{
	
	/* fase di inizializzazione --> */
	int sockfds;
	struct sockaddr_in addr, cli_addr;
	socklen_t dimaddr;	
	signal(SIGUSR1,pSigHandler);
	memset (&act, '\0', sizeof(act));
	act.sa_handler = &pSigHandler;
	sigaction(SIGUSR1,&act,NULL);
	
	if((sockfds = socket(AF_INET,SOCK_DGRAM,0))<0) /*creo socket server*/
		funz_error("Error during socket creation\n");
	puts("socket assegnata");
	memset((void*)&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(SERV_PORT);
	
	if(bind(sockfds,(struct sockaddr*)&addr,sizeof(addr)) < 0) /*associa socket a indirizzo locale e porta locale*/
		funz_error("Error assigning address to the socket\n");
	puts("connessione della socket effettuata");
	dimaddr = sizeof(sockfds);
	
	/* <--- */
	
	
	/* Job del server --->  */
	while(1)
	{
		
		/* lettura comando */
		
		char line[MAXLINE] = {};
		puts("attendo di ricevere il comando");
		int b = recvfrom(sockfds,line,sizeof(line),0,(struct sockaddr*)&addr,&dimaddr);
		if(b <= 0 )
			funz_error("Error while receiving command\n");
		printf("Il comando ricevuto è : %s\n",line);
		
		/* esecuzione comando tramite processo figlio ---> */
		pid_t pid = fork();
		if(pid == 0)
		{
			int i = 0;
			char command[MAXLINE] = {};
			char filename[MAXLINE] = {};
			
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
			
			filename[strlen(filename)] = '\0';
			if(filename[strlen(filename)-1] == '\n' )
				filename[strlen(filename)-1] = '\0';
			if(filename[strlen(filename)-1] == '\r' )
				filename[strlen(filename)-1] = '\0';
			
			
			/* chiamata a funzione get */
			if(strcasecmp(command,"GET") == 0)
			{
				char * ip_address = inet_ntoa(cli_addr.sin_addr);
				printf("Ricevuta richiesta da client con IP: %s\n", ip_address);
				get_funz(filename,&addr,sockfds,sizeof(addr),&dimaddr);
			}
			/* chiamata a funzione list*/
			if(strcasecmp(command,"LIST") == 0)
			{   
				char * ip_address = inet_ntoa(cli_addr.sin_addr);
				printf("Ricevuta richiesta da client con IP: %s\n", ip_address);
				list_funz(&addr,sockfds,sizeof(addr));
			}
			/* chiamata a funzione post */
			if(strcasecmp(command,"POST") == 0)
			{
				
				char * ip_address = inet_ntoa(cli_addr.sin_addr);
				printf("Ricevuta richiesta da client con IP: %s\n", ip_address);
				post_funz(filename,&addr,&dimaddr,sockfds,sizeof(addr));
				
			}
			
		}
		if(pid < 0 )
			funz_error("Error spawning process\n");
		/*il server attende che il proc figlio abbia finito le comunicazioni in lettura
		 * altrimenti rischia di sovrapporsi */
		if(pid > 0)
			waitpid(pid,NULL,WUNTRACED); // POTREBBE DARE ERRORI ATTENZIONE
	}
	argc = argc;
	argv = argv;
	close(sockfds);
	return EXIT_SUCCESS;
}

