/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-19.
********************************************/
#include "connector.h"


/********************************************
 * static
********************************************/
SortedTimerList* ConnectHandler::timer_lst;
HttpHandler* ConnectHandler::users;
int ConnectHandler::listenfd;