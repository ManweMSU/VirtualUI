#include "../Interfaces/SecureSocket.h"

#include "../Interfaces/Threading.h"

#include <Network/Network.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>
#include <netdb.h>

using namespace Engine::Streaming;

namespace Engine
{
	namespace Network
	{
		class SecureSocket : public Socket
		{
			SocketAddressDomain addr_domain; 
			string host;
			nw_connection_t connection;
			volatile int send_error_state;
			SafePointer<Semaphore> semaphore;
			int timeout;
		public:
			SecureSocket(SocketAddressDomain domain, const string & verify_host) : addr_domain(domain), host(verify_host), connection(0), send_error_state(0), timeout(-1) { semaphore = CreateSemaphore(0); }
			virtual ~SecureSocket(void) override { if (connection) nw_release(connection); }

			virtual void Read(void * buffer, uint32 length) override
			{
				if (send_error_state) throw IO::FileAccessException();
				uint8 * bytes = reinterpret_cast<uint8 *>(buffer);
				volatile int lock_state = 0;
				volatile int * lock_state_ptr = &lock_state;
				volatile int data_read = 0;
				volatile int * data_read_ptr = &data_read;
				nw_connection_receive(connection, length, length, ^(dispatch_data_t content, nw_content_context_t context, bool is_complete, nw_error_t error) {
					if (content) {
						auto real_size = dispatch_data_get_size(content);
						dispatch_data_apply(content, ^ bool (dispatch_data_t region, size_t offset, const void *buffer, size_t size) {
							MemoryCopy(bytes + offset, buffer, size);
							return true;
						});
						if (real_size < length) {
							*lock_state_ptr = 2;
							*data_read_ptr = real_size;
						} else {
							*lock_state_ptr = 1;
							*data_read_ptr = real_size;
						}
					} else if (is_complete) {
						*lock_state_ptr = 2;
						*data_read_ptr = 0;
					} else {
						*lock_state_ptr = 3;
					}
					semaphore->Open();
				});
				semaphore->Wait();
				if (lock_state == 2) throw IO::FileReadEndOfFileException(data_read);
				else if (lock_state == 3) throw IO::FileAccessException();
			}
			virtual void Write(const void * data, uint32 length) override
			{
				if (send_error_state) throw IO::FileAccessException();
				volatile int * error_state = &send_error_state;
				Retain();
				dispatch_data_t dispatch = dispatch_data_create(data, length, dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
				nw_connection_send(connection, dispatch, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, false, ^(nw_error_t error) {
					if (error) {
						*error_state = 1;
						Release();
					}
				});
				dispatch_release(dispatch);
			}
			virtual int64 Seek(int64 position, SeekOrigin origin) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual uint64 Length(void) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void SetLength(uint64 length) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Flush(void) override { if (send_error_state) throw IO::FileAccessException(); }
			virtual void Connect(const Address & address, uint16 port) override
			{
				nw_endpoint_t endpoint;
				if (addr_domain == SocketAddressDomain::IPv6) {
					sockaddr_in6 addr;
					addr.sin6_len = sizeof(addr);
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
					endpoint = nw_endpoint_create_address(reinterpret_cast<sockaddr *>(&addr));
				} else if (addr_domain == SocketAddressDomain::IPv4) {
					if (!address.IsIPv4()) throw InvalidArgumentException();
					sockaddr_in addr;
					addr.sin_len = sizeof(addr);
					addr.sin_family = AF_INET;
					addr.sin_port = InverseEndianess(port);
					addr.sin_addr.s_addr = InverseEndianess(address.IPv4);
					ZeroMemory(&addr.sin_zero, sizeof(addr.sin_zero));
					endpoint = nw_endpoint_create_address(reinterpret_cast<sockaddr *>(&addr));
				} else throw InvalidStateException();
				Array<char> host_name(0x100);
				host_name.SetLength(host.GetEncodedLength(Encoding::UTF8) + 1);
				host.Encode(host_name.GetBuffer(), Encoding::UTF8, true);
				nw_parameters_t params = nw_parameters_create_secure_tcp(^(nw_protocol_options_t options) {
					sec_protocol_options_set_tls_server_name((sec_protocol_options_t) options, host_name.GetBuffer());
				}, NW_PARAMETERS_DEFAULT_CONFIGURATION);
				connection = nw_connection_create(endpoint, params);
				volatile int lock_state = 0;
				volatile int * lock_state_ptr = &lock_state;
				nw_connection_set_queue(connection, dispatch_get_global_queue(QOS_CLASS_UTILITY, 0));
				nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t error) {
					if (state == nw_connection_state_ready) *lock_state_ptr = 1;
					else if (state == nw_connection_state_failed || state == nw_connection_state_cancelled || state == nw_connection_state_waiting || state == nw_connection_state_invalid) {
						if (nw_error_get_error_domain(error) == nw_error_domain_tls) *lock_state_ptr = 3;
						else *lock_state_ptr = 2;
					}
					if (*lock_state_ptr) semaphore->Open();
				});
				nw_release(endpoint);
				nw_release(params);
				nw_connection_start(connection);
				semaphore->Wait();
				if (lock_state == 2) throw IO::FileAccessException();
				else if (lock_state == 3) throw SecurityAuthenticationFailedException();
				if (lock_state != 1) {
					nw_release(connection);
					connection = 0;
				}
				nw_connection_set_state_changed_handler(connection, 0);
			}
			virtual void Bind(const Address & address, uint16 port) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Bind(uint16 port) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Listen(void) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual Socket * Accept() override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual Socket * Accept(Address & address, uint16 & port) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Shutdown(bool close_read, bool close_write) override
			{
				if (send_error_state) throw IO::FileAccessException();
				if (close_write) {
					nw_connection_send(connection, 0, NW_CONNECTION_FINAL_MESSAGE_CONTEXT, true, ^(nw_error_t error) {
						semaphore->Open();
					});
					semaphore->Wait();
				}
				if (close_write && close_read) nw_connection_cancel(connection);
			}
			virtual bool Wait(int time) override { return false; }
			virtual void SetReadTimeout(int time) override { timeout = time; }
			virtual int GetReadTimeout(void) override { return timeout; }
		};

		SecurityAuthenticationFailedException::SecurityAuthenticationFailedException(void) {}
		string SecurityAuthenticationFailedException::ToString(void) const { return L"SecurityAuthenticationFailedException"; }
		Socket * CreateSecureSocket(SocketAddressDomain domain, const string & verify_host) { return new SecureSocket(domain, verify_host); }
	}
}