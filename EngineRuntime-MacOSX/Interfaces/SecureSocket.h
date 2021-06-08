#pragma once

#include "Socket.h"

namespace Engine
{
	namespace Network
	{
		class SecurityAuthenticationFailedException : public Exception { public: SecurityAuthenticationFailedException(void); string ToString(void) const override; };

		Socket * CreateSecureSocket(SocketAddressDomain domain, const string & verify_host);
	}
}