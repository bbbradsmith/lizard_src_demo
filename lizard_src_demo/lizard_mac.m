#import <AppKit/AppKit.h>

void alert_objc(const char* text, const char* caption)
{
	NSString* ns_text = [[NSString alloc] initWithUTF8String:text];
	NSString* ns_caption = [[NSString alloc] initWithUTF8String:caption];
	
	NSAlert* alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle:@"OK"];
	[alert setMessageText:ns_caption];
	[alert setInformativeText:ns_text];
	[alert setAlertStyle:NSWarningAlertStyle];

	[alert runModal];

	[alert release];
	[ns_caption release];
	[ns_text release];
}

// end of file
