//
//  main.c
//  prog2
//
//  Created by Yangping Zheng on 15/10/23.
//  Copyright © 2015 Yangping Zheng. All rights reserved.
//
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<netdb.h>
#include<stdarg.h>
#include<strings.h>
#include <pthread.h>
#include <sys/uio.h>


#define SERVER_PORT "60450"    // the port users will be connecting to
#define MYPORT "55001"

#define BUFFER_SIZE 1200
#define STR_SIZE 100
#define USERWAIT 5

int register_flag = 0;
int broadcast_flag = 0;
int send_flag = 0;

int FM_number = 0;

char port[50][20];
char ip[50][20];
int addr_index[50];

char recv_buffer[BUFFER_SIZE] = "";
char respond_msg[STR_SIZE] = "";

int thread_recv_flag = 1;

struct sockaddr_in server_addr;
int sockfd;

int i = 0;

//void quitthread(char *msg, int fd) {
//    printf(msg);
//    fflush(stdout);
//    close(fd);
//    pthread_exit(NULL);
//}

int get_index(int addr_index[50], int num) {
    //    printf("addr_index:%d %d %d %d\n",addr_index[0],addr_index[1],addr_index[2],addr_index[3]);
    //    printf("addr_index:");
    //    printf("addr_index_size:%d ",(int)sizeof(addr_index));
    for (int i = 0; i < 50; i++) {
        //        printf("%d ", addr_index[i]);
        if (addr_index[i] == num) {
            //            printf("chechto: %d\n ", i);
            return i;
        }
    }
    return -1;
}

int detect_hc(char* recv_buffer) {
    char hc_str[5]="";
    strncpy(hc_str, recv_buffer+20, 2);
    return atoi(hc_str);
}

int detect_no(char* recv_buffer) {
    char no_str[5]="";
    strncpy(no_str, recv_buffer+25, 3);
    return atoi(no_str);
}

int detect_np(char* recv_buffer) {
    char np_str[5]="";
    strncpy(np_str, recv_buffer+15, 2);
    return atoi(np_str);
}

int detect_to(char* recv_buffer) {
    char to_str[5]="";
    strncpy(to_str, recv_buffer+10, 2);
    //    printf("to:%s  ",to_str);
    return atoi(to_str);
}

int detect_fm(char* recv_buffer) {
    char fm_str[5]="";
    strncpy(fm_str, recv_buffer+4, 2);
    //    printf("fm:%s  ",fm_str);
    return atoi(fm_str);
}

char* detect_data(char* recv_buffer) {
    char* data_str;
    int d_index = 0;
    while(recv_buffer[d_index] != 'D') {
        d_index++;
    }
    strncpy(data_str, recv_buffer+d_index+5, strlen(recv_buffer)-d_index-5);
    return data_str;
}

char* detect_vl(char* recv_buffer) {
    char* vl_str;
    int v_index = 0;
    int d_index = 0;
    while(recv_buffer[v_index] != 'V') {
        v_index++;
    }
    while(recv_buffer[d_index] != 'D') {
        d_index++;
    }
    strncpy(vl_str, recv_buffer+v_index+2, d_index-v_index);
    return vl_str;
}


int if_in_vl(char* recv_buffer) {
    int vl_arr[20];
    int vl_arr_index = 0;
    int num_in_vl = 0;
    char vl_str[20]="";
    int v_index = 0;
    int d_index = 0;
    while(recv_buffer[v_index] != 'V') {
        v_index++;
    }
    while(recv_buffer[d_index] != 'D') {
        d_index++;
    }
    strncpy(vl_str, recv_buffer+v_index+2, d_index-v_index);
    for(int i = 0;i<strlen(vl_str);i++) {
        if(vl_str[i]!=' ') {
            num_in_vl = num_in_vl*10+atoi(&vl_str[i]);
        }else {
            vl_arr[vl_arr_index] = num_in_vl;
            vl_arr_index++;
            num_in_vl = 0;
        }
    }
    for (int i = 0; i < vl_arr_index; i++) {
        if (FM_number == vl_arr[i]) {
            return 1;
        }
    }
    return 0;
}


int whether_nphc_no(char* recv_buffer, int np, int hc, int no){
    char np_str[5]="";
    char hc_str[5]="";
    char no_str[5]="";
    strncpy(np_str, recv_buffer+15, 2);
    strncpy(hc_str, recv_buffer+20, 2);
    strncpy(no_str, recv_buffer+25, 3);
    //    printf("np_str:%s hc_str:%s no_str:%s\n",np_str,hc_str,no_str);
    if(atoi(np_str)==np && atoi(hc_str)==hc && atoi(no_str)==no) return 1;
    else return 0;
    
}


int whether_swap(char* msg, char* recv_buffer){
    char msg_fm_str[5]="";
    char msg_to_str[5]="";
    char recv_fm_str[5]="";
    char recv_to_str[5]="";
    strncpy(msg_fm_str, msg+3, 2);
    strncpy(recv_fm_str, recv_buffer+3, 2);
    strncpy(msg_to_str, msg+9, 2);
    strncpy(recv_to_str, recv_buffer+9, 2);
    //    printf("msg_fm_str:%s msg_to_str:%s recv_fm_str:%s recv_to_str:%s\n",msg_fm_str,msg_to_str,recv_fm_str,recv_to_str);
    if(atoi(msg_fm_str)==atoi(recv_to_str)||atoi(msg_to_str)==atoi(recv_fm_str))return 1;
    else return 0;
}

int udp_send(int sock, char* buf, struct sockaddr_in serv_addr) {
    int send_result;
    send_result = (int)sendto(sock, buf, strlen(buf),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    if(send_result < 0)
    {
        perror("Send Failed:");
        exit(1);
    }else {
        printf("Send: %s\n", buf);
    }
    return send_result;
}

int udp_recv(int sock, char* recv_buf, int to) {
    memset(recv_buf,0,BUFFER_SIZE);
    fd_set myset;
    struct timeval tv;  // Time out
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    //    int to;
    int selectresult;
    int recv_result = 0;
    
    int req_num = 0;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    
    while(req_num < 1) {
        FD_ZERO(&myset);
        FD_SET(sock, &myset);
        tv.tv_sec = to;
        tv.tv_usec = 0;
        if ((to > 0) && ((selectresult = select(sock+1, &myset, NULL, NULL, &tv)) <= 0)) {
            printf("Time out\n");
            req_num ++;
        }else {
            clilen = sizeof(cliaddr);
            memset(recv_buf,0,BUFFER_SIZE);
            if((recv_result = (int)recvfrom(sock, recv_buf, BUFFER_SIZE - 1, 0, (struct sockaddr*)&cliaddr, &clilen)) == -1) {
                perror("recvfrom");
                exit(1);
            } else {
                printf("Recieved packet: %s\n", recv_buf);
                break;
            }
            
        }
        
    }
    if(req_num == 5) {
        perror("registration fail");
        exit(1);
    }
    return recv_result;
}

int get_src_addr(char* src_str) {
    char dest_str[2] = "";
    memset(dest_str, 0, sizeof(dest_str));
    strncpy(dest_str, src_str + 67 , 2);
    //    strncpy(dest_str, src_str + (strlen(src_str)-2) , 2);
    //    printf("FM_number_str:%s\n",dest_str);
    //    malloc(sizeof(dest_str));
    int fm = atoi(dest_str);
    return fm ;
}



void *server_func(void* new)
{
    char buf1[BUFFER_SIZE] = "";//for receiving
    char buf2[BUFFER_SIZE] = "";//for sending
    int *fd = (int *)new;
    int new_fd = *fd;
    int numbytes = 0;
    
    free(fd);
    printf("\n[thread begin]\n");
    fflush(stdout);
    memset(buf1, 0, sizeof(buf1));
    //    numbytes = udp_recv(new_fd, buf1, USERWAIT);
    
    while (1) {
        sleep(1);
        if (thread_recv_flag == 1) {  //the swither of the thread
            printf("\n[thread]");
            numbytes = udp_recv(new_fd, buf1, USERWAIT);
            if (numbytes > 0) {
                if(detect_np(buf1)==3 && detect_to(buf1)==FM_number) {
                    int fm = detect_fm(buf1);
                    printf("\n[thread]:received a package for me:%s\n",buf1);
                    sprintf(buf2, "FM:%.1d TO:%.2d NP:4 HC:1 NO:%.3d VL: DATA:OK",detect_to(buf1),fm,detect_no(buf1));
                    memset(&server_addr, 0, sizeof(server_addr));
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_port = htons(atoi(port[fm-1]));
                    server_addr.sin_addr.s_addr = inet_addr(ip[fm-1]);
                    printf("\n[thread]");
                    udp_send(new_fd, buf2, server_addr);
                    fflush(stdout);
                    numbytes = 0;
                    memset(buf1, 0, sizeof(buf1));
                    memset(buf2, 0, sizeof(buf2));
                    continue;
                }else if (detect_np(buf1)==3 && detect_to(buf1)!=FM_number) {
                    int fm = detect_fm(buf1);
                    printf("\n[thread]:received a package for me:%s\n",buf1);
                    sprintf(buf2, "FM:%.1d TO:%.2d NP:4 HC:1 NO:%.3d VL: DATA:OK",detect_to(buf1),fm,detect_no(buf1));
                    memset(&server_addr, 0, sizeof(server_addr));
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_port = htons(atoi(port[fm-1]));
                    server_addr.sin_addr.s_addr = inet_addr(ip[fm-1]);
                    printf("\n[thread]");
                    udp_send(new_fd, buf2, server_addr);
                    memset(buf2, 0, sizeof(buf2));
                    int hc = detect_hc(buf1);
                    if (hc == 0) {
                        printf("dropped – hopcount exceeded\n");
                        continue;
                    }else if (hc > 0) {
                        if(if_in_vl(buf1)==1) {
                            printf("dropped – node revisited\n");
                            continue;
                        }
                        else if (if_in_vl(buf1)==0) {
                            char* new_vl;
                            char myser_name[5] = "";
                            sprintf(myser_name," %d",FM_number);
                            new_vl = detect_vl(buf1);
                            strcat(new_vl, myser_name);
                            sprintf(buf2, "FM:%.1d TO:%.2d NP:4 HC:%1d NO:%.3d VL:%s DATA:%s",FM_number,detect_to(buf1),detect_hc(buf1),detect_no(buf1),new_vl,detect_data(buf1));
                            for (int dex = 1; dex < i; dex++) {
                                //                                memset(broadcast_msg, 0, sizeof(broadcast_msg));
                                //                                sprintf(broadcast_msg, "FM:%.1d TO:%.2d NP:9 HC:1 NO:000 VL: DATA:%s",FM_number,addr_index[dex],msg);
                                if (strcmp(ip[dex], ip[FM_number-1])!=0) {
                                    memset(&server_addr, 0, sizeof(server_addr));
                                    server_addr.sin_family = AF_INET;
                                    server_addr.sin_port = htons(atoi(port[dex]));
                                    server_addr.sin_addr.s_addr = inet_addr(ip[dex]);
                                    
                                    udp_send(sockfd, buf2, server_addr);
    
                                   int resend_num = 0;
                                    while ( resend_num < 5) {
                                        numbytes = 0;
                                        if((numbytes = udp_recv(sockfd, buf1, 1)) < 1){
                                            udp_send(sockfd, buf2, server_addr);
                                            resend_num ++;
                                        }else if(whether_swap(buf2, buf1)&&whether_nphc_no(recv_buffer, 4, 1, 0)){
                                            printf("TO and FM comfirmed\n");
                                            printf("NP HC NO comfirmed\n");

                                            memset(respond_msg, 0, sizeof(respond_msg));
                                            sprintf(respond_msg,"FM:%.1d TO:%.2d NP:4 HC:1 NO:000 VL: DATA:OK",FM_number,addr_index[dex]);
                                            udp_send(sockfd, respond_msg, server_addr);
                                            break;
                                        }
                                    }
                                    if (resend_num == 5) {
                                        printf("No callback, give up.\n");
                                        //                exit(1);
                                    }
    
                                }
                                
                            }
                        }
                    }
                    
                }
                numbytes = 0;
            }
        }
        
    }
    
    //
    //    strcpy(buf2,"DUDE");
    //
    //
    //    udp_send(new_fd, buf2, strlen(buf2));
    //
    //    printf("Server waiting (to force client timeout)\n");
    //    fflush(stdout);
    //
    //    sleep(10);
    //
    //    quitthread("Server done.\n", new_fd);
    //    pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
    //    struct map_ip {
    //        int num[40];
    //        char port[40][20];
    //        char ip[40][10];
    //    };
    
    //    char map_ip[10][30];
    
    char* content;
    
    //    socklen_t clilen;
    struct hostent *hp;
    //    char recv_buffer[BUFFER_SIZE] = "";
    char buffer[BUFFER_SIZE] = "";
    char map_data[BUFFER_SIZE] = "";
    //    struct map_ip *map_data_addr;
    
    int numbytes = 0;
    
    
    
    struct addrinfo hints, *servinfo;
    int rv;
    
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    bzero(&hints, sizeof(hints));
    //    freeaddrinfo(&hints);
    
    content = "FM:00 TO:01 NP:1 HC:1 NO:000 VL: DATA:register me";
    bzero(&server_addr, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(SERVER_PORT));
    hp = gethostbyname("dns.postel.org");
    //    server_addr.sin_addr.s_addr = inet_addr(gethostbyname("dns.postel.org")->h_addr);
    bcopy(hp->h_addr, &(server_addr.sin_addr.s_addr), hp->h_length);
    bzero(&server_addr.sin_zero, 8);
    
    
    
    
    
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("socket");
        exit(1);
    } else {
        printf ("create socket.\n");
    }
    
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("listener: bind");
    }
    
    
    
    int resend_num = 0;
    while ( resend_num < 5) {
        int numbytes = 0;
        if((numbytes = udp_recv(sockfd, recv_buffer, 1)) < 1){
            udp_send(sockfd, content, server_addr);
            resend_num ++;
        }else {
            break;
        }
    }
    if (resend_num == 6) {
        perror("No callback");
        exit(1);
    }
    FM_number = get_src_addr(recv_buffer);
    
    //    printf("fm:%d, to:%d, np:%d, no:%d, hc:%d, FM_numbere:%d, data:%s\n",detect_fm(recv_buffer),detect_to(recv_buffer),detect_np(recv_buffer),detect_no(recv_buffer),detect_hc(recv_buffer),get_src_addr(recv_buffer),detect_data(recv_buffer));
    
    char inputstring[STR_SIZE] = "";
    
    memset(buffer,0,BUFFER_SIZE);
    FM_number = get_src_addr(recv_buffer);
    sprintf(buffer, "FM:%.2d TO:01 NP:5 HC:1 NO:000 VL: DATA:GET MAP",FM_number);
    udp_send(sockfd, buffer, server_addr);
    if(udp_recv(sockfd, recv_buffer, 5) < 1) {
        perror("Get map fail");
        exit(1);
    }
    strncpy(map_data, recv_buffer+38, strlen(recv_buffer)-38);
    memset(recv_buffer,0,BUFFER_SIZE);
    //    printf("mapdata: %s\n", map_data);
    //    printf("mapdata: %c\n", map_data[6]);
    char substr[STR_SIZE] = "";
    char portstr[20] = "";
    char ipstr[STR_SIZE] = "";
    char addr_index_str[3] = "";
    //    int i = 0;
    int end = 0;
    int begin = end;
    int at_index = 0;
    printf("|-------|----------MapTable-----|----------|\n");
    while(end < strlen(map_data)){
        while((map_data[end])!=','||end == strlen(map_data)) {
            end ++;
            if (end == strlen(map_data)) {
                break;
            }
        }
        while((map_data[begin])!='=') {
            begin ++;
        }
        strncpy(addr_index_str, map_data+begin-2, 2);
        addr_index[i] = atoi(addr_index_str);
        
        strncpy(substr, map_data+begin+1, end-begin-1);
        at_index = 0;
        while((substr[at_index])!='@') {
            at_index ++;
        }
        strncpy(portstr, substr+(strlen(substr)-5),5);
        //       strcpy((char *)map_data_addr->port[i], portstr);
        for(int index = 0; index < 20; index++) {
            port[i][index] = portstr[index];
        }
        
        strncpy(ipstr, substr,at_index);
        for(int index = 0; index < 20; index++) {
            ip[i][index] = ipstr[index];
        }
        printf("|%2d  \t|ip:%s  \t|port:%s|\n",addr_index[i], ip[i],port[i]);
        
        end++;
        begin++;
        i++;
        memset(substr, 0, strlen(substr));
        memset(ipstr, 0, strlen(ipstr));
        memset(portstr, 0, strlen(ipstr));
    }
    printf("|-------|-----------------------|----------|\n");
    
    pthread_t server_thread;
    int *fdarg = (int *)malloc(sizeof(int));
    *fdarg = sockfd;
    if( pthread_create( &server_thread , NULL , server_func, fdarg) < 0)
    {
        perror("Server: could not create thread");
        return 1;
    }
    
    while (1) {
        char com_dest[STR_SIZE] = "";
        char msg_line[STR_SIZE] = "";
        char msg[BUFFER_SIZE] = "";
        
        printf("please type \" r \" or \" s \" or \" b \": ");
        scanf("%s",inputstring);
        while (strcmp("r",inputstring)!=0 && strcmp("b",inputstring)!=0 && strcmp("s",inputstring)!=0) {
            memset(inputstring,0,STR_SIZE);
            printf("please type \" r \" or \" s \" or \" b \": ");
            scanf("%s",inputstring);
        }
        if (strcmp("r",inputstring)==0) {
            register_flag = 1;
        }
        else if (strcmp("s",inputstring)==0) {
            send_flag = 1;
        }
        else if (strcmp("b",inputstring)==0) {
            broadcast_flag = 1;
        }
        
        
        if (register_flag == 1) {
            memset(recv_buffer, 0, sizeof(recv_buffer));
            thread_recv_flag = 0;
            printf("\n[thread stop]\n");
            //            memset(buffer,0,BUFFER_SIZE);
            
            sprintf(buffer, "FM:%.2d TO:01 NP:5 HC:1 NO:000 VL: DATA:GET MAP",FM_number);
            udp_send(sockfd, buffer, server_addr);
            if(udp_recv(sockfd, recv_buffer, 5) < 1) {
                perror("Get map fail");
                exit(1);
            }
            memset(map_data, 0, STR_SIZE);
            strncpy(map_data, recv_buffer+38, strlen(recv_buffer)-38);
            printf("map_data_size: %d\n", (int)sizeof(map_data));
            memset(recv_buffer,0,BUFFER_SIZE);
            //    printf("mapdata: %s\n", map_data);
            //            printf("mapdata: %c\n", map_data[6]);
            //            char substr[STR_SIZE] = "";
            //            char portstr[20] = "";
            //            char ipstr[STR_SIZE] = "";
            //            char addr_index_str[3] = "";
            memset(substr, 0, STR_SIZE);
            memset(portstr , 0, 20);
            memset(ipstr, 0, STR_SIZE);
            memset(addr_index_str, 0, 3);
            i = 0;
            //    int i = 0;
            end = 0;
            begin = end;
            at_index = 0;
            printf("|-------|----------MapTable-----|----------|\n");
            while(end < strlen(map_data)){
                while((map_data[end])!=','||end == strlen(map_data)) {
                    end ++;
                    if (end == strlen(map_data)) {
                        break;
                    }
                }
                while((map_data[begin])!='='||begin == strlen(map_data)) {
                    begin ++;
                    if (begin == strlen(map_data)) {
                        break;
                    }
                }
                strncpy(addr_index_str, map_data+begin-2, 2);
                addr_index[i] = atoi(addr_index_str);
                
                strncpy(substr, map_data+begin+1, end-begin-1);
                at_index = 0;
                while((substr[at_index])!='@') {
                    at_index ++;
                }
                strncpy(portstr, substr+(strlen(substr)-5),5);
                //       strcpy((char *)map_data_addr->port[i], portstr);
                for(int index = 0; index < 20; index++) {
                    port[i][index] = portstr[index];
                }
                
                strncpy(ipstr, substr,at_index);
                for(int index = 0; index < 20; index++) {
                    ip[i][index] = ipstr[index];
                }
                printf("|%2d  \t|ip:%s  \t|port:%s|\n",addr_index[i], ip[i],port[i]);
                
                end++;
                begin++;
                i++;
                memset(substr, 0, strlen(substr));
                memset(ipstr, 0, strlen(ipstr));
                memset(portstr, 0, strlen(ipstr));
            }
            printf("|-------|-----------------------|----------|\n");
            //            printf("port addr: ");
            //            for (int i; i < sizeof(addr_index); i++) {
            //                printf("%d ",addr_index[i]);
            //            }
            end = 0;
            begin = 0;
            thread_recv_flag = 1;
            printf("\n[thread stop]\n");
            register_flag = 0;
        }
        else if (send_flag == 1) {
            
            
            printf("please choose an address (0-40):");
            
            
            scanf("%s", com_dest);
            if(strlen(com_dest) > 0) {
                
                int dest = get_index(addr_index, atoi(com_dest));
                
                //                printf("\naddr_index: %d\n",dest);
                if(dest < 0 || dest >= i || dest == -1) {
                    perror("invalid destination.");
                    exit(1);
                }
                sprintf(msg, "FM:%.1d TO:%.2d NP:3 HC:1 NO:000 VL: DATA:",FM_number,addr_index[dest]);
                printf("please enter the message followed by \" . \":");
                scanf("%s",msg_line);
                while(strcmp(msg_line+strlen(msg_line)-1, ".") != 0) {
                    strcat(msg, msg_line);
                    printf("keep entering:");
                    scanf("%s",msg_line);
                }
                
                thread_recv_flag = 0; //cease the thread
                printf("\n[thread closed]\n");
                strcat(msg, msg_line);
                
                
                
                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(atoi(port[dest]));
                server_addr.sin_addr.s_addr = inet_addr(ip[dest]);
                
                udp_send(sockfd, msg, server_addr);
                
                //            udp_recv(sockfd, recv_buffer, 10);
                //            if(whether_swap(msg, recv_buffer)){
                //                printf("TO and FM comfirmed");
                //                break;
                //            }
                resend_num = 0;
                while ( resend_num < 5) {
                    int numbytes = 0;
                    if((numbytes = udp_recv(sockfd, recv_buffer, 1)) < 1){
                        udp_send(sockfd, msg, server_addr);
                        resend_num ++;
                    }else if(whether_swap(msg, recv_buffer)&&whether_nphc_no(recv_buffer, 4, 1, 0)){
                        printf("TO and FM comfirmed\n");
                        printf("NP HC NO comfirmed\n");
                        break;
                    }
                }
                if (resend_num == 5) {
                    perror("No callback, give up.");
                    resend_num = 0;
                    //exit(1);
                }
            }
            send_flag = 0;
            continue;
        }
        if (broadcast_flag == 1) {
            thread_recv_flag = 1; //restart the thread
            printf("\n[thread restarted]\n");
            memset(msg, 0, sizeof(msg));
            memset(msg_line, 0, sizeof(msg_line));
            //            printf("please enter \" b \" to send a broadcast:");
            //            scanf("%s",inputstring);
            //            while (strcmp("b",inputstring)!=0) {
            //                memset(inputstring,0,STR_SIZE);
            //                printf("please enter \" b \" to send a broadcast:");
            //                scanf("%s",inputstring);
            //            }
            //    printf("%d",registed_num);
            printf("please enter the broadcasting message followed by \" . \":");
            scanf("%s",msg_line);
            while(strcmp(msg_line+strlen(msg_line)-1, ".") != 0) {
                strcat(msg, msg_line);
                printf("keep entering:");
                scanf("%s",msg_line);
            }
            strcat(msg, msg_line);
            
            thread_recv_flag = 0; //cease the thread
            printf("\n[thread closed]\n");
            
            
            char broadcast_msg[STR_SIZE] = "";
            
            for (int dex = 1; dex < i; dex++) {
                memset(broadcast_msg, 0, sizeof(broadcast_msg));
                sprintf(broadcast_msg, "FM:%.1d TO:%.2d NP:9 HC:1 NO:000 VL: DATA:%s",FM_number,addr_index[dex],msg);
                if (strcmp(ip[dex], ip[FM_number-1])!=0) {
                    memset(&server_addr, 0, sizeof(server_addr));
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_port = htons(atoi(port[dex]));
                    server_addr.sin_addr.s_addr = inet_addr(ip[dex]);
                    //            server_addr.sin_port = htons(atoi(port[0]));
                    //            server_addr.sin_addr.s_addr = inet_addr(ip[0]);
                    
                    
                    udp_send(sockfd, broadcast_msg, server_addr);
                    
                    resend_num = 0;
                    while ( resend_num < 5) {
                        numbytes = 0;
                        if((numbytes = udp_recv(sockfd, recv_buffer, 1)) < 1){
                            udp_send(sockfd, broadcast_msg, server_addr);
                            resend_num ++;
                        }else if(whether_swap(msg, recv_buffer)&&whether_nphc_no(recv_buffer, 9, 1, 0)){
                            printf("TO and FM comfirmed\n");
                            printf("NP HC NO comfirmed\n");
                            
                            memset(respond_msg, 0, sizeof(respond_msg));
                            sprintf(respond_msg,"FM:%.1d TO:%.2d NP:9 HC:1 NO:000 VL: DATA:OK",FM_number,addr_index[dex]);
                            udp_send(sockfd, respond_msg, server_addr);
                            break;
                        }
                    }
                    if (resend_num == 5) {
                        printf("No callback, give up.\n");
                        //                exit(1);
                    }
                    
                }
                
            }
            broadcast_flag = 0;
            continue;
        }
    }
    
    
    memset(recv_buffer, 0, sizeof(recv_buffer));
    //    if((numbytes = udp_recv(sockfd, recv_buffer, 10)) < 1) {
    //        int NP_num = detect_np(recv_buffer);
    //        if(NP_num==3) {
    //            
    //        }
    //    }
    thread_recv_flag = 1;
    close(sockfd);
    freeaddrinfo(servinfo);
    //    freeaddrinfo(hints);
    //    freeaddrinfo(&hints);
    
    return 0;
}

