#ifndef _MOSYNC_EXTENSION_PROTOCOL_H_
#define _MOSYNC_EXTENSION_PROTOCOL_H_

#include "MoSync.h"
#include <helpers/cpp_defs.h>

@protocol MoSyncExtensionProtocol
	-(id) initWithMoSync:(MoSync*)mosync;
	-(void) close;
	-(int) getHash;
	-(NSString*) getName;
	-(long long) invokeExtensionWithIndex:(int)id andArg1:(int)a andArg2:(int)b andArg3:(int)c;
@end

void initExtensions(MoSync* mosync);
MAExtensionModule extensionModuleLoad(const char *name, int hash);
MAExtensionFunction extensionFunctionLoad(MAHandle moduleHandle, int index);
int extensionFunctionInvoke(MAExtensionFunction funcId , int a, int b, int c);


#endif // _MOSYNC_EXTENSION_PROTOCOL_H_
