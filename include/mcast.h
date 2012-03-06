//
// C++ Interface: %{MODULE}
//
// Description: 
//
//
// Author: %{AUTHOR} <%{EMAIL}>, (C) %{YEAR}
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <netinet/in.h>  /* for struct sockaddr_in */

int isMulticastAddr(struct sockaddr_storage *addr);
int joinGroup(int sockfd, int loopBack, unsigned int mcastTTL,struct sockaddr_storage *addr,int interface);
