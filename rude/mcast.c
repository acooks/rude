/*
*  C Implementation: %{$MODULE}
*
* Description:
*
*
*
* Copyright: See COPYING file that comes with this distribution
*
*/
#include <mcast.h>
#include <rude.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <net/if.h>
#include <sys/ioctl.h>

int
isMulticastAddr(struct sockaddr_storage *addr)
{
	int retVal;

	retVal=-1;

	switch (addr->ss_family) {
		case AF_INET: {
			struct sockaddr_in *addr4=(struct sockaddr_in *)addr;
			retVal = IN_MULTICAST(ntohl(addr4->sin_addr.s_addr));
		} break;

		case AF_INET6: {
			struct sockaddr_in6 *addr6=(struct sockaddr_in6 *)addr;
			retVal = IN6_IS_ADDR_MULTICAST(&addr6->sin6_addr);
		} break;

		default:
		;
	}

	return retVal;
}




int joinGroup(int sockfd, int loopBack, unsigned int mcastTTL,struct sockaddr_storage *addr,int interface)
{
	int r1, r2, r3, retval;

	retval=-1;
	switch (addr->ss_family) {
		case AF_INET: {
			struct ip_mreq      mreq;
			//zjistit adresu interfacu(v parametru je jeho index),INADDR_ANY je pro defaultni
			mreq.imr_multiaddr.s_addr=
				((struct sockaddr_in *)addr)->sin_addr.s_addr;
			if(interface < 0){
				mreq.imr_interface.s_addr= INADDR_ANY;
				RUDEBUG7("joinGroup: using inaddr_any interface\n");
			}
			else{
				struct ifreq ifr;
				char name[IF_NAMESIZE];
				RUDEBUG7("joinGroup: using interface %i\n",interface);
				if(!if_indextoname(interface,name)){
					RUDEBUG1("joinGroup: interface doesnt exist.\n");
					return retval;
				}

				strcpy(ifr.ifr_name, name);
				if(ioctl(sockfd,SIOCGIFADDR,&ifr)==-1){
					RUDEBUG1("joinGroup() - ioctl failed for getting interface address: %s\n",strerror(errno));
					return retval;
				}
				struct sockaddr_in *si = (struct sockaddr_in*)&ifr.ifr_addr;
				RUDEBUG7("joinGroup(): interface address: %s\n",inet_ntoa(si->sin_addr));
				//mreq.imr_interface.s_addr = si->sin_addr.s_addr;
				memcpy(&mreq.imr_interface,&(si->sin_addr),sizeof(struct in_addr));

				//mreq.imr_interface = si->sin_addr;
			}

			r1= setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
			               &loopBack, sizeof(loopBack));
			if (r1<0)
				RUDEBUG1("joinGroup:: IP_MULTICAST_LOOP:: ");

			r2= setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
			               &mcastTTL, sizeof(mcastTTL));
			if (r2<0)
				RUDEBUG1("joinGroup:: IP_MULTICAST_TTL:: ");

			r3= setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			               (const void *)&mreq, sizeof(mreq));
			if (r3<0)
				RUDEBUG1("joinGroup:: IP_ADD_MEMBERSHIP:: ");

		} break;

		case AF_INET6: {
			struct ipv6_mreq    mreq6;

			memcpy(&mreq6.ipv6mr_multiaddr,
			       &(((struct sockaddr_in6 *)addr)->sin6_addr),
			       sizeof(struct in6_addr));

			if(interface < 0){
				mreq6.ipv6mr_interface= 0;
				RUDEBUG7("joinGroup: using default interface\n");
			}else{
				mreq6.ipv6mr_interface= interface;
			RUDEBUG7("joinGroup: using interface with index: %d\n",interface);
			}


			r1= setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
			               &loopBack, sizeof(loopBack));
			if (r1<0)
				RUDEBUG1("joinGroup(): IPV6_MULTICAST_LOOP error");

			r2= setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
			               &mcastTTL, sizeof(mcastTTL));
			if (r2<0)
				RUDEBUG1("joinGroup() IPV6_MULTICAST_HOPS error  ");

			r3=setsockopt(sockfd,IPPROTO_IPV6,IPV6_JOIN_GROUP,&mreq6,sizeof(mreq6));
			//r3=setsockopt(sockfd,IPPROTO_IPV6,IP_ADD_MEMBERSHIP,&mreq6,sizeof(mreq6));
			if (r3<0)
				RUDEBUG1("joinGroup() IPV6_JOIN_GROUP error ");

			} break;

			default:
				r1=r2=r3=-1;
	}

	if ((r1>=0) && (r2>=0) && (r3>=0))
		retval=0;

	return retval;
}
