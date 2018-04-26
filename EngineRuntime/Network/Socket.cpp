#include "Socket.h"

#include "../Miscellaneous/DynamicString.h"

#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Normaliz.lib")

#undef ZeroMemory
#undef min
#undef max

namespace Engine
{
	namespace Network
	{
		class WinSocket : public Socket
		{
			SOCKET handle;
			bool ipv6;
		public:
			WinSocket(SOCKET object, bool is_ipv6) : handle(object), ipv6(is_ipv6) {}
			~WinSocket(void) override
			{
				shutdown(handle, SD_BOTH);
				closesocket(handle);
				WSACleanup();
			}
			virtual void Read(void * buffer, uint32 length) override
			{
				int read = recv(handle, reinterpret_cast<char *>(buffer), length, MSG_WAITALL);
				if (read == SOCKET_ERROR) throw IO::FileAccessException();
				if (read != length) throw IO::FileReadEndOfFileException(read);
			}
			virtual void Write(const void * data, uint32 length) override
			{
				if (send(handle, reinterpret_cast<const char *>(data), length, 0) == SOCKET_ERROR) throw IO::FileAccessException();
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
					addr.sin6_addr.u.Word[0] = InverseEndianess(uint16((address.DWord4 & 0xFFFF0000) >> 16));
					addr.sin6_addr.u.Word[1] = InverseEndianess(uint16(address.DWord4 & 0xFFFF));
					addr.sin6_addr.u.Word[2] = InverseEndianess(uint16((address.DWord3 & 0xFFFF0000) >> 16));
					addr.sin6_addr.u.Word[3] = InverseEndianess(uint16(address.DWord3 & 0xFFFF));
					addr.sin6_addr.u.Word[4] = InverseEndianess(uint16((address.DWord2 & 0xFFFF0000) >> 16));
					addr.sin6_addr.u.Word[5] = InverseEndianess(uint16(address.DWord2 & 0xFFFF));
					addr.sin6_addr.u.Word[6] = InverseEndianess(uint16((address.DWord1 & 0xFFFF0000) >> 16));
					addr.sin6_addr.u.Word[7] = InverseEndianess(uint16(address.DWord1 & 0xFFFF));
					addr.sin6_scope_id = 0;
					if (connect(handle, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) throw IO::FileAccessException();
				} else {
					if (!address.IsIPv4()) throw InvalidArgumentException();
					sockaddr_in addr;
					addr.sin_family = AF_INET;
					addr.sin_port = InverseEndianess(port);
					addr.sin_addr.S_un.S_addr = InverseEndianess(address.IPv4);
					ZeroMemory(&addr.sin_zero, sizeof(addr.sin_zero));
					if (connect(handle, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) throw IO::FileAccessException();
				}
			}
			virtual void Bind(const Address & address, uint16 port) override
			{
				if (ipv6) {
					sockaddr_in6 addr;
					addr.sin6_family = AF_INET6;
					addr.sin6_port = InverseEndianess(port);
					addr.sin6_flowinfo = 0;
					addr.sin6_addr.u.Word[0] = InverseEndianess(uint16((address.DWord4 & 0xFFFF0000) >> 16));
					addr.sin6_addr.u.Word[1] = InverseEndianess(uint16(address.DWord4 & 0xFFFF));
					addr.sin6_addr.u.Word[2] = InverseEndianess(uint16((address.DWord3 & 0xFFFF0000) >> 16));
					addr.sin6_addr.u.Word[3] = InverseEndianess(uint16(address.DWord3 & 0xFFFF));
					addr.sin6_addr.u.Word[4] = InverseEndianess(uint16((address.DWord2 & 0xFFFF0000) >> 16));
					addr.sin6_addr.u.Word[5] = InverseEndianess(uint16(address.DWord2 & 0xFFFF));
					addr.sin6_addr.u.Word[6] = InverseEndianess(uint16((address.DWord1 & 0xFFFF0000) >> 16));
					addr.sin6_addr.u.Word[7] = InverseEndianess(uint16(address.DWord1 & 0xFFFF));
					addr.sin6_scope_id = 0;
					if (bind(handle, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) throw IO::FileAccessException();
				} else {
					if (!address.IsIPv4()) throw InvalidArgumentException();
					sockaddr_in addr;
					addr.sin_family = AF_INET;
					addr.sin_port = InverseEndianess(port);
					addr.sin_addr.S_un.S_addr = InverseEndianess(address.IPv4);
					ZeroMemory(&addr.sin_zero, sizeof(addr.sin_zero));
					if (bind(handle, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) throw IO::FileAccessException();
				}
			}
			virtual void Bind(uint16 port) override { Bind(Address::CreateAny(), port); }
			virtual void Listen(void) override
			{
				if (listen(handle, SOMAXCONN) == SOCKET_ERROR) throw IO::FileAccessException();
			}
			virtual Socket * Accept() override
			{
				SOCKET new_socket = accept(handle, 0, 0);
				if (new_socket != INVALID_SOCKET) return new WinSocket(new_socket, ipv6); else return 0;
			}
		};
		Socket * CreateSocket(SocketAddressDomain domain, SocketProtocol protocol)
		{
			if (domain == SocketAddressDomain::Unknown) throw InvalidArgumentException();
			if (protocol == SocketProtocol::Unknown) throw InvalidArgumentException();	
			WSADATA data;
			ZeroMemory(&data, sizeof(data));
			if (WSAStartup(MAKEWORD(2, 2), &data)) return 0;
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
				DWORD value = FALSE;
				setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char *>(&value), 4);
			}
			if (handle == INVALID_SOCKET) {
				WSACleanup();
			}
			return (handle == INVALID_SOCKET) ? 0 : new WinSocket(handle, domain == SocketAddressDomain::IPv6);
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
		string InternalConverter(const string & text)
		{
			bool regular = true;
			for (int i = 0; i < text.Length(); i++) if (text[i] & 0xFFFFFF80) regular = false;
			if (regular) return text;
			Array<uint32> ucs(0x10);
			ucs.SetLength(text.GetEncodedLength(Encoding::UTF32) + 1);
			text.Encode(ucs.GetBuffer(), Encoding::UTF32, true);
			DynamicString result;
			int run_length = 0;
			uint32 max_char = 0;
			for (int i = 0; i < ucs.Length() - 1; i++) {
				if (ucs[i] > max_char) max_char = ucs[i];
				if (ucs[i] < 128) {
					run_length++;
					result += string(ucs.GetBuffer() + i, 1, Encoding::UTF32);
				}
			}
			if (run_length) result += L'-';
			run_length++;
			uint32 cur_char = 128;
			int state = 0;
			int last_state = -1;
			int bias = 72;
			while (cur_char <= max_char) {
				for (int i = 0; i < ucs.Length(); i++) {
					if (ucs[i] == cur_char) {
						int ds = state - last_state - 1;
						if (ds) {
							int ids = ds;
							int div = 0;
							while (true) {
								div += 36;
								int edge = max(min(div - bias, 26), 1);
								if (ds < edge) break;
								result += L"abcdefghijklmnopqrstuvwxyz0123456789"[edge + ((ds - edge) % (36 - edge))];
								ds = (ds - edge) / (36 - edge);
							}
							result += L"abcdefghijklmnopqrstuvwxyz0123456789"[ds];
							if (last_state == -1) ids /= 700;
							else ids /= 2;
							ids += (ids / (run_length));
							int k = 0;
							while (ids > (35 * 26) / 2) {
								ids /= 35;
								k += 36;
							}
							bias = k + ((36 * ids) / (ids + 38));
						} else {
							result += L'a';
						}
						last_state = state;
						run_length++;
					}
					if (ucs[i] <= cur_char) state++;
				}
				cur_char++;
			}
			return L"xn--" + result;
		}
		string ConvertToPunycode(const string & text)
		{
			auto domains = text.Split(L'.');
			DynamicString result;
			for (int i = 0; i < domains.Length(); i++) {
				if (i) result += L'.';
				result += InternalConverter(domains[i].LowerCase());
			}
			return result;
		}
		Array<AddressEntity>* GetAddressByHost(const string & host_name, uint16 host_port, SocketAddressDomain domain, SocketProtocol protocol)
		{
			WSADATA data;
			ZeroMemory(&data, sizeof(data));
			if (WSAStartup(MAKEWORD(2, 2), &data)) return 0;
			try {
				string wname = ConvertToPunycode(host_name);
				SafePointer< Array<uint8> > name = wname.EncodeSequence(Encoding::ANSI, true);
				SafePointer< Array<uint8> > port = string(host_port).EncodeSequence(Encoding::ANSI, true);
				addrinfo * info = 0;
				addrinfo * current = 0;
				addrinfo req;
				ZeroMemory(&req, sizeof(req));
				req.ai_flags = AI_FQDN;
				if (domain == SocketAddressDomain::IPv6) req.ai_flags |= AI_ALL;
				if (domain == SocketAddressDomain::IPv4) req.ai_family = AF_INET;
				else if (domain == SocketAddressDomain::IPv6) req.ai_family = AF_UNSPEC;
				if (protocol == SocketProtocol::TCP) req.ai_protocol = IPPROTO_TCP;
				else if (protocol == SocketProtocol::UDP) req.ai_protocol = IPPROTO_UDP;
				int error = 0;
				error = getaddrinfo(reinterpret_cast<char *>(name->GetBuffer()),
					reinterpret_cast<char *>(port->GetBuffer()), &req, &info);
				if (error) {
					if (WSAGetLastError() == WSANO_DATA || WSAGetLastError() == WSAHOST_NOT_FOUND) return new Array<AddressEntity>(0x10);
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
						New.EntityAddress = Address::CreateIPv4(InverseEndianess(uint32(addr->sin_addr.S_un.S_addr)));
					} else if (current->ai_family == AF_INET6) {
						sockaddr_in6 * addr = reinterpret_cast<sockaddr_in6 *>(current->ai_addr);
						New.EntityAddress.DWord1 = InverseEndianess(addr->sin6_addr.u.Word[7]) |
							(uint32(InverseEndianess(addr->sin6_addr.u.Word[6])) << 16);
						New.EntityAddress.DWord2 = InverseEndianess(addr->sin6_addr.u.Word[5]) |
							(uint32(InverseEndianess(addr->sin6_addr.u.Word[4])) << 16);
						New.EntityAddress.DWord3 = InverseEndianess(addr->sin6_addr.u.Word[3]) |
							(uint32(InverseEndianess(addr->sin6_addr.u.Word[2])) << 16);
						New.EntityAddress.DWord4 = InverseEndianess(addr->sin6_addr.u.Word[1]) |
							(uint32(InverseEndianess(addr->sin6_addr.u.Word[0])) << 16);
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
			catch (...) {
				WSACleanup();
				throw;
			}
		}
		AddressEntity::operator string(void) const
		{
			return string(EntityAddress) + L" \"" + EntityName + L"\" " +
				(EntityDomain == SocketAddressDomain::IPv4 ? L"IPv4" : L"IPv6");
		}
	}
}