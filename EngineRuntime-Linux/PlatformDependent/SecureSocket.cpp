#include "../Interfaces/SecureSocket.h"
#include "SocketAPI.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace Engine
{
	namespace Network
	{
		uint OpenSSL_InitState = 0;
		void InitOpenSSL(void)
		{
			if (!OpenSSL_InitState) 
			{
				if (InterlockedIncrement(OpenSSL_InitState) == 1) SSL_library_init();
				else InterlockedDecrement(OpenSSL_InitState);
			}
		}

		class SystemSecureSocket : public Socket
		{
			int _wrapped_handle, _timeout;
			bool _shutdown;
			SafePointer<Socket> _wrapped_socket;
			SSL * _ssl;
		public:
			SystemSecureSocket(SocketAddressDomain domain, const string & verify_host) : _shutdown(true), _timeout(-1)
			{
				InitOpenSSL();
				SSL_CTX * context;
				Array<char> host(1);
				host.SetLength(verify_host.GetEncodedLength(Encoding::UTF8) + 1);
				verify_host.Encode(host.GetBuffer(), Encoding::UTF8, true);
				_wrapped_socket = CreateSocket(domain, SocketProtocol::TCP);
				if (!_wrapped_socket) throw Exception();
				_wrapped_handle = GetSocketHandle(_wrapped_socket);
				context = SSL_CTX_new(TLS_client_method());
				if (!context) throw OutOfMemoryException();
				SSL_CTX_set_verify(context, SSL_VERIFY_PEER, 0);
				SSL_CTX_set_default_verify_paths(context);
				X509_VERIFY_PARAM * param = SSL_CTX_get0_param(context);
				X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
				if (!X509_VERIFY_PARAM_set1_host(param, host, host.Length() - 1)) {
					SSL_CTX_free(context);
					throw OutOfMemoryException();
				}
				_ssl = SSL_new(context);
				SSL_CTX_free(context);
				if (!_ssl) throw OutOfMemoryException();
				SSL_set_fd(_ssl, _wrapped_handle);
			}
			virtual ~SystemSecureSocket(void) override { SSL_free(_ssl); }
			virtual void Read(void * buffer, uint32 length) override
			{
				if (!length) return;
				if (_shutdown) throw InvalidStateException();
				if (SSL_get_shutdown(_ssl) & SSL_RECEIVED_SHUTDOWN) throw IO::FileReadEndOfFileException(0);
				char * dest = reinterpret_cast<char *>(buffer);
				uint32 read_left = length;
				uint32 bytes_read = 0;
				while (read_left) {
					auto error = SSL_read(_ssl, dest, read_left);
					if (error > 0) {
						dest += error;
						bytes_read += error;
						read_left -= error;
					} else {
						auto error_ex = SSL_get_error(_ssl, error);
						if (error_ex == SSL_ERROR_ZERO_RETURN) throw IO::FileReadEndOfFileException(bytes_read);
						else if (error_ex == SSL_ERROR_WANT_READ) {
							if (_wrapped_socket->Wait(_timeout)) {
								continue;
							} else {
								throw IO::FileAccessException(IO::Error::ReadFailure);
							}
						} else if (error_ex == SSL_ERROR_WANT_WRITE) continue;
						else throw IO::FileAccessException(IO::Error::ReadFailure);
					}
				}
			}
			virtual void Write(const void * data, uint32 length) override
			{
				if (!length) return;
				if (_shutdown) throw InvalidStateException();
				while (true) {
					auto error = SSL_write(_ssl, data, length);
					if (error > 0) {
						if (length != error) throw IO::FileAccessException(IO::Error::WriteFailure);
						else return;
					} else {
						auto error_ex = SSL_get_error(_ssl, error);
						if (error_ex == SSL_ERROR_WANT_READ) {
							_wrapped_socket->Wait(-1);
							continue;
						} else if (error_ex == SSL_ERROR_WANT_WRITE) continue;
						else throw IO::FileAccessException(IO::Error::WriteFailure);
					}
				}
			}
			virtual int64 Seek(int64 position, Streaming::SeekOrigin origin) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual uint64 Length(void) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void SetLength(uint64 length) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Flush(void) override { _wrapped_socket->Flush(); }
			virtual void Connect(const Address & address, uint16 port) override
			{
				_wrapped_socket->Connect(address, port);
				while (true) {
					ERR_clear_error();
					int error = SSL_connect(_ssl);
					if (error == 0) throw SecurityAuthenticationFailedException();
					else if (error < 0) {
						int error_ex = SSL_get_error(_ssl, error);
						if (error_ex == SSL_ERROR_WANT_READ) {
							_wrapped_socket->Wait(-1);
							continue;
						} else if (error_ex == SSL_ERROR_WANT_WRITE) {
							continue;
						} else if (error_ex == SSL_ERROR_SSL) {
							throw SecurityAuthenticationFailedException();
						} else throw IO::FileAccessException(IO::Error::Unknown);
					} else if (error > 1) throw Exception();
					else if (error == 1) break;
				}
				_shutdown = false;
			}
			virtual void Bind(const Address & address, uint16 port) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Bind(uint16 port) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Listen(void) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual Socket * Accept() override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual Socket * Accept(Address & address, uint16 & port) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Shutdown(bool close_read, bool close_write) override
			{
				if (!_shutdown && (close_read || close_write)) {
					while (true) {
						auto error = SSL_shutdown(_ssl);
						if (error == 1) break;
						else if (error == 0) continue;
						else if (error < 0) {
							auto error_ex = SSL_get_error(_ssl, error);
							if (error_ex == SSL_ERROR_WANT_READ) {
								_wrapped_socket->Wait(-1);
								continue;
							} else if (error_ex == SSL_ERROR_WANT_WRITE) {
								continue;
							} else throw IO::FileAccessException(IO::Error::Unknown);
						} else throw IO::FileAccessException(IO::Error::Unknown);
					}
					_wrapped_socket->Shutdown(close_read, close_write);
					_shutdown = true;
				}
			}
			virtual bool Wait(int time) override
			{
				if (_shutdown) throw InvalidStateException();
				if (SSL_pending(_ssl) > 0) return true;
				return _wrapped_socket->Wait(time);
			}
			virtual void SetReadTimeout(int time) override { _timeout = time; }
			virtual int GetReadTimeout(void) override { return _timeout; }
		};

		SecurityAuthenticationFailedException::SecurityAuthenticationFailedException(void) {}
		string SecurityAuthenticationFailedException::ToString(void) const { return L"SecurityAuthenticationFailedException"; }
		Socket * CreateSecureSocket(SocketAddressDomain domain, const string & verify_host) { return new SystemSecureSocket(domain, verify_host); }
	}
}