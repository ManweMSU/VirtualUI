#include "Socket.h"

#include "Punycode.h"
#include "../Miscellaneous/DynamicString.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>
#include <netdb.h>

namespace Engine
{
	namespace Network
	{
        typedef int SOCKET;
		class UnixSocket : public Socket
		{
			SOCKET handle;
			bool ipv6;
		public:
			UnixSocket(SOCKET object, bool is_ipv6) : handle(object), ipv6(is_ipv6) {}
			~UnixSocket(void) override
			{
				shutdown(handle, SHUT_RDWR);
				close(handle);
			}
			virtual void Read(void * buffer, uint32 length) override
			{
                int read = 0;
                while (true) {
                    read = recv(handle, reinterpret_cast<char *>(buffer), length, MSG_WAITALL);
                    if (read == -1) {
                        if (errno != EINTR) throw IO::FileAccessException();
                    } else break;
                }
				if (read != length) throw IO::FileReadEndOfFileException(read);
			}
			virtual void Write(const void * data, uint32 length) override
			{
                while (true) {
                    int result = send(handle, reinterpret_cast<const char *>(data), length, 0);
                    if (result == -1 && errno != EINTR) throw IO::FileAccessException();
                    else if (result >= 0) return;
                }
			}
			virtual int64 Seek(int64 position, Streaming::SeekOrigin origin) override { throw IO::FileAccessException(); }
			virtual uint64 Length(void) override { throw IO::FileAccessException(); }
			virtual void SetLength(uint64 length) override { throw IO::FileAccessException(); }
			virtual void Flush(void) override {}
			virtual void Connect(const Address & address, uint16 port) override
			{
				if (ipv6) {
					sockaddr_in6 addr;
					addr.sin6_family = AF_INET6;
					addr.sin6_port = InverseEndianess(port);
					addr.sin6_flowinfo = 0;
                    addr.sin6_addr.s6_addr[0] = uint8((address.DWord4 & 0xFF000000) >> 24);
                    addr.sin6_addr.s6_addr[1] = uint8((address.DWord4 & 0x00FF0000) >> 16);
                    addr.sin6_addr.s6_addr[2] = uint8((address.DWord4 & 0x0000FF00) >> 8);
                    addr.sin6_addr.s6_addr[3] = uint8((address.DWord4 & 0x000000FF));
                    addr.sin6_addr.s6_addr[4] = uint8((address.DWord3 & 0xFF000000) >> 24);
                    addr.sin6_addr.s6_addr[5] = uint8((address.DWord3 & 0x00FF0000) >> 16);
                    addr.sin6_addr.s6_addr[6] = uint8((address.DWord3 & 0x0000FF00) >> 8);
                    addr.sin6_addr.s6_addr[7] = uint8((address.DWord3 & 0x000000FF));
                    addr.sin6_addr.s6_addr[8] = uint8((address.DWord2 & 0xFF000000) >> 24);
                    addr.sin6_addr.s6_addr[9] = uint8((address.DWord2 & 0x00FF0000) >> 16);
                    addr.sin6_addr.s6_addr[10] = uint8((address.DWord2 & 0x0000FF00) >> 8);
                    addr.sin6_addr.s6_addr[11] = uint8((address.DWord2 & 0x000000FF));
                    addr.sin6_addr.s6_addr[12] = uint8((address.DWord1 & 0xFF000000) >> 24);
                    addr.sin6_addr.s6_addr[13] = uint8((address.DWord1 & 0x00FF0000) >> 16);
                    addr.sin6_addr.s6_addr[14] = uint8((address.DWord1 & 0x0000FF00) >> 8);
                    addr.sin6_addr.s6_addr[15] = uint8((address.DWord1 & 0x000000FF));
					addr.sin6_scope_id = 0;
                    while (true) {
                        int result = connect(handle, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
                        if (result == -1 && errno != EINTR) throw IO::FileAccessException();
                        else if (result >= 0) break;
                    }
				} else {
					if (!address.IsIPv4()) throw InvalidArgumentException();
					sockaddr_in addr;
					addr.sin_family = AF_INET;
					addr.sin_port = InverseEndianess(port);
					addr.sin_addr.s_addr = InverseEndianess(address.IPv4);
					ZeroMemory(&addr.sin_zero, sizeof(addr.sin_zero));
                    while (true) {
                        int result = connect(handle, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
                        if (result == -1 && errno != EINTR) throw IO::FileAccessException();
                        else if (result >= 0) break;
                    }
				}
			}
			virtual void Bind(const Address & address, uint16 port) override
			{
				if (ipv6) {
					sockaddr_in6 addr;
					addr.sin6_family = AF_INET6;
					addr.sin6_port = InverseEndianess(port);
					addr.sin6_flowinfo = 0;
					addr.sin6_addr.s6_addr[0] = uint8((address.DWord4 & 0xFF000000) >> 24);
                    addr.sin6_addr.s6_addr[1] = uint8((address.DWord4 & 0x00FF0000) >> 16);
                    addr.sin6_addr.s6_addr[2] = uint8((address.DWord4 & 0x0000FF00) >> 8);
                    addr.sin6_addr.s6_addr[3] = uint8((address.DWord4 & 0x000000FF));
                    addr.sin6_addr.s6_addr[4] = uint8((address.DWord3 & 0xFF000000) >> 24);
                    addr.sin6_addr.s6_addr[5] = uint8((address.DWord3 & 0x00FF0000) >> 16);
                    addr.sin6_addr.s6_addr[6] = uint8((address.DWord3 & 0x0000FF00) >> 8);
                    addr.sin6_addr.s6_addr[7] = uint8((address.DWord3 & 0x000000FF));
                    addr.sin6_addr.s6_addr[8] = uint8((address.DWord2 & 0xFF000000) >> 24);
                    addr.sin6_addr.s6_addr[9] = uint8((address.DWord2 & 0x00FF0000) >> 16);
                    addr.sin6_addr.s6_addr[10] = uint8((address.DWord2 & 0x0000FF00) >> 8);
                    addr.sin6_addr.s6_addr[11] = uint8((address.DWord2 & 0x000000FF));
                    addr.sin6_addr.s6_addr[12] = uint8((address.DWord1 & 0xFF000000) >> 24);
                    addr.sin6_addr.s6_addr[13] = uint8((address.DWord1 & 0x00FF0000) >> 16);
                    addr.sin6_addr.s6_addr[14] = uint8((address.DWord1 & 0x0000FF00) >> 8);
                    addr.sin6_addr.s6_addr[15] = uint8((address.DWord1 & 0x000000FF));
					addr.sin6_scope_id = 0;
					if (bind(handle, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) throw IO::FileAccessException();
				} else {
					if (!address.IsIPv4()) throw InvalidArgumentException();
					sockaddr_in addr;
					addr.sin_family = AF_INET;
					addr.sin_port = InverseEndianess(port);
					addr.sin_addr.s_addr = InverseEndianess(address.IPv4);
					ZeroMemory(&addr.sin_zero, sizeof(addr.sin_zero));
					if (bind(handle, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) throw IO::FileAccessException();
				}
			}
			virtual void Bind(uint16 port) override { Bind(Address::CreateAny(), port); }
			virtual void Listen(void) override
			{
				if (listen(handle, SOMAXCONN) == -1) throw IO::FileAccessException();
			}
			virtual Socket * Accept() override
			{
				SOCKET new_socket;
                while (true) {
                    new_socket = accept(handle, 0, 0);
                    if (new_socket == -1 && errno != EINTR) return 0;
                    else if (new_socket >= 0) break;
                }
				return new UnixSocket(new_socket, ipv6);
			}
			virtual Socket * Accept(Address & address, uint16 & port) override
			{
				SOCKET new_socket = -1;
				if (ipv6) {
					sockaddr_in6 addr;
					socklen_t addr_len = sizeof(addr);
                    while (true) {
                        new_socket = accept(handle, reinterpret_cast<sockaddr *>(&addr), &addr_len);
                        if (new_socket == -1 && errno != EINTR) return 0;
                        else if (new_socket >= 0) break;
                    }
                    port = InverseEndianess(addr.sin6_port);
                    address.DWord1 = uint32(addr.sin6_addr.s6_addr[15]) | (uint32(addr.sin6_addr.s6_addr[14]) << 8) |
                        (uint32(addr.sin6_addr.s6_addr[13]) << 16) | (uint32(addr.sin6_addr.s6_addr[12]) << 24);
                    address.DWord2 = uint32(addr.sin6_addr.s6_addr[11]) | (uint32(addr.sin6_addr.s6_addr[10]) << 8) |
                        (uint32(addr.sin6_addr.s6_addr[9]) << 16) | (uint32(addr.sin6_addr.s6_addr[8]) << 24);
                    address.DWord3 = uint32(addr.sin6_addr.s6_addr[7]) | (uint32(addr.sin6_addr.s6_addr[6]) << 8) |
                        (uint32(addr.sin6_addr.s6_addr[5]) << 16) | (uint32(addr.sin6_addr.s6_addr[4]) << 24);
                    address.DWord4 = uint32(addr.sin6_addr.s6_addr[3]) | (uint32(addr.sin6_addr.s6_addr[2]) << 8) |
                        (uint32(addr.sin6_addr.s6_addr[1]) << 16) | (uint32(addr.sin6_addr.s6_addr[0]) << 24);
				} else {
					sockaddr_in addr;
					socklen_t addr_len = sizeof(addr);
                    while (true) {
                        new_socket = accept(handle, reinterpret_cast<sockaddr *>(&addr), &addr_len);
                        if (new_socket == -1 && errno != EINTR) return 0;
                        else if (new_socket >= 0) break;
                    }
                    port = InverseEndianess(addr.sin_port);
                    address = Address::CreateIPv4(InverseEndianess(uint32(addr.sin_addr.s_addr)));
				}
				return new UnixSocket(new_socket, ipv6);
			}
			virtual void Shutdown(bool close_read, bool close_write) override
			{
				int sd = 0;
				if (close_read && !close_write) sd = SHUT_RD;
				else if (!close_read && close_write) sd = SHUT_WR;
				else if (close_read && close_write) sd = SHUT_RDWR;
				else return;
				shutdown(handle, sd);
			}
		};
		Socket * CreateSocket(SocketAddressDomain domain, SocketProtocol protocol)
		{
			if (domain == SocketAddressDomain::Unknown) throw InvalidArgumentException();
			if (protocol == SocketProtocol::Unknown) throw InvalidArgumentException();	
			int socket_domain = 0;
			int socket_type = 0;
			int socket_protocol = 0;
			if (domain == SocketAddressDomain::IP) socket_domain = AF_INET;
			else if (domain == SocketAddressDomain::IPv6) socket_domain = AF_INET6;
			if (protocol == SocketProtocol::TCP) {
				socket_type = SOCK_STREAM;
				socket_protocol = IPPROTO_TCP;
			} else if (protocol == SocketProtocol::UDP) {
				socket_type = SOCK_DGRAM;
				socket_protocol = IPPROTO_UDP;
			}
			SOCKET handle = socket(socket_domain, socket_type, socket_protocol);
			if (domain == SocketAddressDomain::IPv6) {
				uint32 value = 0;
				setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char *>(&value), 4);
			}
			return (handle == -1) ? 0 : new UnixSocket(handle, domain == SocketAddressDomain::IPv6);
		}
		Address::Address(void) {}
		bool Address::IsIPv4(void) const { return DWord2 == 0x0000FFFF && DWord3 == 0 && DWord4 == 0; }
		Address::operator string(void) const
		{
			if (IsIPv4()) {
				return string((IPv4 & 0xFF000000) >> 24) + L"." + string((IPv4 & 0xFF0000) >> 16) + L"." +
					string((IPv4 & 0xFF00) >> 8) + L"." + string(IPv4 & 0xFF);
			} else {
				DynamicString result;
				result += string((DWord4 & 0xFFFF0000) >> 16, L"0123456789abcdef", 4) + L":";
				result += string(DWord4 & 0xFFFF, L"0123456789abcdef", 4) + L":";
				result += string((DWord3 & 0xFFFF0000) >> 16, L"0123456789abcdef", 4) + L":";
				result += string(DWord3 & 0xFFFF, L"0123456789abcdef", 4) + L":";
				result += string((DWord2 & 0xFFFF0000) >> 16, L"0123456789abcdef", 4) + L":";
				result += string(DWord2 & 0xFFFF, L"0123456789abcdef", 4) + L":";
				result += string((DWord1 & 0xFFFF0000) >> 16, L"0123456789abcdef", 4) + L":";
				result += string(DWord1 & 0xFFFF, L"0123456789abcdef", 4);
				return result.ToString();
			}
		}
		Address Address::CreateIPv4(uint32 address)
		{
			Address result;
			result.IPv4 = address;
			result.DWord2 = 0x0000FFFF;
			result.DWord3 = 0;
			result.DWord4 = 0;
			return result;
		}
		Address Address::CreateIPv6(uint32 dword_1, uint32 dword_2, uint32 dword_3, uint32 dword_4)
		{
			Address result;
			result.DWord1 = dword_1;
			result.DWord2 = dword_2;
			result.DWord3 = dword_3;
			result.DWord4 = dword_4;
			return result;
		}
		Address Address::CreateLoopBackIPv4(void) { return CreateIPv4(0x7F000001); }
		Address Address::CreateLoopBackIPv6(void) { return CreateIPv6(1, 0, 0, 0); }
		Address Address::CreateAny(void) { return CreateIPv6(0, 0, 0, 0); }
		uint32 InverseEndianess(uint32 value) { return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24); }
		uint16 InverseEndianess(uint16 value) { return ((value & 0xFF00) >> 8) | ((value & 0xFF) << 8); }
		Array<AddressEntity>* GetAddressByHost(const string & host_name, uint16 host_port, SocketAddressDomain domain, SocketProtocol protocol)
		{
            string wname = DomainNameToPunycode(host_name);
            SafePointer< Array<uint8> > name = wname.EncodeSequence(Encoding::ANSI, true);
            SafePointer< Array<uint8> > port = string(host_port).EncodeSequence(Encoding::ANSI, true);
            addrinfo * info = 0;
            addrinfo * current = 0;
            addrinfo req;
            ZeroMemory(&req, sizeof(req));
            req.ai_flags = AI_CANONNAME;
            if (domain == SocketAddressDomain::IPv6) req.ai_flags |= AI_ALL;
            if (domain == SocketAddressDomain::IPv4) req.ai_family = AF_INET;
            else if (domain == SocketAddressDomain::IPv6) req.ai_family = AF_UNSPEC;
            if (protocol == SocketProtocol::TCP) req.ai_protocol = IPPROTO_TCP;
            else if (protocol == SocketProtocol::UDP) req.ai_protocol = IPPROTO_UDP;
            int error = 0;
            error = getaddrinfo(reinterpret_cast<char *>(name->GetBuffer()),
                reinterpret_cast<char *>(port->GetBuffer()), &req, &info);
            if (error) {
                if (error == EAI_NONAME || error == EAI_SERVICE || error == EAI_ADDRFAMILY || error == EAI_NODATA) return new Array<AddressEntity>(0x10);
                else throw Exception();
            }
            SafePointer< Array<AddressEntity> > result = new Array<AddressEntity>(0x10);
            current = info;
            string cname = L"";
            while (current) {
                AddressEntity New;
                if (current->ai_family == AF_INET) New.EntityDomain = SocketAddressDomain::IPv4;
                else if (current->ai_family == AF_INET6) New.EntityDomain = SocketAddressDomain::IPv6;
                else New.EntityDomain = SocketAddressDomain::Unknown;
                if (current->ai_family == AF_INET) {
                    sockaddr_in * addr = reinterpret_cast<sockaddr_in *>(current->ai_addr);
                    New.EntityAddress = Address::CreateIPv4(InverseEndianess(uint32(addr->sin_addr.s_addr)));
                } else if (current->ai_family == AF_INET6) {
                    sockaddr_in6 * addr = reinterpret_cast<sockaddr_in6 *>(current->ai_addr);
                    New.EntityAddress.DWord1 = uint32(addr->sin6_addr.s6_addr[15]) | (uint32(addr->sin6_addr.s6_addr[14]) << 8) |
                        (uint32(addr->sin6_addr.s6_addr[13]) << 16) | (uint32(addr->sin6_addr.s6_addr[12]) << 24);
                    New.EntityAddress.DWord2 = uint32(addr->sin6_addr.s6_addr[11]) | (uint32(addr->sin6_addr.s6_addr[10]) << 8) |
                        (uint32(addr->sin6_addr.s6_addr[9]) << 16) | (uint32(addr->sin6_addr.s6_addr[8]) << 24);
                    New.EntityAddress.DWord3 = uint32(addr->sin6_addr.s6_addr[7]) | (uint32(addr->sin6_addr.s6_addr[6]) << 8) |
                        (uint32(addr->sin6_addr.s6_addr[5]) << 16) | (uint32(addr->sin6_addr.s6_addr[4]) << 24);
                    New.EntityAddress.DWord4 = uint32(addr->sin6_addr.s6_addr[3]) | (uint32(addr->sin6_addr.s6_addr[2]) << 8) |
                        (uint32(addr->sin6_addr.s6_addr[1]) << 16) | (uint32(addr->sin6_addr.s6_addr[0]) << 24);
                } else {
                    New.EntityAddress = Address::CreateAny();
                }
                if (!cname.Length()) cname = string(current->ai_canonname, -1, Encoding::ANSI);
                New.EntityName = cname;
                result->Append(New);
                current = current->ai_next;
            }
            freeaddrinfo(info);
            result->Retain();
            return result;
		}
		AddressEntity::operator string(void) const
		{
			return string(EntityAddress) + L" \"" + EntityName + L"\" " +
				(EntityDomain == SocketAddressDomain::IPv4 ? L"IPv4" : L"IPv6");
		}
	}
}