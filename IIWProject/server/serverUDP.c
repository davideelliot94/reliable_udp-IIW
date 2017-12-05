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
#define SERV_PORT 5193
#define MAXLINE 1024

void funz_error(char*mexerr)
{	
	perror(mexerr);
	exit(-1);	
}

void get_funz(struct sockaddr_in* addr,socklen_t * dimaddr,int sockfds,size_t addrsize)
{
		char filename[1024] = {};
		char buff[1024] = {};
		struct stat stat_buf;
		int fd,bytes_sent = 0,rc;
		size_t fsize;
	/*ricevo il nome del file da client*/
		puts("attendo nome del file");
		int recv = recvfrom(sockfds,filename,sizeof(filename),0,(struct sockaddr*)addr,dimaddr);
		if(recv < 0 )
			funz_error("Nome file non ricevuto\n");
		puts("nome file ricevuto");
		filename[recv] = '\0';
		if(filename[strlen(filename)-1] == '\n' )
			filename[strlen(filename)-1] = '\0';
		if(filename[strlen(filename)-1] == '\r' )
			filename[strlen(filename)-1] = '\0';

		fprintf(stderr,"Rievuta richiesta di inviare il file: '%s'\n",filename);

	/*apro il file da mandare*/

		fd = open(filename,O_RDONLY);
		if(fd == -1)
			funz_error("Impossibile aprire \n");	

		fstat(fd,&stat_buf);
		fsize = stat_buf.st_size;
		sprintf(buff,"%zu",fsize);
	 
		printf("dim file %zu\n",fsize);
		bytes_sent = sendto(sockfds, buff,sizeof(buff),0,(struct sockaddr*)addr,addrsize);/*invio la dim del file*/
		puts("dim file inviata\n");
		if(bytes_sent == -1)
			funz_error("Errore durante l'invio della dimensione del file\n");
	
	
		int controllo = read(fd,buff,fsize);
		printf("contenuto %s\n",buff);
		if(controllo == -1)
			funz_error("Errore lettura file");
		rc = sendto(sockfds,buff,strlen(buff), 0,(struct sockaddr*)addr,addrsize);
		if(rc == -1)	
			funz_error("Errore durante l'invio del file\n");
		if(rc != stat_buf.st_size)
			funz_error("Trasferimento incompleto\n");
	
		close(fd);
				
}


void list_funz(struct sockaddr_in* addr,int sockfds,size_t addrsize)
{
		
		char buff[1024] = {};
		struct stat stat_buf;
		struct dirent *dir_p;
		int fd,bytes_sent = 0,rc;
		size_t fsize;
		puts("ricevuto comando LIST");
		DIR *dp;
		
		dp = opendir("/home/casa/Scrivania/iiwproj/IIWProject/server");
		
		if ( dp == NULL )
			exit(1);
		fd = open("listafile",O_CREAT | O_WRONLY, 0644);
		while( ( dir_p = readdir(dp) ) != NULL )
		{
				
			int w = write(fd,dir_p -> d_name,strlen(dir_p -> d_name));
			if(w < 0 )
				funz_error("error writing on file\n");
			int u = write(fd,"\n",strlen("\n"));
			if( u < 0 )
				funz_error("Error writing on file\n");
				
		}
		closedir(dp);

		fstat(fd,&stat_buf);
		fsize = stat_buf.st_size;
		sprintf(buff,"%zu",fsize);
	 
	 	printf("dim file %zu\n",fsize);
		bytes_sent = sendto(sockfds, buff,sizeof(buff),0,(struct sockaddr*)addr,addrsize);/*invio la dim del file*/
		puts("dim file inviata\n");
		if(bytes_sent == -1)
			funz_error("Errore durante l'invio della dimensione del file\n");
		close(fd);
		char content[1024] = {};
		fd = open("listafile",O_RDONLY);
		int controllo = read(fd,content,fsize);
		printf("contenuto %s\n",content);
		if(controllo == -1)
			funz_error("Errore lettura file");
		rc = sendto(sockfds,content,strlen(content), 0,(struct sockaddr*)addr,addrsize);
		if(rc == -1)	
			funz_error("Errore durante l'invio del file\n");
		if(rc != stat_buf.st_size)
			funz_error("Trasferimento incompleto\n");
		
		close(fd);
				
}


void post_funz(struct sockaddr_in* addr,socklen_t * dimaddr,int sockfds)
{
		char filename[1024] = {};
		char buff[1024] = {};
		int fd,nread = 0;
		unsigned int tmp = 0;
		size_t dim;
	/*DEVO RICEVERE NOMEFILE*/
		int f = recvfrom(sockfds,filename,sizeof(filename),0,(struct sockaddr*)addr,dimaddr);
		if(f < 0 )
			funz_error("Error namefile non ricevuto");
		filename[f] = '\0';
		if(filename[strlen(filename)-1] == '\n' )
			filename[strlen(filename)-1] = '\0';
		if(filename[strlen(filename)-1] == '\r' )
			filename[strlen(filename)-1] = '\0';
			
		fd = open(filename, O_CREAT | O_WRONLY, 0644);
		if(fd < 0 )
		{
			funz_error("Error opening file\n");
		}
    
		int r = recvfrom(sockfds,buff,sizeof(buff),0,(struct sockaddr*)addr,dimaddr);
		if(r == -1)
			funz_error("Dimensione non arrivata\n");
		dim = atoi(buff);
		printf("Ho ricevuto dim in char %s\n",buff);
		printf("dim file ricevuta %zu\n",dim);
		puts("entro nel while");
		while(tmp != dim)
		{
			puts("attendo il server");
			while((nread = recvfrom(sockfds,buff,sizeof(buff),0,(struct sockaddr*)addr,dimaddr)) > 0 ){
				printf("Ho ricevuto cose %s\n",buff);
				int v = write(fd,buff,nread);
				if( v < 0 )
					funz_error("Error writing on file\n");
				tmp += nread;
			}
		}
		printf("File ricevuto\n");
		close(fd);
			
}


int main(int argc, char **argv)
{
	int sockfds;
	struct sockaddr_in addr, cli_addr;
	socklen_t dimaddr;	

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
	
	while(1)
	{
		char command[1024] = {};
		
		puts("attendo di ricevere il comando");
		int comm = recvfrom(sockfds,command,sizeof(command),0,(struct sockaddr*)&addr,&dimaddr);
		if(comm <= 0 )
			funz_error("dio carol\n");
		printf("Il comando ricevuto è : %s\n",command);

		if(strcasecmp(command,"GET") == 0)
		{
		
			get_funz(&addr,&dimaddr,sockfds,sizeof(addr));
			char * ip_address = inet_ntoa(cli_addr.sin_addr);
			printf("IP del client: %s\n", ip_address);
	
		}
	
		if(strcasecmp(command,"LIST") == 0)
		{   
			
			list_funz(&addr,sockfds,sizeof(addr));
			char * ip_address = inet_ntoa(cli_addr.sin_addr);
			printf("IP del client: %s\n", ip_address);
			
		}
		
		if(strcasecmp(command,"POST") == 0)
		{
		
			post_funz(&addr,&dimaddr,sockfds);
			char * ip_address = inet_ntoa(cli_addr.sin_addr);
			printf("IP del client: %s\n", ip_address);
		
		}
			
	}
	argc = argc;
	argv = argv;
	close(sockfds);
	return EXIT_SUCCESS;
}

