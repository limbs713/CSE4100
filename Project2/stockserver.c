/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

typedef struct {		
	int maxfd;
    fd_set read_set;			
    fd_set ready_set;			
    int nready;					
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientRio[FD_SETSIZE];
} pool;

typedef struct{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
}item;

typedef struct Node{
    item data;
    struct Node* left;
    struct Node* right;
}Node;

void echo(int connfd);
void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_client(pool *p, Node* root);
void insert(Node* curr, item data);
void show(int connfd, Node* curr);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pool pool;`

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd,&pool);
    /**
     * stock.txt를 item에 저장*/
    FILE *fp = fopen("./stock.txt", "r");
    int var1,var2,var3;
    Node* root = (Node*)malloc(sizeof(Node));
    while(fscanf(fp,"%d %d %d",&var1,&var2,&var3) == 3)
    {
        item curr;
        curr.ID=var1;
        curr.left_stock=var2;
        curr.price=var3;
        if(root == NULL)
        {
            root->data = curr;
            root->left = NULL;
            root->right = NULL;
        }
        else
        {
            insert(root, curr);
        }

    } 
     
    fclose(fp);
    //
    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd + 1 , &pool.ready_set , NULL ,NULL ,NULL);
        
        if (FD_ISSET(listenfd, &pool.ready_set)) // connect가 들어왔다면
        {	
        	clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            add_client(connfd, &pool);	
        }

        check_client(&pool,root);
    }

}

/* $end echoserverimain */
void init_pool(int listenfd, pool *p) {
    p->maxi = -1;
    for (int i = 0; i < FD_SETSIZE; i++)
    	p->clientfd[i] = -1;	
    
    p->maxfd = listenfd;		
    FD_ZERO(&p->read_set);		
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p) {
	int i;
	p->nready--;		
    
    for (i = 0; i < FD_SETSIZE; i++) {		
    	if (p->clientfd[i] < 0) {			
        	p->clientfd[i] = connfd;		
            Rio_readinitb(&p->clientRio[i], connfd);
            
            FD_SET(connfd, &p->read_set);	
            
            if (connfd > p->maxfd)			
            	p->maxfd = connfd;		
            if (i > p->maxi)				
            	p->maxi = i;				
            break;
        }
    }
    
    if (i == FD_SETSIZE)
    	app_error("Error in add_client!\n");
}

void check_client(pool *p, Node* root) {
	int n, connfd;
    char buf[MAXLINE];
    rio_t Rio;
    
    // nready가 남아있는 경우, connfd Array를 쫘악 훑는다. 
    for (int i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
    	connfd = p->clientfd[i];
        Rio = p->clientRio[i];
        
        // 현재 조회중인 connfd에 Pending Input이 있다면,
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
        	if ((n = Rio_readlineb(&Rio, buf, MAXLINE)) != 0) {
                if(!strcmp(buf,"show"))
                {
                    show(connfd,root);
                }
                else if(!strcmp(buf,"exit"))
                {
                    exit(0);
                }
                else if(!strncmp(buf,"buy",3)){
                    char* cmd = strtok(buf," ");
                    char* id = strtok(buf," ");
                    char* cnt = strtok(buf," ");

                    Node* curr = root;
                    while(curr->data.ID != atoi(id))
                    {
                        if(curr->data.ID < atoi(id))
                        {
                            curr = curr->right;
                        }
                        else
                        {
                            curr = curr->left;
                        }
                    }

                    if(curr->data.left_stock < atoi(cnt))
                    {
                        char msg[MAXLINE];
                        strcpy(msg,"Not enough left stock\n");
                        Rio_writen(connfd, msg , MAXLINE);
                    }   
                    else
                    {
                        curr ->data.left_stock -= atoi(cnt);
                    }
                }
                else if(!strncmp(buf,"sell",4))
                {
                    char* cmd = strtok(buf," ");
                    char* id = strtok(buf," ");
                    char* cnt = strtok(buf," ");

                    Node* curr = root;
                    while(curr->data.ID != atoi(id))
                    {
                        if(curr->data.ID < atoi(id))
                        {
                            curr = curr->right;
                        }
                        else
                        {
                            curr = curr->left;
                        }
                    }
                        curr ->data.left_stock += atoi(cnt);
                }
        	}
        	else {								// connfd에서 EOF를 만난 경우,
        		Close(connfd);					// 디스크립터를 닫고
        	    FD_CLR(connfd, &p->read_set);	// fdset에서 Inactive로 만들고
        	    p->clientfd[i] = -1;			// connfd Array에서도 제거!
        	}
        }
    }
}

void insert(Node* curr, item data)
{
    if(curr->data.ID > data.ID)
    {
        if (curr->left == NULL) {
                Node * temp = (Node*)malloc(sizeof (Node));
                temp->data= data;
                curr->left = temp;
        } else {
            insert(curr->left, data);
        }
    }
    else if(curr->data.ID < data.ID)
    {
        if (curr->right == NULL) {
                    Node * temp = (Node*)malloc(sizeof (Node));
                    temp->data= data;
                    curr->right = temp;
                } else {
                    insert(curr->right,data);
                }
    }
}

void show(int connfd, Node* curr)
{
    if(curr == NULL)
        return;

    char buf[MAXLINE];
    show(connfd,curr->left);
    sprintf(buf,"%d %d %d\n",curr->data.ID,curr->data.left_stock,curr->data.price);
    Rio_writen(connfd,buf,MAXLINE);
    show(connfd,curr->right);

    return;
}