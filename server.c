#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include "lotto.h"

void exit_signal(int signum);

int* lotto_fork();
int cal_lotto();
void lotto();
int check();
int* end(int *child);

void logging(char *ip, char *result);

void sort(int arr[], int len);

// global socket value
int listenfd = 0;
int connfd = 0;
int myNum = -1;

int pfd[LOTTO_NUM][2];
int cfd[LOTTO_NUM][2];

int main(int argc, char* argv[]) {

	int i, *child;
	int *cresult;
	struct sockaddr_in serv_addr, client_addr;
  socklen_t clientaddr_size = sizeof(client_addr);
  char result[64], rbuf[8], ip[16];

  // rand seed init
  srand(time(NULL));

	// set kill signal
	signal(SIGINT, exit_signal);

	// init socekt
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	memset(&client_addr, 0, sizeof(client_addr));
	memset(result, 0, sizeof(result));

	// set socket
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000);

	// connect setting
  if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
    perror("binding Error");
    return -1;
  }

	listen(listenfd, 10);
  debug("listen\n");

	// accept loop
  while ((connfd = accept(listenfd, (struct sockaddr *)&client_addr, &clientaddr_size))
				&& fork()
        && !close(connfd)) rand(); // rand count;

  // get ip address
  snprintf(ip, sizeof(ip), "%d.%d.%d.%d",
    (int)(client_addr.sin_addr.s_addr&0xFF),
    (int)((client_addr.sin_addr.s_addr&0xFF00)>>8),
    (int)((client_addr.sin_addr.s_addr&0xFF0000)>>16),
    (int)((client_addr.sin_addr.s_addr&0xFF000000)>>24));

  debug("accept: %s\n", ip);

	// lotto start
	child = lotto_fork();

	// lotto do
	do {
		lotto();
	} while (check());

	// lotto end
	cresult = end(child);
	sort(cresult, LOTTO_NUM);

	// create result
  for (i = 0; i < LOTTO_NUM; i++) {
		snprintf(rbuf, sizeof(rbuf), "%d ", cresult[i]);
    strcat(result, rbuf);
	}
	strcat(result, "\n");

	// send lotto
	write(connfd, result, strlen(result));

  // create log
  logging(ip, result);

	// main end
	close(connfd);
	close(listenfd);

	return 0;
}

void exit_signal(int signum) {
	close(listenfd);
	exit(1);
}

int* lotto_fork() {
	int i, *arr;
  arr = (int*)malloc(sizeof(int) * LOTTO_NUM);

  debug("> lotto_fork\n");

  for (i = 0; i < LOTTO_NUM; i++) {
    // create pipe
    pipe(pfd[i]);
    pipe(cfd[i]);

    // fork init
    srand(time(NULL) - (myNum * myNum) + rand());
    arr[i] = fork();

    // child start
    if (!arr[i]) {
      myNum = i;
      close(pfd[i][1]);
      close(cfd[i][0]);

      cal_lotto();
    }
  }

  debug("< lotto_fork\n");

  return arr;
}

int* end(int *child) {
  int *cresult, i;
  char buf[8];
  cresult = (int*)malloc(sizeof(int) * LOTTO_NUM);

  debug("> end\n");

  for (i = 0; i < LOTTO_NUM; i++) {
    close(pfd[i][1]);
  }

  for (i = 0; i < LOTTO_NUM; i++) {
    waitpid(child[i], &cresult[i], 0);
    cresult[i] >>= 8;
  }

  debug("end result: ");
  for (i = 0; i < LOTTO_NUM; i++) {
    debug("%d ", cresult[i]);
  }
  debug("\n");

  debug("< end\n");

  return cresult;
}

void sort(int arr[], int len) {
	int i, j;

	for (i = 0; i < len - 1; i++) {
		for (j = 0; j < len - i - 1; j++) {
			if (arr[j] > arr[j + 1]) {
				swap(arr[j], arr[j + 1]);
			}
		}
	}
}

void lotto() {
  int i;

  debug("> lotto\n");

  for (i = 0; i < LOTTO_NUM; i++) {
    write(pfd[i][1], "OK", 2);
  } 

  debug("< lotto\n");
}

int check() {
  int cresult[LOTTO_NUM], i;
  char buf[8];
  debug("> check\n");

  for (i = 0; i < LOTTO_NUM; i++) {
    read(cfd[i][0], buf, sizeof(buf));
    cresult[i] = atoi(buf);
  }

  sort(cresult, LOTTO_NUM);

  debug("check value: ");
  for (i = 0; i < LOTTO_NUM; i++)
    debug("%d ", cresult[i]);

  debug("\n");

  for (i = 1; i < LOTTO_NUM; i++)
    if (cresult[i - 1] == cresult[i]) return 1;

  debug("< check\n");

  return 0;
}

/* fork */
int cal_lotto() {
  char buf[8];
  int lotto = -1;

  debug("child[%d]: > cal_lotto\n", myNum);

  while (read(pfd[myNum][0], buf, sizeof(buf))) {
    lotto = rand() % 44 + 1;
    snprintf(buf, sizeof(buf), "%d", lotto);

    debug("lotto : %s\n", buf);

    write(cfd[myNum][1], buf, sizeof(buf));
  }

  debug("child[%d]: < cal_lotto : %d\n", myNum, lotto);

  // result return
	exit(lotto);
}

/* log */
void logging(char *ip, char *result) {
  int fd;
  char buf[64];
  struct flock lock;

  debug("> log\n");

  fd = open("log.txt", O_CREAT | O_WRONLY | O_APPEND, 0600);

  lockf(fd, F_LOCK, sizeof(buf));
  debug(">> file lock\n");

  snprintf(buf, sizeof(buf), "[%s]: %s", ip, result);
  write(fd, buf, strlen(buf));
  debug("LOG: %s", buf);
  //sleep(10);

  lockf(fd, F_ULOCK, sizeof(buf));
  debug("<< file unlock\n");
  close(fd);

  debug("< log\n");
}