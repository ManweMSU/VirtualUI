#include "InternetRequest.h"

#include <Windows.h>
#include <WinInet.h>

#pragma comment(lib, "WinInet.lib")

#undef min

namespace Engine
{
	namespace Network
	{
		namespace WinINet
		{
			class InternetRequest : public Network::InternetRequest
			{
				HINTERNET request;
				friend class WriterStream;
				friend class ReaderStream;
				class WriterStream : public Streaming::Stream
				{
					HINTERNET request;
				public:
					WriterStream(HINTERNET handle) : request(handle) {}
					virtual ~WriterStream(void) override { HttpEndRequestW(request, 0, 0, 0); }
					virtual void Read(void * buffer, uint32 length) override { throw Exception(); }
					virtual void Write(const void * data, uint32 length) override
					{
						DWORD Read;
						if (!InternetWriteFile(request, data, length, &Read)) throw IO::FileAccessException();
						if (Read != length) throw IO::FileAccessException();
					}
					virtual int64 Seek(int64 position, Streaming::SeekOrigin origin) override { throw Exception(); }
					virtual uint64 Length(void) override { throw Exception(); }
					virtual void SetLength(uint64 length) override { throw Exception(); }
					virtual void Flush(void) override {}
				};
				class ReaderStream : public Streaming::Stream
				{
					HINTERNET request;
				public:
					ReaderStream(HINTERNET handle) : request(handle) {}
					virtual ~ReaderStream(void) override {}
					virtual void Read(void * buffer, uint32 length) override
					{
						char * to = reinterpret_cast<char *>(buffer);
						DWORD available, left = length;
						while (left) {
							if (!InternetQueryDataAvailable(request, &available, 0, 0)) throw IO::FileAccessException();
							if (available == 0) throw IO::FileReadEndOfFileException(length - left);
							DWORD read = min(available, left), was_read;
							if (!InternetReadFile(request, to, read, &was_read)) throw IO::FileAccessException();
							to += was_read;
							left -= was_read;
						}
					}
					virtual void Write(const void * data, uint32 length) override { throw Exception(); }
					virtual int64 Seek(int64 position, Streaming::SeekOrigin origin) override { throw Exception(); }
					virtual uint64 Length(void) override
					{
						DWORD length, len = 4, num = 0;
						if (!HttpQueryInfoW(request, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &length, &len, &num)) throw IO::FileAccessException();
						return length;
					}
					virtual void SetLength(uint64 length) override { throw Exception(); }
					virtual void Flush(void) override {}
				};
				SafePointer<WriterStream> writer;
				SafePointer<ReaderStream> reader;
			public:
				InternetRequest(HINTERNET handle) : request(handle) {}
				virtual ~InternetRequest(void) override { InternetCloseHandle(request); }
				virtual void SetHeader(const string & header, const string & value) override
				{
					SafePointer< Array<uint8> > encoded = (header + L": " + value).EncodeSequence(Encoding::ANSI, false);
					encoded->Append(13);
					encoded->Append(10);
					encoded->Append(0);
					if (!HttpAddRequestHeadersA(request, reinterpret_cast<char *>(encoded->GetBuffer()), -1,
						HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE)) throw Exception();
				}
				virtual void Send(void) override
				{
					if (!HttpSendRequestW(request, 0, 0, 0, 0)) throw IO::FileAccessException();
					reader.SetReference(new ReaderStream(request));
				}
				virtual void Send(const void * data, uint32 size) override
				{
					if (!HttpSendRequestW(request, 0, 0, const_cast<void *>(data), size)) throw IO::FileAccessException();
					reader.SetReference(new ReaderStream(request));
				}
				virtual Streaming::Stream * BeginSend(void) override
				{
					if (!HttpSendRequestExW(request, 0, 0, 0, 0)) throw IO::FileAccessException();
					writer.SetReference(new WriterStream(request));
					return writer;
				}
				virtual void EndSend(void) override { writer.SetReference(0); reader.SetReference(new ReaderStream(request)); }
				virtual Streaming::Stream * GetResponceStream(void) override { return reader; }
				virtual uint32 GetStatus(void) override
				{
					DWORD status, len = 4, num = 0;
					if (!HttpQueryInfoW(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &len, &num)) throw Exception();
					return status;
				}
				virtual string GetHeader(const string & header) override
				{
					Array<char> buffer(0x100);
					DWORD size = 0;
					if (!HttpQueryInfoA(request, HTTP_QUERY_RAW_HEADERS, buffer.GetBuffer(), &size, 0)) {
						if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) throw Exception();
						buffer.SetLength(size);
						if (!HttpQueryInfoA(request, HTTP_QUERY_RAW_HEADERS, buffer.GetBuffer(), &size, 0)) {
							throw Exception();
						}
					}
					int cp = 0;
					while (buffer[cp]) {
						int ep = cp;
						while (buffer[ep]) ep++;
						ep++;
						string line = string(buffer.GetBuffer() + cp, -1, Encoding::ANSI);
						int colon = line.FindFirst(L':');
						if (colon && string::CompareIgnoreCase(line.Fragment(0, colon), header) == 0) {
							colon++;
							while (line[colon] == L' ') colon++;
							int end = line.Length() - 1;
							while (line[end] == L' ') end--;
							if (end < colon) return L""; else return line.Fragment(colon, end - colon + 1);
						}
						cp = ep;
					}
					return L"";
				}
			};
			class InternetConnection : public Network::InternetConnection
			{
				HINTERNET connection;
			public:
				InternetConnection(HINTERNET handle) : connection(handle) {}
				virtual ~InternetConnection(void) override { InternetCloseHandle(connection); }
				virtual InternetRequest * CreateRequest(const string & object, InternetVerb verb = InternetVerb::Get) override
				{
					const widechar * Verb = L"GET";
					if (verb == InternetVerb::Options) Verb = L"OPTIONS";
					else if (verb == InternetVerb::Get) Verb = L"GET";
					else if (verb == InternetVerb::Head) Verb = L"HEAD";
					else if (verb == InternetVerb::Post) Verb = L"POST";
					else if (verb == InternetVerb::Put) Verb = L"PUT";
					else if (verb == InternetVerb::Patch) Verb = L"PATCH";
					else if (verb == InternetVerb::Delete) Verb = L"DELETE";
					else if (verb == InternetVerb::Trace) Verb = L"TRACE";
					else if (verb == InternetVerb::Connect) Verb = L"CONNECT";
					LPCWSTR Types[] = { L"*", 0 };
					HINTERNET request = HttpOpenRequestW(connection, Verb, object, 0, 0, Types,
						INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, 0);
					if (request) return new WinINet::InternetRequest(request); else return 0;
				}
			};
			class InternetSession : public Network::InternetSession
			{
				HINTERNET session;
			public:
				InternetSession(HINTERNET handle) : session(handle) {}
				virtual ~InternetSession(void) override { InternetCloseHandle(session); }
				virtual InternetConnection * Connect(const string & server_address) override
				{
					HANDLE connection = InternetConnectW(session, server_address, INTERNET_DEFAULT_HTTP_PORT, 0, 0, INTERNET_SERVICE_HTTP, 0, 0);
					if (connection) return new WinINet::InternetConnection(connection); else return 0;
				}
			};
		}

		InternetSession * OpenInternetSession(const string & agent_name)
		{
			HINTERNET session = InternetOpenW(agent_name, INTERNET_OPEN_TYPE_DIRECT, 0, 0, 0);
			if (session) return new WinINet::InternetSession(session); else return 0;
		}
	}
}