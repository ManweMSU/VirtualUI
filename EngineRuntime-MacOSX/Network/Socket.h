#pragma once

#include "../Streaming.h"

namespace Engine
{
	namespace Network
	{
		enum class SocketAddressDomain { Unknown = 0, IP = 1, IPv4 = 1, IPv6 = 2 };
		enum class SocketProtocol { Unknown = 0, TCP = 1, UDP = 2 };

		class Address
		{
		public:
			union {
				uint32 DWord1;
				uint32 IPv4;
			};
			uint32 DWord2;
			uint32 DWord3;
			uint32 DWord4;

			Address(void);

			bool IsIPv4(void) const;
			operator string(void) const;

			static Address CreateIPv4(uint32 address);
			static Address CreateIPv6(uint32 dword_1, uint32 dword_2, uint32 dword_3, uint32 dword_4);
			static Address CreateLoopBackIPv4(void);
			static Address CreateLoopBackIPv6(void);
			static Address CreateAny(void);
		};

		class AddressEntity
		{
		public:
			Address EntityAddress;
			SocketAddressDomain EntityDomain;
			string EntityName;

			operator string(void) const;
		};

		class Socket : public Streaming::Stream
		{
		public:
			virtual void Connect(const Address & address, uint16 port) = 0;
			virtual void Bind(const Address & address, uint16 port) = 0;
			virtual void Bind(uint16 port) = 0;
			virtual void Listen(void) = 0;
			virtual Socket * Accept() = 0;
			virtual Socket * Accept(Address & address, uint16 & port) = 0;
			virtual void Shutdown(bool close_read, bool close_write) = 0;
		};
		Socket * CreateSocket(SocketAddressDomain domain, SocketProtocol protocol);
		uint32 InverseEndianess(uint32 value);
		uint16 InverseEndianess(uint16 value);
		Array<AddressEntity> * GetAddressByHost(const string & host_name, uint16 host_port, SocketAddressDomain domain, SocketProtocol protocol);
	}
}