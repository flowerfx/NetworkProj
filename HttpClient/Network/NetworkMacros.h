
#ifndef __NETWORK_MARCO_H__
#define __NETWORK_MARCO_H__

#include "winsock2.h"
#include "winsock.h"
#include "Ws2tcpip.h"

#include "Core/CoreMacros.h"
namespace App
{
	namespace Network
	{
		struct IPv4
		{
			u8 net0;
			u8 net1;
			u8 subnet;
			u8 host;

			IPv4()
			{
				net0 = net1 = subnet = host = 0;
			}

			IPv4(u8 v1, u8 v2, u8 v3, u8 v4) :
				net0(v1),
				net1(v2),
				subnet(v3),
				host(v4)
			{
			}

			static IPv4 LocalHostIP()
			{
				return IPv4(127, 0, 0, 1);
			}

			IPv4(u8 value)
			{
				net0 = net1 = subnet = host = value;
			}

			void set(const IPv4 & ip)
			{
				net0 = ip.net0;
				net1 = ip.net1;
				subnet = ip.subnet;
				host = ip.host;
			}

			IPv4(const IPv4 & ip)
			{
				set(ip);
			}

			void operator=(const IPv4& ip) {
				set(ip);
			}

			bool operator==(const IPv4& ip) {
				bool res = true;
				if (net0 != ip.net0)
					return false;
				if (net1 != ip.net1)
					return false;
				if (subnet != ip.subnet)
					return false;
				if (host != ip.host)
					return false;
				return res;
			}

			virtual ~IPv4()
			{

			}


		};

		struct in6_addr {
			s8  s6_ad[16];  /* IPv6 address */
		};

		struct IPv6
		{
			u8           sin6_len;      /* length of this structure */
			u8           sin6_family;   /* AF_INET6                 */
			u16       sin6_port;     /* Transport layer port #   */
			u32       sin6_flowinfo; /* IPv6 flow information    */
			struct in6_addr  sin6_addr;     /* IPv6 address             */
		};
	}
}

#endif // __NETWORK_MARCO_H__