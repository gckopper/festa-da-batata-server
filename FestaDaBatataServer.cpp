// Festa-da-batata-server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define SERVER
#include "MessageHandler.h"

#include "Logger.h"

#define DEFAULT_PORT "27015"
#define MAX_CLIENTS 128

// Main que na realidade é responsável por abrir o socket
int main()
{
    std::cout << "Hello World!\n";
	MessageHandler game;
    WSADATA wsaData;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
		printf("Erro ao inicializar o Winsock: %d\n", iResult);
		return 1;
	}
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP; // TCP
    hints.ai_flags = AI_PASSIVE; // Servidor
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("Erro ao obter informações do endereço: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
		printf("Erro ao criar o socket: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("Erro ao associar o socket a porta: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);
	if (listen(ListenSocket, MAX_CLIENTS) == SOCKET_ERROR) {
		printf("Erro ao iniciar a escuta: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	SOCKET ClientSocket;
	ClientSocket = INVALID_SOCKET;
	WSAPOLLFD fdArray[MAX_CLIENTS];
	ZeroMemory(fdArray, MAX_CLIENTS * sizeof(WSAPOLLFD));
	fdArray[0].fd = ListenSocket;
	fdArray[0].events = POLLRDNORM;
	unsigned int fdCount = 1;
	// Escolhe uma seed aleatória quando fora do modo de debug
	// Alguns testes dependem na seed ser 8 e são rodados em 
	// modo de debug
#ifdef _DEBUG
	uint64_t seed = 8;
#else
	uint64_t seed = time(NULL);
#endif
	std::srand(seed);
	while (true) {
		int nSocks = WSAPoll(fdArray, MAX_CLIENTS, -1);
		if (nSocks == SOCKET_ERROR) {
			printf("Erro ao chamar WSAPoll: %ld\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		for (int i = fdCount-1; i > 0; --i) {
			if (fdArray[i].revents == 0) {
				continue;
			}
			if (fdArray[i].revents & POLLHUP || fdArray[i].revents & POLLERR || fdArray[i].revents & POLLNVAL) {
				game.deleteRoomAndEndGame(fdArray[i].fd);
				closesocket(fdArray[i].fd);
				fdArray[i].fd = fdArray[fdCount-1].fd;
				fdArray[fdCount - 1].fd = INVALID_SOCKET;
				fdCount--;
				// GARBAGE COLLECTION
				LOG("Cliente disconectado total atual: " << fdCount);
				continue;
			}
			if (fdArray[i].revents & POLLRDNORM) {
				LOG("MESSAGEM CHEGANDO");
				game.handleClient(fdArray[i].fd);
				continue;
			}
			LOG("TEMOS UM SOCKET ESQUISITO: " << fdArray[i].revents);
			continue;
		}
		if (fdArray[0].revents & POLLRDNORM && fdCount <= MAX_CLIENTS) {
			ClientSocket = accept(ListenSocket, NULL, NULL);
			if (ClientSocket == INVALID_SOCKET) {
				printf("Erro ao aceitar a conexão: %ld\n", WSAGetLastError());
				closesocket(ListenSocket);
				WSACleanup();
				return 1;
			}
			fdArray[fdCount].fd = ClientSocket;
			fdArray[fdCount].events = POLLRDNORM;
			fdCount++;
			// pega a porta do cliente
			sockaddr_in addr{};
			int len = sizeof(addr);
			getpeername(ClientSocket, (sockaddr*)&addr, &len);
			LOG("novo cliente total atual: " << fdCount << " port=" << addr.sin_port);
		}
	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
