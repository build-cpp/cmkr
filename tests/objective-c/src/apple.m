#import <Foundation/Foundation.h>

void platform_specific() {
  NSFileHandle *stdout = [NSFileHandle fileHandleWithStandardOutput];
  NSString *message = @"Hello from Objective-C!\n";
  [stdout writeData:[message dataUsingEncoding:NSUTF8StringEncoding] error:nil];
}
