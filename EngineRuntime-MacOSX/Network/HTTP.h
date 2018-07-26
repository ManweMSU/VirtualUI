#pragma once

#include "../EngineBase.h"
#include "../Streaming.h"
#include "../Miscellaneous/Dictionary.h"

namespace Engine
{
	namespace Network
	{
		enum class HttpVerb { Options, Get, Head, Post, Put, Patch, Delete, Trace, Connect };
		class HttpRequest : public Object
		{
		public:
			virtual void Send(void) = 0;
			virtual void Send(const void * data, uint32 size) = 0;
			virtual Streaming::Stream * BeginSend(uint32 size) = 0;
			virtual void EndSend(void) = 0;
			virtual Streaming::Stream * GetResponceStream(void) = 0;
			virtual uint32 GetStatus(void) = 0;
			virtual void SetHeader(const string & header, const string & value) = 0;
			virtual string GetHeader(const string & header) = 0;
			virtual Array<string> * GetHeaders(void) = 0;
		};
		class HttpConnection : public Object
		{
		public:
			virtual HttpRequest * CreateRequest(const string & object, HttpVerb verb = HttpVerb::Get) = 0;
			virtual void ResetConnection(void) = 0;
		};
		class HttpSession : public Object
		{
		public:
			virtual HttpConnection * Connect(const string & server_address) = 0;
		};
		HttpSession * OpenHttpSession(const string & agent_name = L"");
	}
}