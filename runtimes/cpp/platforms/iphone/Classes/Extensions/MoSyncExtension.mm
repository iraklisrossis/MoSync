#include <objc/runtime.h>
#include "MoSyncExtension.h"
#include <helpers/cpp_defs.h>

static NSMutableArray* sExtensions;

void initExtensions(MoSync* mosync)
{
	sExtensions = [[NSMutableArray alloc] init ];
	int numClasses = objc_getClassList(NULL, 0);
	Class *classes = NULL;
	classes = (Class*)malloc(sizeof(Class) * numClasses);
	numClasses = objc_getClassList(classes, numClasses);
    NSLog(@"number of classes:%d", numClasses);
	for(int i = 0; i < numClasses; i++)
	{
		if(classes[i] && class_conformsToProtocol(classes[i], @protocol(MoSyncExtensionProtocol)))
		{
			id extension = [[classes[i] alloc] initWithMoSync:mosync];
			[sExtensions addObject:extension];
            NSLog(@"Found extension:%s", [extension getName]);
		}
	}
}

MAExtensionModule extensionModuleLoad(const char *name, int hash)
{
    NSString *extName = [[NSString alloc] initWithUTF8String:name];
    NSObject<MoSyncExtensionProtocol> *extensionModule;
    MAExtensionModule moduleHandle = -1;

    for(int i = 0; i < sExtensions.count; i++)
    {
        NSObject<MoSyncExtensionProtocol>* extension = [sExtensions objectAtIndex:i];
        if([[extension getName] isEqualToString:extName])
        {
            extensionModule = extension;
            moduleHandle = i;
            NSLog(@"Loading module:%s with handle:%d", [extensionModule getName], moduleHandle);
            break;
        }
    }
    [extName release];

    if(extensionModule == NULL)
    {
        return MA_EXTENSION_MODULE_UNAVAILABLE;
    }
    /*
    if(hash != [extensionModule getHash])
    {

    }
     */
    return moduleHandle;
}

MAExtensionFunction extensionFunctionLoad(MAHandle moduleHandle, int index)
{
    if (moduleHandle >= 0 && moduleHandle < sExtensions.count)
    {
        // maybe have method count as a generated part of the extension
        return (moduleHandle << 8) | (index & 0xff);
    }
    else
    {
        return MA_EXTENSION_FUNCTION_UNAVAILABLE;
    }
}

int extensionFunctionInvoke(MAExtensionFunction funcId , int a, int b, int c)
{
    int moduleHandle = funcId >> 8;
    if (moduleHandle >= 0 && moduleHandle < sExtensions.count)
    {
        NSObject<MoSyncExtensionProtocol> *module = [sExtensions objectAtIndex:moduleHandle];
        int function = funcId & 0xff;

        return [module invokeExtensionWithIndex:function andArg1:a andArg2:b andArg3:c];
    }

    return MA_EXTENSION_FUNCTION_UNAVAILABLE;
}

#if 0
@interface TestExtension : NSObject<MoSyncExtensionProtocol>
{
}

-(long long) maScreenGrabberGetScreen:(int)image;

@end

@implementation TestExtension
-(id) initWithMoSync:(MoSync*)mosync
{
	return self;
}

-(void) close
{
}

-(NSString*) getName
{
	return @"ScreenGrabber";
}

-(int) getHash
{
	return 0x1234124;
}

-(long long) invokeExtensionWithIndex:(int)_index andArg1:(int)a andArg2:(int)b andArg3:(int)c
{
	switch(_index)
    {
		case 1:
			return [self maScreenGrabberGetScreen:a];
	}
	return MA_EXTENSION_FUNCTION_UNAVAILABLE;
}

-(long long) maScreenGrabberGetScreen:(int)image
{
	return 0;
}
@end

#endif // if 0
