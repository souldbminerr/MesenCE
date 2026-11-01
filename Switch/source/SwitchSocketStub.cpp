/* Stub out BSD sockets as I havent implemented them yet */
#include "Utilities/Socket.h"

Socket::Socket()
{
	SetConnectionErrorFlag();
}

Socket::Socket(uintptr_t socket)
{
	(void)socket;
	SetConnectionErrorFlag();
}

Socket::~Socket()
{
}

void Socket::SetSocketOptions()
{
}

void Socket::SetConnectionErrorFlag()
{
	_connectionError = true;
}

void Socket::Close()
{
}

bool Socket::ConnectionError()
{
	return true;
}

void Socket::Bind(uint16_t port)
{
	(void)port;
	SetConnectionErrorFlag();
}

bool Socket::Connect(const char* hostname, uint16_t port)
{
	(void)hostname;
	(void)port;
	SetConnectionErrorFlag();
	return false;
}

void Socket::Listen(int backlog)
{
	(void)backlog;
	SetConnectionErrorFlag();
}

unique_ptr<Socket> Socket::Accept()
{
	return nullptr;
}

int Socket::Send(char* buf, int len, int flags)
{
	(void)buf;
	(void)len;
	(void)flags;
	SetConnectionErrorFlag();
	return -1;
}

void Socket::BufferedSend(char* buf, int len)
{
	(void)buf;
	(void)len;
}

void Socket::SendBuffer()
{
}

int Socket::Recv(char* buf, int len, int flags)
{
	(void)buf;
	(void)len;
	(void)flags;
	SetConnectionErrorFlag();
	return -1;
}
