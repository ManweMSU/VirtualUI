#pragma once

#include "../EngineBase.h"
#include "../Streaming.h"
#include "../Miscellaneous/Dictionary.h"

namespace Engine
{
	namespace Network
	{
		enum class InternetVerb { Options, Get, Head, Post, Put, Patch, Delete, Trace, Connect };
		class InternetRequest : public Object
		{
		public:
			virtual void SetHeader(const string & header, const string & value) = 0;
			virtual void Send(void) = 0;
			virtual void Send(const void * data, uint32 size) = 0;
			virtual Streaming::Stream * BeginSend(void) = 0;
			virtual void EndSend(void) = 0;
			virtual Streaming::Stream * GetResponceStream(void) = 0;
			virtual uint32 GetStatus(void) = 0;
			virtual string GetHeader(const string & header) = 0;
		};
		class InternetConnection : public Object
		{
		public:
			virtual InternetRequest * CreateRequest(const string & object, InternetVerb verb = InternetVerb::Get) = 0;
		};
		class InternetSession : public Object
		{
		public:
			virtual InternetConnection * Connect(const string & server_address) = 0;
		};
		InternetSession * OpenInternetSession(const string & agent_name = L"Engine Runtime");
	}
}