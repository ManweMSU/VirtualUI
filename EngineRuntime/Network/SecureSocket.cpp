#include "SecureSocket.h"

using namespace Engine::Streaming;

#define SECURITY_WIN32

#include <windows.h>
#include <sspi.h>
#include <schannel.h>

#pragma comment(lib, "Secur32.lib")

namespace Engine
{
	namespace Network
	{
		class SecureSocket : public Socket
		{
			SocketAddressDomain addr_domain;
			SafePointer<Socket> sock;
			Array<widechar> host;
			CredHandle cred;
			CtxtHandle ctxt;
			ULONG context_attrs;
			SecPkgContext_StreamSizes data_sizes;
			int sock_state;

			Array<uint8> write_buffer;
			Array<uint8> read_buffer;
			int read_pos;
			
			Array<uint8> * ReceivePackage(void) noexcept
			{
				try {
					SafePointer< Array<uint8> > data = new Array<uint8>(0x100);
					try {
						data->SetLength(5);
						sock->Read(data->GetBuffer(), 5);
						int length = (int(data->ElementAt(3)) << 8) | int(data->ElementAt(4));
						data->SetLength(5 + length);
						sock->Read(data->GetBuffer() + 5, length);
					} catch (IO::FileReadEndOfFileException & e) { data->Clear(); }
					data->Retain();
					return data;
				} catch (...) { return 0; }
			}
			bool SendPackage(const void * data, int length) noexcept { try { if (data && length) sock->Write(data, length); return true; } catch (...) { return false; } }
			bool ReceiveReadBufferEncrypted(void)
			{
				try {
					SafePointer< Array<uint8> > package = ReceivePackage();
					if (!package) return false;
					if (!package->Length()) {
						read_buffer.Clear();
						read_pos = 0;
						return true;
					}
					SecBufferDesc recv_desc;
					SecBuffer recv[4];
					recv_desc.ulVersion = SECBUFFER_VERSION;
					recv_desc.cBuffers = 4;
					recv_desc.pBuffers = recv;
					recv[0].BufferType = SECBUFFER_DATA;
					recv[0].cbBuffer = package->Length();
					recv[0].pvBuffer = package->GetBuffer();
					ZeroMemory(&recv[1], sizeof(recv[1]));
					ZeroMemory(&recv[2], sizeof(recv[2]));
					ZeroMemory(&recv[3], sizeof(recv[3]));
					
					auto status = DecryptMessage(&ctxt, &recv_desc, 0, 0);
					if (status != SEC_E_OK) {
						if (status == SEC_I_CONTEXT_EXPIRED) {
							read_buffer.Clear();
							read_pos = 0;
							return true;
						} else return false;
					} else {
						for (int i = 0; i < recv_desc.cBuffers; i++) if (recv_desc.pBuffers[i].BufferType == SECBUFFER_DATA) {
							read_buffer.SetLength(recv_desc.pBuffers[i].cbBuffer);
							MemoryCopy(read_buffer.GetBuffer(), recv_desc.pBuffers[i].pvBuffer, recv_desc.pBuffers[i].cbBuffer);
							read_pos = 0;
							return true;
						}
						return false;
					}
				} catch (...) { return false; }
			}
			bool SubmitWriteBufferEncrypted(void)
			{
				try {
					Array<uint8> enc_data(0x100);
					enc_data.SetLength(data_sizes.cbHeader + data_sizes.cbTrailer + write_buffer.Length());
					MemoryCopy(enc_data.GetBuffer() + data_sizes.cbHeader, write_buffer.GetBuffer(), write_buffer.Length());
					SecBufferDesc send_desc;
					SecBuffer send[4];
					send_desc.ulVersion = SECBUFFER_VERSION;
					send_desc.cBuffers = 4;
					send_desc.pBuffers = send;
					send[0].BufferType = SECBUFFER_STREAM_HEADER;
					send[0].cbBuffer = data_sizes.cbHeader;
					send[0].pvBuffer = enc_data.GetBuffer();
					send[1].BufferType = SECBUFFER_DATA;
					send[1].cbBuffer = write_buffer.Length();
					send[1].pvBuffer = enc_data.GetBuffer() + data_sizes.cbHeader;
					send[2].BufferType = SECBUFFER_STREAM_TRAILER;
					send[2].cbBuffer = data_sizes.cbTrailer;
					send[2].pvBuffer = enc_data.GetBuffer() + data_sizes.cbHeader + write_buffer.Length();
					ZeroMemory(&send[3], sizeof(send[3]));

					auto status = EncryptMessage(&ctxt, 0, &send_desc, 0);
					if (status == SEC_E_OK) {
						if (SendPackage(enc_data.GetBuffer(), enc_data.Length())) {
							write_buffer.Clear();
							return true;
						} else return false;
					} else return false;
				} catch (...) { return false; }
			}
			bool SubmitAlert(uint8 level, uint8 code)
			{
				try {
					Array<uint8> enc_data(0x100);
					enc_data.SetLength(data_sizes.cbHeader + data_sizes.cbTrailer + 2);
					enc_data[data_sizes.cbHeader] = level;
					enc_data[data_sizes.cbHeader + 1] = code;
					SecBufferDesc send_desc;
					SecBuffer send[4];
					send_desc.ulVersion = SECBUFFER_VERSION;
					send_desc.cBuffers = 4;
					send_desc.pBuffers = send;
					send[0].BufferType = SECBUFFER_STREAM_HEADER;
					send[0].cbBuffer = data_sizes.cbHeader;
					send[0].pvBuffer = enc_data.GetBuffer();
					send[1].BufferType = SECBUFFER_DATA;
					send[1].cbBuffer = 2;
					send[1].pvBuffer = enc_data.GetBuffer() + data_sizes.cbHeader;
					send[2].BufferType = SECBUFFER_STREAM_TRAILER;
					send[2].cbBuffer = data_sizes.cbTrailer;
					send[2].pvBuffer = enc_data.GetBuffer() + data_sizes.cbHeader + 2;
					ZeroMemory(&send[3], sizeof(send[3]));

					auto status = EncryptMessage(&ctxt, SECQOP_WRAP_OOB_DATA, &send_desc, 0);
					if (status == SEC_E_OK) return SendPackage(enc_data.GetBuffer(), enc_data.Length());
					else return false;
				} catch (...) { return false; }
			}
		public:
			SecureSocket(SocketAddressDomain domain, const string & verify_host) : host(0x10), write_buffer(0x10000), read_buffer(0x10000)
			{
				sock_state = 0;
				context_attrs = 0;
				read_pos = -1;
				addr_domain = domain;
				sock = CreateSocket(domain, SocketProtocol::TCP);
				host.SetLength(verify_host.GetEncodedLength(Encoding::UTF16) + 1);
				verify_host.Encode(host.GetBuffer(), Encoding::UTF16, true);
				auto status = AcquireCredentialsHandle(0, UNISP_NAME, SECPKG_CRED_OUTBOUND, 0, 0, 0, 0, &cred, 0);
				if (status != SEC_E_OK) throw Exception();
				sock_state |= 1;
			}
			virtual ~SecureSocket(void) override
			{
				sock.SetReference(0);
				if (sock_state & 2) DeleteSecurityContext(&ctxt);
				if (sock_state & 1) FreeCredentialsHandle(&cred);
			}

			virtual void Read(void * buffer, uint32 length) override
			{
				if (!(sock_state & 2)) throw InvalidStateException();
				uint8 * bytes = reinterpret_cast<uint8 *>(buffer);
				int data_read = 0;
				while (length) {
					if (read_pos < 0) if (!ReceiveReadBufferEncrypted()) throw IO::FileAccessException();
					if (read_pos == read_buffer.Length()) {
						if (!read_buffer.Length()) throw IO::FileReadEndOfFileException(data_read);
						if (!ReceiveReadBufferEncrypted()) throw IO::FileAccessException();
					}
					int read_now = min(length, read_buffer.Length() - read_pos);
					if (!read_now) throw IO::FileReadEndOfFileException(data_read);
					MemoryCopy(bytes + data_read, read_buffer.GetBuffer() + read_pos, read_now);
					read_pos += read_now;
					data_read += read_now;
					length -= read_now;
				}
			}
			virtual void Write(const void * data, uint32 length) override
			{
				if (!(sock_state & 2)) throw InvalidStateException();
				const uint8 * bytes = reinterpret_cast<const uint8 *>(data);
				while (length) {
					int move_size = min(length, data_sizes.cbMaximumMessage);
					int move_at = write_buffer.Length();
					write_buffer.SetLength(move_at + move_size);
					MemoryCopy(write_buffer.GetBuffer() + move_at, bytes, move_size);
					if (write_buffer.Length() >= data_sizes.cbMaximumMessage) {
						if (!SubmitWriteBufferEncrypted()) throw IO::FileAccessException();
					}
					bytes += move_size;
					length -= move_size;
				}
			}
			virtual int64 Seek(int64 position, SeekOrigin origin) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual uint64 Length(void) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void SetLength(uint64 length) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Flush(void) override
			{
				if (!(sock_state & 2)) throw InvalidStateException();
				if (write_buffer.Length()) if (!SubmitWriteBufferEncrypted()) throw IO::FileAccessException();
				sock->Flush();
			}
			virtual void Connect(const Address & address, uint16 port) override
			{
				if (sock_state & 2) throw InvalidStateException();
				sock->Connect(address, port);

				SecBufferDesc output_desc;
				SecBuffer output;
				ULONG context_attrs;
				output_desc.ulVersion = SECBUFFER_VERSION;
				output_desc.cBuffers = 1;
				output_desc.pBuffers = &output;
				ZeroMemory(&output, sizeof(output));

				auto status = InitializeSecurityContext(&cred, 0, host.GetBuffer(), ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY, 0, 0, 0, 0, &ctxt, &output_desc, &context_attrs, 0);
				if (status != SEC_I_CONTINUE_NEEDED && status != SEC_E_OK) throw Exception();
				if (!SendPackage(output.pvBuffer, output.cbBuffer)) {
					FreeContextBuffer(output.pvBuffer);
					DeleteSecurityContext(&ctxt);
					sock = CreateSocket(addr_domain, SocketProtocol::TCP);
					throw IO::FileAccessException();
				}
				FreeContextBuffer(output.pvBuffer);
				while (status == SEC_I_CONTINUE_NEEDED) {
					SafePointer< Array<uint8> > package = ReceivePackage();
					if (!package) {
						DeleteSecurityContext(&ctxt);
						sock = CreateSocket(addr_domain, SocketProtocol::TCP);
						throw IO::FileAccessException();
					}
					SecBufferDesc input_desc;
					SecBuffer input[2];
					input_desc.ulVersion = SECBUFFER_VERSION;
					input_desc.cBuffers = 2;
					input_desc.pBuffers = input;
					input[0].BufferType = SECBUFFER_TOKEN;
					input[0].cbBuffer = package->Length();
					input[0].pvBuffer = package->GetBuffer();
					ZeroMemory(&input[1], sizeof(input[1]));
					ZeroMemory(&output, sizeof(output));

					status = InitializeSecurityContext(&cred, &ctxt, host.GetBuffer(), ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY, 0, 0, &input_desc, 0, 0, &output_desc, &context_attrs, 0);
					if (status == SEC_I_CONTINUE_NEEDED || status == SEC_E_OK) {
						if (!SendPackage(output.pvBuffer, output.cbBuffer)) {
							FreeContextBuffer(output.pvBuffer);
							DeleteSecurityContext(&ctxt);
							sock = CreateSocket(addr_domain, SocketProtocol::TCP);
							throw IO::FileAccessException();
						}
						FreeContextBuffer(output.pvBuffer);
					}
				}
				if (status != SEC_E_OK || QueryContextAttributes(&ctxt, SECPKG_ATTR_STREAM_SIZES, &data_sizes) != SEC_E_OK) {
					DeleteSecurityContext(&ctxt);
					sock = CreateSocket(addr_domain, SocketProtocol::TCP);
					if (status == SEC_E_NO_AUTHENTICATING_AUTHORITY || status == SEC_E_TARGET_UNKNOWN || status == SEC_E_WRONG_PRINCIPAL) throw SecurityAuthenticationFailedException();
					else throw Exception();
				}
				sock_state |= 2;
			}
			virtual void Bind(const Address & address, uint16 port) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Bind(uint16 port) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Listen(void) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual Socket * Accept() override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual Socket * Accept(Address & address, uint16 & port) override { throw IO::FileAccessException(IO::Error::NotImplemented); }
			virtual void Shutdown(bool close_read, bool close_write) override
			{
				if (!(sock_state & 2)) throw InvalidStateException();
				if (close_write) {
					if (write_buffer.Length() && !SubmitWriteBufferEncrypted()) throw IO::FileAccessException();
					if (!SubmitAlert(1, 0)) throw IO::FileAccessException(IO::Error::NotImplemented);
					sock->Flush();
				}
				if (close_read) sock->Shutdown(true, false);
			}
			virtual bool Wait(int time) override
			{
				if (!(sock_state & 2)) throw InvalidStateException();
				if (read_pos < 0) return sock->Wait(time);
				if (read_pos == read_buffer.Length()) {
					if (read_buffer.Length()) return sock->Wait(time);
					else return true;
				}
				return true;
			}
			virtual void SetReadTimeout(int time) override { sock->SetReadTimeout(time); }
			virtual int GetReadTimeout(void) override { return sock->GetReadTimeout(); }
		};

		SecurityAuthenticationFailedException::SecurityAuthenticationFailedException(void) {}
		string SecurityAuthenticationFailedException::ToString(void) const { return L"SecurityAuthenticationFailedException"; }
		Socket * CreateSecureSocket(SocketAddressDomain domain, const string & verify_host) { return new SecureSocket(domain, verify_host); }
	}
}