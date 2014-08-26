#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment( lib, "WS2_32" ) // Link in the WS2_32.lib static library

//-----------------------------------------------------------------------------------------------
const int BUFFER_LIMIT = 512;

//-----------------------------------------------------------------------------------------------
enum ClientServerType
{
	CLIENT_SERVER_TCP,
	CLIENT_SERVER_UDP
};

//-----------------------------------------------------------------------------------------------
void ToUpperCaseString( std::string& outString );
ClientServerType PromptUserForTCPOrUDP();
void PromptUserForIPAndPortInfo( std::string& ipAddress_out, std::string& port_out );
bool InitializeWinSocket();
bool CreateAddressInfoForTCP( const std::string& ipAddress, const std::string& tcpPort, addrinfo*& tcpAddressInfo );
bool CreateAddressInfoForUDP( const std::string& ipAddress, const std::string& udpPort, addrinfo*& udpAddressInfo );
bool CreateSocket( addrinfo*& addressInfo, SOCKET& newSocket );
bool BindSocket( SOCKET& socketToBind, addrinfo*& addressInfo );
bool SetSocketListen( SOCKET& socketToSet );
bool AcceptClientSocket( SOCKET& serverSocket, SOCKET& clientSocket );
bool ConnectSocket( SOCKET& socketToConnect, addrinfo*& addressinfo );
bool TCPSendMessage( const SOCKET& socketToSendTo );
bool TCPSendMessage( const SOCKET& socketToSendTo, const char* message, int messageLength );
bool TCPReceiveMessageServer( const SOCKET& serverSocket, const SOCKET& acceptedClientSocket );
bool TCPReceiveMessageClient( const SOCKET& clientSocket );
bool UDPSendMessageClient( const SOCKET& socketToSendTo );
bool UDPSendMessageServer( const SOCKET& socketToSendTo );
bool UDPSendMessageServer( const SOCKET& socketToSendTo, const char* message, int messageLength );
bool UDPReceiveMessageServer( const SOCKET& serverSocket );
bool UDPReceiveMessageClient( const SOCKET& clientSocket );

bool SetupTCPClientServer( const std::string& ipaddress, const std::string& tcpPort, SOCKET& tcpServerSocket_out, SOCKET& tcpAcceptedClientSocket_out, SOCKET& tcpClientSocket_out );
bool SetupUDPClientServer( const std::string& ipaddress, const std::string& udpPort, SOCKET& udpServerSocket_out, SOCKET& udpClientSocket_out );
bool RunTCPClientServer( SOCKET& tcpServerSocket, SOCKET& tcpAcceptedClientSocket, SOCKET& tcpClientSocket );
bool RunUDPClientServer( SOCKET& udpServerSocket, SOCKET& udpClientSocket );
bool CreateAndSetupTCPServerToListen( addrinfo*& addressInfo, SOCKET& newSocket );

std::string IPAddressAsString;
std::string portAsString;
std::string udpClientPortAsString;

addrinfo* UDPServerAddrInfo = nullptr;
addrinfo* UDPClientAddrInfo = nullptr;

//-----------------------------------------------------------------------------------------------
int __cdecl main(int argc, char **argv)
{
	ClientServerType typeOfClientAndServerToRun;

	typeOfClientAndServerToRun = PromptUserForTCPOrUDP();

	PromptUserForIPAndPortInfo( IPAddressAsString, portAsString );
	
	bool serverClientRunResult = true;

	if( typeOfClientAndServerToRun == CLIENT_SERVER_TCP )
	{
		SOCKET TCPServerSocket = INVALID_SOCKET;
		SOCKET TCPAcceptSocket = INVALID_SOCKET;
		SOCKET TCPClientSocket = INVALID_SOCKET;

		serverClientRunResult = SetupTCPClientServer( IPAddressAsString, portAsString, TCPServerSocket, TCPAcceptSocket, TCPClientSocket );
		if( serverClientRunResult )
		{
			serverClientRunResult = RunTCPClientServer( TCPServerSocket, TCPAcceptSocket, TCPClientSocket );
		}

		closesocket( TCPServerSocket );
		closesocket( TCPAcceptSocket );
		closesocket( TCPClientSocket );
	}
	else
	{
		std::printf( "Enter Port for client: " );
		std::cin >> udpClientPortAsString;

		SOCKET UDPServerSocket = INVALID_SOCKET;
		SOCKET UDPClientSocket = INVALID_SOCKET;

		serverClientRunResult = SetupUDPClientServer( IPAddressAsString, portAsString, UDPServerSocket, UDPClientSocket );
		if( serverClientRunResult )
		{
			serverClientRunResult = RunUDPClientServer( UDPServerSocket, UDPClientSocket );
		}

		closesocket( UDPServerSocket );
		closesocket( UDPClientSocket );
	}

	WSACleanup();

	if( !serverClientRunResult )
	{
		return 1;
	}
	return 0;
}


//-----------------------------------------------------------------------------------------------
void ToUpperCaseString( std::string& outString )
{
	for(int i = 0; i < (int)outString.length(); ++ i)
	{
		outString[i] = static_cast<char>(toupper(static_cast<int>(outString[i])));
	}
}


//-----------------------------------------------------------------------------------------------
ClientServerType PromptUserForTCPOrUDP()
{
	std::string clientServerTypeAsString = "";

	while( clientServerTypeAsString != "TCP" && clientServerTypeAsString != "UDP" )
	{
		clientServerTypeAsString = "";
		system( "cls" );

		std::printf( "Choose server/client type:\nType TCP for TCP or UDP for UDP: " );
		std::cin >> clientServerTypeAsString;

		ToUpperCaseString( clientServerTypeAsString );
	}

	if( clientServerTypeAsString == "TCP\0" )
	{
		return CLIENT_SERVER_TCP;
	}
	else
	{
		return CLIENT_SERVER_UDP;
	}
}


//-----------------------------------------------------------------------------------------------
void PromptUserForIPAndPortInfo( std::string& ipAddress_out, std::string& port_out )
{
	std::printf( "\nEnter IP Address for the server: " );
	std::cin >> ipAddress_out;

	std::printf( "IP Address of the server is %s\n\n", ipAddress_out.c_str() );

	std::printf( "Enter Port for server: " );
	std::cin >> port_out;

	std::printf( "Port of the server is %s\n\n", port_out.c_str() );
}


//-----------------------------------------------------------------------------------------------
bool InitializeWinSocket()
{
	WSAData winSocketAData;

	int initializationResult;
	initializationResult = WSAStartup( MAKEWORD( 2,2 ), &winSocketAData );

	if( initializationResult != 0 )
	{
		printf( "WSAStartup failed %d\n", initializationResult );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool CreateAddressInfoForTCP( const std::string& ipAddress, const std::string& tcpPort, addrinfo*& tcpAddressInfo )
{
	int getAddressInfoResult;

	addrinfo hints;

	ZeroMemory( &hints, sizeof( hints ) );

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	getAddressInfoResult = getaddrinfo( ipAddress.c_str(), tcpPort.c_str(), &hints, &tcpAddressInfo );

	if( getAddressInfoResult != 0 )
	{
		printf( "getaddrinfo failed %d\n", getAddressInfoResult );
		freeaddrinfo( tcpAddressInfo );
		WSACleanup();

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool CreateAddressInfoForUDP( const std::string& ipAddress, const std::string& udpPort, addrinfo*& udpAddressInfo )
{
	int getAddressInfoResult;

	addrinfo hints;

	ZeroMemory( &hints, sizeof( hints ) );
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	getAddressInfoResult = getaddrinfo( ipAddress.c_str(), udpPort.c_str(), &hints, &udpAddressInfo );

	if( getAddressInfoResult != 0 )
	{
		printf( "getaddrinfo failed %d\n", getAddressInfoResult );
		freeaddrinfo( udpAddressInfo );
		WSACleanup();

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool CreateSocket( addrinfo*& addressInfo, SOCKET& newSocket )
{
	newSocket = socket( addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol );

	if( newSocket == INVALID_SOCKET )
	{
		printf( "socket failed with error: %ld\n", WSAGetLastError() );
		WSACleanup();

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool BindSocket( SOCKET& socketToBind, addrinfo*& addressInfo )
{
	int bindResultAsInt;

	bindResultAsInt = bind( socketToBind, addressInfo->ai_addr, static_cast< int >( addressInfo->ai_addrlen ) );
	
	if( bindResultAsInt == SOCKET_ERROR )
	{
		printf( "bind failed with error: %d\n", WSAGetLastError() );

		closesocket( socketToBind );
		WSACleanup();

		return false;
	}	

	return true;
}


//-----------------------------------------------------------------------------------------------
bool SetSocketListen( SOCKET& socketToSet )
{
	int listenResultAsInt;

	listenResultAsInt = listen( socketToSet, SOMAXCONN );

	if ( listenResultAsInt == SOCKET_ERROR ) 
	{
		printf( "listen failed with error: %d\n", WSAGetLastError() );
		closesocket( socketToSet );
		WSACleanup();

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool AcceptClientSocket( SOCKET& serverSocket, SOCKET& clientSocket )
{
	clientSocket = accept( serverSocket, NULL, NULL );

	if ( clientSocket == INVALID_SOCKET ) 
	{
		printf( "accept failed with error: %d\n", WSAGetLastError() );
		closesocket( serverSocket );
		WSACleanup();

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool ConnectSocket( SOCKET& socketToConnect, addrinfo*& addressinfo )
{
	int connectResult;

	connectResult = connect( socketToConnect, addressinfo->ai_addr, ( int )addressinfo->ai_addrlen );
	if( socketToConnect == SOCKET_ERROR )
	{
		closesocket( socketToConnect );
		socketToConnect = INVALID_SOCKET; 

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool TCPSendMessage( const SOCKET& socketToSendTo )
{
	std::string sendAsString;

	system( "cls" );
	std::printf( "Type Message To Send: ");

	std::cin >> sendAsString;

	int sendResultAsInt;

	sendResultAsInt = send( socketToSendTo, sendAsString.c_str(), sendAsString.size(), 0 );

	if ( sendResultAsInt == SOCKET_ERROR ) 
	{
		printf( "send failed with error: %d\n", WSAGetLastError() );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool TCPSendMessage( const SOCKET& socketToSendTo, const char* message, int messageLength )
{
	int sendResultAsInt;

	sendResultAsInt = send( socketToSendTo, message, messageLength, 0 );

	if ( sendResultAsInt == SOCKET_ERROR ) 
	{
		printf( "send failed with error: %d\n", WSAGetLastError() );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool TCPReceiveMessageServer( const SOCKET& serverSocket, const SOCKET& acceptedClientSocket )
{
	char receiveBuffer[ BUFFER_LIMIT ];
	memset( receiveBuffer, '\0', BUFFER_LIMIT );

	int bytesReceived = 0;

	bytesReceived = recv( acceptedClientSocket, receiveBuffer, BUFFER_LIMIT, 0 );

	if( bytesReceived == 0 )
	{
		return true;
	}
	else if( bytesReceived > 0 )
	{
		std::printf( "Server received message: %s\n\n", receiveBuffer );
		TCPSendMessage( acceptedClientSocket, receiveBuffer, bytesReceived );
		
		return true;
	}
	else
	{
		std::printf( "recv failed: %d\n", WSAGetLastError() );
		return false;
	}
}


//-----------------------------------------------------------------------------------------------
bool TCPReceiveMessageClient( const SOCKET& clientSocket )
{
	char receiveBuffer[ BUFFER_LIMIT ];
	memset( receiveBuffer, '\0', BUFFER_LIMIT );

	int bytesReceived = 0;

	bytesReceived = recv( clientSocket, receiveBuffer, BUFFER_LIMIT, 0 );

	if( bytesReceived == 0 )
	{
		return true;
	}
	else if( bytesReceived > 0 )
	{
		std::printf( "Client received message: %s\n\n", receiveBuffer );

		return true;
	}
	else
	{
		std::printf( "recv failed: %d\n", WSAGetLastError() );
		return false;
	}
	return false;
}


//-----------------------------------------------------------------------------------------------
bool UDPSendMessageServer( const SOCKET& socketToSendTo )
{
	std::string sendAsString;

	system( "cls" );
	std::printf( "Type Message To Send: ");

	std::cin >> sendAsString;

	int sendResultAsInt;

	sendResultAsInt = sendto( socketToSendTo, sendAsString.c_str(), sendAsString.size(), 0, UDPClientAddrInfo->ai_addr, sizeof( addrinfo ) );

	if ( sendResultAsInt == SOCKET_ERROR ) 
	{
		printf( "send failed with error: %d\n", WSAGetLastError() );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool UDPSendMessageServer( const SOCKET& socketToSendTo, const char* message, int messageLength )
{
	int sendResultAsInt;

	sendResultAsInt = sendto( socketToSendTo, message, messageLength, 0, UDPClientAddrInfo->ai_addr, sizeof( addrinfo ) );

	if ( sendResultAsInt == SOCKET_ERROR ) 
	{
		printf( "send failed with error: %d\n", WSAGetLastError() );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool UDPSendMessageClient( const SOCKET& socketToSendTo )
{
	std::string sendAsString;

	system( "cls" );
	std::printf( "Type Message To Send: ");

	std::cin >> sendAsString;

	int sendResultAsInt;

	sendResultAsInt = sendto( socketToSendTo, sendAsString.c_str(), sendAsString.size(), 0, UDPServerAddrInfo->ai_addr, sizeof( addrinfo ) );

	if ( sendResultAsInt == SOCKET_ERROR ) 
	{
		printf( "send failed with error: %d\n", WSAGetLastError() );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool UDPReceiveMessageServer( const SOCKET& serverSocket )
{
	char receiveBuffer[ BUFFER_LIMIT ];
	memset( receiveBuffer, '\0', BUFFER_LIMIT );

	int bytesReceived = 0;

	int addrInfoLength = sizeof( addrinfo );

	bytesReceived = recvfrom( serverSocket, receiveBuffer, BUFFER_LIMIT, 0, UDPClientAddrInfo->ai_addr, &addrInfoLength );

	if( bytesReceived == 0 )
	{
		return true;
	}
	else if( bytesReceived > 0 )
	{
		std::printf( "Server received message: %s\n\n", receiveBuffer );
		UDPSendMessageServer( serverSocket, receiveBuffer, bytesReceived );

		return true;
	}
	else
	{
		std::printf( "recv failed: %d\n", WSAGetLastError() );
		return false;
	}
}


//-----------------------------------------------------------------------------------------------
bool UDPReceiveMessageClient( const SOCKET& clientSocket )
{
	char receiveBuffer[ BUFFER_LIMIT ];
	memset( receiveBuffer, '\0', BUFFER_LIMIT );

	int bytesReceived = 0;

	int addrInfoLength = sizeof( addrinfo );

	bytesReceived = recvfrom( clientSocket, receiveBuffer, BUFFER_LIMIT, 0, UDPServerAddrInfo->ai_addr, &addrInfoLength );

	if( bytesReceived == 0 )
	{
		return true;
	}
	else if( bytesReceived > 0 )
	{
		std::printf( "Client received message: %s\n\n", receiveBuffer );

		return true;
	}
	else
	{
		std::printf( "recv failed: %d\n", WSAGetLastError() );
		return false;
	}
	return false;
}


//-----------------------------------------------------------------------------------------------
bool SetupTCPClientServer( const std::string& ipaddress, const std::string& tcpPort, SOCKET& tcpServerSocket_out, SOCKET& tcpAcceptedClientSocket_out, SOCKET& tcpClientSocket_out )
{
	bool wasAllInitializationSuccessful = true;

	addrinfo* TCPAddressInfo = nullptr;

	wasAllInitializationSuccessful &= InitializeWinSocket();

	wasAllInitializationSuccessful &= CreateAddressInfoForTCP( ipaddress, tcpPort, TCPAddressInfo );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	wasAllInitializationSuccessful &= CreateAndSetupTCPServerToListen( TCPAddressInfo, tcpServerSocket_out );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	wasAllInitializationSuccessful &= CreateSocket( TCPAddressInfo, tcpClientSocket_out );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	wasAllInitializationSuccessful &= ConnectSocket( tcpClientSocket_out, TCPAddressInfo );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	wasAllInitializationSuccessful &= AcceptClientSocket( tcpServerSocket_out, tcpAcceptedClientSocket_out );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	freeaddrinfo( TCPAddressInfo );

	return true;
}


//-----------------------------------------------------------------------------------------------
bool SetupUDPClientServer( const std::string& ipaddress, const std::string& udpPort, SOCKET& udpServerSocket_out, SOCKET& udpClientSocket_out )
{
	bool wasAllInitializationSuccessful = true;

	addrinfo* UDPAddressInfo = nullptr;

	wasAllInitializationSuccessful &= InitializeWinSocket();

	wasAllInitializationSuccessful &= CreateAddressInfoForUDP( ipaddress, udpPort, UDPAddressInfo );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	wasAllInitializationSuccessful &= CreateSocket( UDPAddressInfo, udpServerSocket_out );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	wasAllInitializationSuccessful &= BindSocket( udpServerSocket_out, UDPAddressInfo );
	
	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	wasAllInitializationSuccessful &= CreateSocket( UDPAddressInfo, udpClientSocket_out );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}
	
	wasAllInitializationSuccessful &= CreateAddressInfoForUDP( ipaddress, udpClientPortAsString, UDPClientAddrInfo );

	wasAllInitializationSuccessful &= BindSocket( udpClientSocket_out, UDPClientAddrInfo );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	UDPServerAddrInfo = UDPAddressInfo;

	return true;
}


//-----------------------------------------------------------------------------------------------
bool RunTCPClientServer( SOCKET& tcpServerSocket, SOCKET& tcpAcceptedClientSocket, SOCKET& tcpClientSocket )
{
	std::string currentInputAsString = "";

	system( "cls" );

	bool receiveAndSendSuccessful = true;

	while( currentInputAsString != "QUIT" && receiveAndSendSuccessful )
	{
		currentInputAsString = "";

		std::printf( "Type 'server' to send messages from server to client, or\n    'client' to send messages from client to server, or\n    'quit' to quit application: " );

		std::cin >> currentInputAsString;

		ToUpperCaseString( currentInputAsString );

		if( currentInputAsString == "SERVER" )
		{
			receiveAndSendSuccessful &= TCPSendMessage( tcpAcceptedClientSocket );
			receiveAndSendSuccessful &= TCPReceiveMessageClient( tcpClientSocket );
		}
		else if( currentInputAsString == "CLIENT" )
		{
			receiveAndSendSuccessful &= TCPSendMessage( tcpClientSocket );
			receiveAndSendSuccessful &= TCPReceiveMessageServer( tcpServerSocket, tcpAcceptedClientSocket );
			receiveAndSendSuccessful &= TCPReceiveMessageClient( tcpClientSocket );
		}
		else
		{
			system( "cls" );
		}
	}

	return receiveAndSendSuccessful;
}


//-----------------------------------------------------------------------------------------------
bool RunUDPClientServer( SOCKET& udpServerSocket, SOCKET& udpClientSocket )
{
	std::string currentInputAsString = "";

	system( "cls" );

	bool receiveAndSendSuccessful = true;

	while( currentInputAsString != "QUIT" && receiveAndSendSuccessful )
	{
		currentInputAsString = "";

		std::printf( "Type 'server' to send messages from server to client, or\n    'client' to send messages from client to server, or\n    'quit' to quit application: " );

		std::cin >> currentInputAsString;

		ToUpperCaseString( currentInputAsString );

		if( currentInputAsString == "SERVER" )
		{
			receiveAndSendSuccessful &= UDPSendMessageServer( udpServerSocket );
			receiveAndSendSuccessful &= UDPReceiveMessageClient( udpClientSocket );
		}
		else if( currentInputAsString == "CLIENT" )
		{
			receiveAndSendSuccessful &= UDPSendMessageClient( udpClientSocket );
			receiveAndSendSuccessful &= UDPReceiveMessageServer( udpServerSocket );
			receiveAndSendSuccessful &= UDPReceiveMessageClient( udpClientSocket );
		}
		else
		{
			system( "cls" );
		}
	}

	return receiveAndSendSuccessful;

	return false;
}


//-----------------------------------------------------------------------------------------------
bool CreateAndSetupTCPServerToListen( addrinfo*& addressInfo, SOCKET& newSocket )
{
	bool wasAllInitializationSuccessful = true;

	wasAllInitializationSuccessful = CreateSocket( addressInfo, newSocket );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	wasAllInitializationSuccessful = BindSocket( newSocket, addressInfo );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	wasAllInitializationSuccessful = SetSocketListen( newSocket );

	if( !wasAllInitializationSuccessful )
	{
		return false;
	}

	return true;
}
