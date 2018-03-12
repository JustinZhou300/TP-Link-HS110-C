//Client for TP-Link HS110(AU) HW 2.0
//Version 1.2
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define BUFFER_LEN 1024
#define HS110PORT 9999

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <getopt.h>
#include <stdbool.h>
#include <time.h>
#if defined(_WIN32)
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(__linux__)
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#endif

char* encrypt(char cmd[],bool udp) {
	int padding = 4 * (!udp);
	char *output = (char *)malloc(strlen(cmd) + padding);
	if (padding > 0)
	{
		for (int i = 0; i < 3; i++)
			output[i] = '\0';
		output[3] = (char)strlen(cmd);
	}
	char key = 171;
	for (int i = 0; i < strlen(cmd); i++)
	{
		char temp = key ^ (char)cmd[i];
		key = temp;
		output[i + 4 * padding] = temp;
	}
	return output;
}

char* decrypt(char msg[],bool udp) {
	int padding = 4*(!udp);
	char *output = (char *)malloc(BUFFER_LEN);
	char key = 171;
	for (int i = padding; i <BUFFER_LEN; i++) {
		char temp = key ^ (char) msg[i];
		key = msg[i];
		output[i-padding] = temp;
	}
    output[strlen(output)-1] = '\0'; //Remove garbage char at end of string;
	return output;
}

void Closesocket(int clientSocket){
#ifdef _WIN32
		closesocket(clientSocket);
		WSACleanup();
#elif defined(__linux__)
		close(clientSocket);
#endif
}

typedef struct {char* cmd; char* msg;} commands; 

static int udpflag = 0;


int main(int argc, char* argv[]) {

/*---- Handling Console Arguments  ----*/
commands clist[] = {
	//System commands
	{"info", "{\"system\":{\"get_sysinfo\":{}}}"},
	{"on", "{\"system\":{\"set_relay_state\":{\"state\":1}}}"},
	{"off", "{\"system\":{\"set_relay_state\":{\"state\":0}}}"},
	{"reboot", "{\"system\":{\"reboot\":{\"delay\":1}}}"},
	{"reset", "{\"system\":{\"reset\":{\"delay\":1}}}"},
	{"LEDoff", "{\"system\":{\"set_led_off\":{\"off\":1}}}"},
	{"LEDon", "{\"system\":{\"set_led_off\":{\"off\":0}}}"},
	{"time", "{\"time\":{\"get_time\":null}}"},
	//Emeter
	{"emter", "{\"emeter\":{\"get_realtime\":{}}}"},
	{"gain", "{\"emeter\":{\"get_vgain_igain\":{}}}"}};

char hmessage[] = "Run the program with the arguments: (Program Path) -i (IPV4 address) -c (command) \n\
	Example: ./HS110Client.exe -i 192.168.0.5 -c on \n\
		List of commands:\n\
		on: Turn on device \n\
		off: Turn off device \n\
		reboot: Reboot device \n\
		info: Retrieve system info including MAC, device ID, hardware ID etc \n\
		emeter: Retrieve real time current and voltage readings \n\
		gain: Retrieve EMeter VGain and IGain settings\n\
		LEDoff: Turn off LED light \n\
		LEDon: Turn on LED light";

int opt;
char *cmd = NULL, *ip = NULL;
int long_index;

static struct option long_options[] = {
	{"help", no_argument, 0, 'h'},
	{"ip", required_argument, 0, 'i'},
	{"command", required_argument, 0, 'c'},
	{"udp", no_argument, &udpflag, 1},
	{0, 0, 0, 0}};

while ((opt = getopt_long(argc, argv, "hi:c:", long_options, &long_index)) != -1)
{
	switch (opt)
	{
	case '0':
		break;
	case 'h':
		printf(hmessage);
		return 0;
		break;
	case 'i':
		ip = optarg;
		break;
	case 'c':
		for (commands *c = clist; c != clist + sizeof(clist) / sizeof(clist[0]); c++)
		{
			if (strcmp(c->cmd, optarg) == 0)
			{
				cmd = c->msg;
				break;
			}
		}
		break;
	case '?':
		printf("%s", "Error: unkown command \n");
		printf(hmessage);
		return 1;
		break;
	default:
		break;
	}
	}

  if (optind < argc)
    {
      printf ("%s","non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }

	if (!ip)  {
		printf("%s","Error: No IP Entered!");
		return 1;
	}
	if(!cmd){
		cmd= clist[0].msg; //Default with get info command
	}

#ifdef _WIN32
	WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(versionWanted, &wsaData);
#endif

	int clientSocket;
	char buffer[BUFFER_LEN] ;
	memset(buffer, '\0', sizeof(buffer));
	struct sockaddr_in HS110Addr;
	socklen_t addr_size;

	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP) */
	clientSocket = socket(PF_INET, udpflag ? SOCK_DGRAM : SOCK_STREAM , 0);
	/*---- Configure settings of the server address struct ----*/
	HS110Addr.sin_family = AF_INET;
	HS110Addr.sin_port = htons(HS110PORT);
	/* Set IP address */
	HS110Addr.sin_addr.s_addr = inet_addr(ip);
	memset(HS110Addr.sin_zero, '\0', sizeof HS110Addr.sin_zero);
	/*---- Connect the socket to the server using the address struct ----*/
	addr_size = sizeof HS110Addr;
	char *msg = encrypt(cmd,udpflag);
	int iResult = 0;

	/*---- Encrypt command and send to HS110 ----*/
	if (udpflag)
	{
		struct sockaddr_in cliAddr;
		int broadcast = 1;

		if (setsockopt(clientSocket, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof broadcast) == -1)
		{
			perror("setsockopt (SO_BROADCAST)");
			return 1;
		}

		/* bind any port */
		cliAddr.sin_family = AF_INET;
		cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		cliAddr.sin_port = htons(0);
		iResult = bind(clientSocket, (struct sockaddr *)&cliAddr, sizeof(cliAddr));
		if (iResult < 0)
		{
			printf("%s: cannot bind port\n", ip);
			return 1;
		}
		iResult = sendto(clientSocket, msg, strlen(cmd), 0,
						 (struct sockaddr *)&HS110Addr, sizeof(HS110Addr));

		int cliLen = sizeof(cliAddr);

#ifdef _WIN32
		DWORD tv = 3000;
		if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)
#elif defined(__linux__)
		struct timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
#endif
		{
			perror("setsockopt (SO_RCVTIMEO)");
			return 1;
		}

		do
		{
			memset(buffer, '\0', sizeof(buffer));
			cliLen = sizeof(cliAddr);
			iResult = recvfrom(clientSocket, buffer, BUFFER_LEN, 0, (struct sockaddr *)&cliAddr, &cliLen);
			if (iResult < 0)
			{
				printf("%s: cannot receive data \n", argv[0]);
				continue;
			}

			char *d_mesg = decrypt(buffer, udpflag);
			printf("Response received: %s\n", d_mesg);

		} while (buffer[4] != '\0');
		Closesocket(clientSocket);
		return 0;
	}

	iResult = connect(clientSocket, (struct sockaddr *) &HS110Addr, addr_size);

	if (iResult == -1) {
		printf("Error: Failed to make connection on socket \n ");
		Closesocket(clientSocket);
		return 1;
	}

	iResult = send(clientSocket, msg, strlen(cmd) +4, 0);

	if (iResult == -1) {
		printf("Error: Failed to send command to device. \n ");		
		Closesocket(clientSocket);
		return 1;
	}

	/* ---- Retrieve reply from HS110 ---- */
	iResult = recv(clientSocket, buffer, BUFFER_LEN, 0);
	if (iResult < 0) {
	printf("Error: Failed recieve response. \n");
	}
	else {
	char *d_mesg = decrypt(buffer,udpflag);
	printf("Response received: %s\n", d_mesg);
	}
	/* ---- Clean up ---- */
	Closesocket(clientSocket);
	return 0;
}


