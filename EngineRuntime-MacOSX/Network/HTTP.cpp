#include "HTTP.h"

#include "../Interfaces/SecureSocket.h"
#include "../Miscellaneous/DynamicString.h"

namespace Engine
{
	namespace Network
	{
		namespace HTTP
		{
			class HttpRequest : public Network::HttpRequest
			{
				friend class WriterStream;
				friend class ReaderStream;
				class WriterStream : public Streaming::Stream
				{
				public:
					SafePointer<Socket> target;
					uint32 max_length;
					uint32 current = 0;

					WriterStream(Socket * connection, uint32 limit_len) : max_length(limit_len) { target.SetRetain(connection); }
					virtual ~WriterStream(void) override {}
					virtual void Read(void * buffer, uint32 length) override { throw Exception(); }
					virtual void Write(const void * data, uint32 length) override
					{
						uint64 new_length = uint64(current) + uint64(length);
						if (new_length > uint64(max_length)) throw Exception();
						target->Write(data, length);
						current += length;
					}
					virtual int64 Seek(int64 position, Streaming::SeekOrigin origin) override { throw Exception(); }
					virtual uint64 Length(void) override { throw Exception(); }
					virtual void SetLength(uint64 length) override { throw Exception(); }
					virtual void Flush(void) override {}
				};
				class ReaderStream : public Streaming::Stream
				{
					SafePointer<Socket> target;
					uint32 max_length;
					uint32 current = 0;
				public:
					ReaderStream(Socket * connection, uint32 limit_len) : max_length(limit_len) { target.SetRetain(connection); }
					virtual ~ReaderStream(void) override { try { uint8 r; while (current < max_length) Read(&r, 1); } catch (...) { target->Shutdown(true, true); } }
					virtual void Read(void * buffer, uint32 length) override
					{
						uint32 avail = max_length - current;
						if (length > avail) {
							length = avail;
							target->Read(buffer, length);
							current = max_length;
							throw IO::FileReadEndOfFileException(length);
						} else {
							target->Read(buffer, length);
							current += length;
						}
					}
					virtual void Write(const void * data, uint32 length) override { throw Exception(); }
					virtual int64 Seek(int64 position, Streaming::SeekOrigin origin) override { throw Exception(); }
					virtual uint64 Length(void) override { return uint64(max_length); }
					virtual void SetLength(uint64 length) override { throw Exception(); }
					virtual void Flush(void) override {}
				};

				struct Header {
					string Name;
					string Content;
				};
				string _agent;
				string _host;
				string _uri;
				string _verb;
				SafePointer<Socket> _connection;
				Array<Header> _headers;
				uint32 _status = 0;

				SafePointer<WriterStream> writer;
				SafePointer<ReaderStream> reader;

				void write_newline(Array<char> & dest) { dest << '\r'; dest << '\n'; }
				void write_string(Array<char> & dest, const string & text)
				{
					int at = dest.Length();
					int len = text.GetEncodedLength(Encoding::ANSI);
					dest.SetLength(at + len);
					text.Encode(dest.GetBuffer() + at, Encoding::ANSI, false);
				}
				void send_http_header(void)
				{
					Array<char> encoded(0x100);
					write_string(encoded, _verb + L" " + _uri + L" HTTP/1.1");
					write_newline(encoded);
					for (int i = 0; i < _headers.Length(); i++) {
						write_string(encoded, _headers[i].Name + L": " + _headers[i].Content);
						write_newline(encoded);
					}
					write_newline(encoded);
					_connection->Write(encoded.GetBuffer(), encoded.Length());
				}
				string read_string(void)
				{
					DynamicString result;
					widechar chr = 0;
					while (true) {
						_connection->Read(&chr, 1);
						if (chr >= 32) {
							result += chr;
						} else if (chr == '\n') break;
					}
					return result;
				}
				void read_http_header(void)
				{
					_headers.Clear();
					bool first = true;
					while (true) {
						string resp = read_string();
						if (resp.Length()) {
							if (first) {
								first = false;
								int sp1 = 0, sp2 = 0;
								while (resp[sp1] && resp[sp1] != L' ') sp1++;
								sp2 = sp1 + 1;
								while (resp[sp2] && resp[sp2] != L' ') sp2++;
								_status = resp.Fragment(sp1 + 1, sp2 - sp1 - 1).ToUInt32();
							} else {
								int cp = 0;
								while (resp[cp] && resp[cp] != L':') cp++;
								int bp = cp + 1;
								while (resp[bp] && resp[bp] == L' ') bp++;
								int ep = resp.Length() - 1;
								while (ep && resp[ep] == L' ') ep--;
								SetHeader(resp.Fragment(0, cp), (ep >= bp) ? resp.Fragment(bp, ep - bp + 1) : L"");
							}
						} else break;
					}
					string len = GetHeader(L"Content-Length");
					reader = new ReaderStream(_connection, len.Length() ? len.ToUInt32() : 0);
				}
				void read_for_body(void)
				{
					while ((_status / 100) <= 1) read_http_header();
				}
			public:
				HttpRequest(const string & agent, const string & host, const string & uri, const widechar * verb, HttpVerb everb, Socket * connection) : _agent(agent), _host(host), _uri(uri), _verb(verb), _headers(0x10)
				{
					_connection.SetRetain(connection);
					_connection->SetReadTimeout(10000);
					SetHeader(L"Cache-Control", L"no-cache");
					SetHeader(L"Accept", L"*");
					SetHeader(L"Accept-Charset", L"utf-8");
					SetHeader(L"Host", _host);
					if (_agent.Length()) SetHeader(L"User-Agent", _agent);
				}
				virtual ~HttpRequest(void) override {}
				virtual void Send(void) override
				{
					if (_status) throw InvalidStateException();
					send_http_header();
					_headers.Clear();
					_status = 1;
					_connection->Flush();
				}
				virtual void Send(const void * data, uint32 size) override
				{
					if (_status) throw InvalidStateException();
					if (size) SetHeader(L"Content-Length", string(size));
					send_http_header();
					if (size) _connection->Write(data, size);
					_headers.Clear();
					_status = 1;
					_connection->Flush();
				}
				virtual Streaming::Stream * BeginSend(uint32 size) override
				{
					if (_status || writer) throw InvalidStateException();
					SetHeader(L"Content-Length", string(size));
					send_http_header();
					writer = new WriterStream(_connection, size);
					return writer;
				}
				virtual void EndSend(void) override
				{
					if (!writer) throw InvalidStateException();
					if (writer->current < writer->max_length) {
						uint8 zero = 0;
						while (writer->current < writer->max_length) writer->Write(&zero, 1);
					}
					writer.SetReference(0);
					_headers.Clear();
					_status = 1;
					_connection->Flush();
				}
				virtual Streaming::Stream * GetResponceStream(void) override
				{
					if (!_status) throw InvalidStateException();
					read_for_body();
					return reader;
				}
				virtual uint32 GetStatus(void) override
				{
					read_for_body();
					return _status;
				}
				virtual void SetHeader(const string & header, const string & value) override
				{
					for (int i = 0; i < _headers.Length(); i++) if (string::CompareIgnoreCase(_headers[i].Name, header) == 0)
					{
						_headers[i].Content = value;
						return;
					}
					_headers << Header{ header, value };
				}
				virtual string GetHeader(const string & header) override
				{
					if (_status == 1) read_for_body();
					for (int i = 0; i < _headers.Length(); i++) if (string::CompareIgnoreCase(_headers[i].Name, header) == 0) return _headers[i].Content;
					return L"";
				}
				virtual Array<string>* GetHeaders(void) override
				{
					if (_status == 1) read_for_body();
					SafePointer< Array<string> > result = new Array<string>(0x10);
					for (int i = 0; i < _headers.Length(); i++) result->Append(_headers[i].Name);
					result->Retain();
					return result;
				}
				virtual void SetReadTimeout(int time) override { _connection->SetReadTimeout(time); }
				virtual int GetReadTimeout(void) override { return _connection->GetReadTimeout(); }
			};
			class HttpConnection : public Network::HttpConnection
			{
				string _agent;
				string _host;
				SocketAddressDomain _domain;
				Address _address;
				uint32 _port;
				bool _secure;
			public:
				HttpConnection(const string & agent, const string & address, bool secure = false) : _agent(agent), _host(address), _secure(secure) { ResetConnection(); }
				virtual ~HttpConnection(void) override {}
				virtual HttpRequest * CreateRequest(const string & object, HttpVerb verb) override
				{
					SafePointer<Socket> connection = _secure ? CreateSecureSocket(_domain, _host) : CreateSocket(_domain, SocketProtocol::TCP);
					if (!connection) throw Exception();
					connection->Connect(_address, _port);
					const widechar * Verb = L"GET";
					if (verb == HttpVerb::Options) Verb = L"OPTIONS";
					else if (verb == HttpVerb::Get) Verb = L"GET";
					else if (verb == HttpVerb::Head) Verb = L"HEAD";
					else if (verb == HttpVerb::Post) Verb = L"POST";
					else if (verb == HttpVerb::Put) Verb = L"PUT";
					else if (verb == HttpVerb::Patch) Verb = L"PATCH";
					else if (verb == HttpVerb::Delete) Verb = L"DELETE";
					else if (verb == HttpVerb::Trace) Verb = L"TRACE";
					else if (verb == HttpVerb::Connect) Verb = L"CONNECT";
					try {
						return new HttpRequest(_agent, _host, object, Verb, verb, connection);
					} catch (...) { return 0; }
				}
				virtual void ResetConnection(void) override
				{
					string server;
					uint16 port;
					if (_host[0] == L'[') {
						int sep;
						if ((sep = _host.FindLast(L':')) != -1 && sep > _host.FindLast(L']')) {
							server = _host.Fragment(1, sep - 2);
							port = uint16(_host.Fragment(sep + 1, -1).ToUInt32());
						} else {
							server = _host.Fragment(1, _host.Length() - 2);
							port = _secure ? 443 : 80;
						}
					} else {
						int sep;
						if ((sep = _host.FindLast(L':')) != -1) {
							server = _host.Fragment(0, sep);
							port = uint16(_host.Fragment(sep + 1, -1).ToUInt32());
						} else {
							server = _host;
							port = _secure ? 443 : 80;
						}
					}
					SafePointer< Array<AddressEntity> > addr = GetAddressByHost(server, port, SocketAddressDomain::IPv6, SocketProtocol::TCP);
					if (!addr->Length()) throw InvalidArgumentException();
					_domain = addr->ElementAt(0).EntityDomain;
					_address = addr->ElementAt(0).EntityAddress;
					_port = port;
				}
			};
			class HttpSession : public Network::HttpSession
			{
				string _agent;
			public:
				HttpSession(const string & agent) : _agent(agent) {}
				virtual ~HttpSession(void) override {}
				virtual HttpConnection * Connect(const string & server_address) override
				{
					try {
						return new HttpConnection(_agent, server_address);
					} catch (...) { return 0; }
				}
				virtual HttpConnection * SecureConnect(const string & server_address) override
				{
					try {
						return new HttpConnection(_agent, server_address, true);
					} catch (...) { return 0; }
				}
			};
		}
		HttpSession * OpenHttpSession(const string & agent_name)
		{
			try {
				return new HTTP::HttpSession(agent_name);
			} catch (...) { return 0; }
		}
	}
}