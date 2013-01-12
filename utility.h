/*
 * utility.h
 *
 *  Created on: 2013-01-11
 *      Author: rlum
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include <stdio.h>

#ifdef DEBUG
// utility routine to be able to leave trails of what functions are being called#ifdef DEBUG
#define TRACE printf("Entered:%s:  AT %s:%d\n",__FUNCTION__, __FILE__, __LINE__);
#define DBGMSG( x, ...) printf(x, __VA_ARGS__);
#else

#define TRACE
#define DBGMSG(x, ...)
#endif




#endif /* UTILITY_H_ */
