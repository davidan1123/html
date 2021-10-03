extern dav	dav;

int listenfd, connfd, num_sent=0;
socklen_t clilen;
struct sockaddr_in cliaddr, servaddr;
long save_fd;
int h264_conn_status=H264_TCP_WAIT_FOR_CONNECT;
  
void setup_h264_tcp_server(void)
{
 //creation of the socket
 listenfd = socket (AF_INET, SOCK_STREAM, 0);

 //preparation of the socket address
 servaddr.sin_family = AF_INET;
 //servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
 servaddr.sin_port = htons(SERV_PORT);
 
 int reuse = 1;
 if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
 {
    perror("Server: setsockopt(SO_REUSEADDR) failed\n");
    log_printf("Server: setsockopt(SO_REUSEADDR) failed\n");
    return;
 }
 
  #ifdef SO_REUSEPORT
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
  {
    perror("Server: setsockopt(SO_REUSEPORT) failed\n");
    log_printf("Server: setsockopt(SO_REUSEPORT) failed\n");
    return;
  }
  #endif
 
 if(bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
 { 
//    perror("Server: error binding\n");
    log_printf("Server: error binding\n");  
    //return;
 } 

 listen (listenfd, LISTENQ);

 save_fd = fcntl( listenfd, F_GETFL );
 save_fd |= O_NONBLOCK;
 fcntl( listenfd, F_SETFL, save_fd );
 

// fprintf(stderr,"%s\n","Server running...waiting for connections.");
 log_printf("Server running...waiting for connections.\n");  
}


void tcp_poll_connect(void)
{
    if((h264_conn_status == H264_TCP_WAIT_FOR_CONNECT))
    {  
      clilen = sizeof(cliaddr);
      connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
    
      if (connfd <=0)
      {
        if (dav.debug)
          printf("Poll: Kein Client am Socket ...\n");
      }
      else
      {
        h264_conn_status=H264_TCP_SEND_HEADER; //must send header
        num_sent=0;
//        fprintf (stderr, "Server: connect from host %s, port %u.\n",
//                    inet_ntoa (cliaddr.sin_addr),
//                    ntohs (cliaddr.sin_port));
        log_printf("Server: connect from host %s, port %u.\n",
                    inet_ntoa (cliaddr.sin_addr),
                    ntohs (cliaddr.sin_port));            
      }
    }      
}


void tcp_send_h264_header(void *data, int len)
  {
     if(h264_conn_status != H264_TCP_WAIT_FOR_CONNECT)
      {        
        if(h264_conn_status == H264_TCP_SEND_HEADER) 
        {
          num_sent=send(connfd, data, len, MSG_NOSIGNAL);
          h264_conn_status=H264_TCP_SEND_DATA;
          if (dav.debug)
            printf("write h264 header:%d \n",len);
          
         if (num_sent < 0 || num_sent !=len) 
          {
            perror("Server: Client connection closed\n");
            log_printf("Server: Client connection closed\n");
            shutdown(connfd, SHUT_RDWR); 
            close(connfd);
            h264_conn_status=H264_TCP_WAIT_FOR_CONNECT;
          }
        }
      }
  }
  
 void tcp_send_h264_data(char * what, void *data, int len)
  {
    if(h264_conn_status != H264_TCP_WAIT_FOR_CONNECT)
    {  
      if(h264_conn_status == H264_TCP_SEND_DATA)
      {
        num_sent=send(connfd, data,len, MSG_NOSIGNAL);
        if (dav.debug)
          printf("write tcp %s:%d \n",what, len);
        if (num_sent < 0 || num_sent !=len) 
        {
          perror("Server: Client connection closed\n");
          log_printf("Server: Client connection closed\n");
          close(connfd);
          h264_conn_status=H264_TCP_WAIT_FOR_CONNECT;
          return;
        }
      }        
  
    }
  }
  
  
  



