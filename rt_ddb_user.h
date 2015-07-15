#ifndef CDB_USER_H
#define CDB_USER_H

#include <rtr.h>

vector<remote_subscription *> subscribe_broadcast(string name, int timeout_sec);

#endif