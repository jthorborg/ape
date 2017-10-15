/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
	 This program is free software: you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.
	 
	 This program is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.
	 
	 You should have received a copy of the GNU General Public License
	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
	 
	 See \licenses\ for additional details on licenses associated with this program.
 
 **************************************************************************************
 
	 file:MacSupport.mm
	 
		Objective-C code.
 
 *************************************************************************************/

#include "Misc.h"
// i dont know why. i seriously dont.
// but all hell breaks loose if this is not here.
#define Point CarbonDummyPointName
#import <Foundation/NSString.h>
#import <AppKit/NSPanel.h>
#include <string.h>

/*********************************************************************************************
 
	Spawns a messagebox using NSRunAlertPanel
 
 *********************************************************************************************/

int MacBox(void * hwndParent, const char *text, const char *caption, int type)
{
	using namespace APE::Misc;
	int ret=0;
	
	NSString *tit=(NSString *)CFStringCreateWithCString(NULL,caption?caption:"",kCFStringEncodingASCII);
	NSString *text2=(NSString *)CFStringCreateWithCString(NULL,text?text:"",kCFStringEncodingASCII);
	
	if ((type & 0xF) == sYesNoCancel)
	{
		ret= NSRunAlertPanel(tit, @"%@", @"Yes", @"No", @"Cancel",text2);
		switch(ret)
		{
			case -1:
				ret = bCancel;
				break;
			case 0:
				ret = bNo;
				break;
			case 1:
				ret = bYes;
				break;
			case 2:
				ret = bCancel;
				break;
		};
	}
	else if ((type & 0xF) == sConTryCancel)
	{
		ret=NSRunAlertPanel(tit,@"%@",@"Continue",@"Try Again",@"Cancel",text2);
		switch(ret)
		{
			case -1:
				ret = bCancel;
				break;
			case 0:
				ret = bTryAgain;
				break;
			case 1:
				ret = bContinue;
				break;
			case 2:
				ret = bCancel;
				break;
		};
	}
	else if ((type & 0xF) == sOk)
	{
		NSRunAlertPanel(tit,@"%@",@"OK",@"",@"",text2);
		ret= bOk;
	}
	
	[text2 release];
	[tit release];
	
	return ret;
}
/*
 this could be anything apparantly
 */
@interface dummyObject : NSObject
- (void) dummyMethod;
@end

@implementation dummyObject
- (void) dummyMethod {
}
@end
/*********************************************************************************************
 
	Reurns the path of our bundle.
	Broken? If called, causes segv fault in another thread at objc_release after autoreleasepoolpage()
 
 *********************************************************************************************/
int GetBundlePath(char * buf, int bufSize)
{
	// bundleForClass retrieves a reference to the bundle that holds the definition
	// for dummyObject. We then call bundlePath on the bundle to retrieve the path of
	// the bundle.
	NSString * path = [[NSBundle bundleForClass:[[dummyObject alloc] class]]bundlePath];
	const char * intString = [path UTF8String];
	int length = strlen(intString);
	int smallestLength = length > bufSize ? bufSize : length;
	memcpy(buf, intString, smallestLength);
	[path release];
	return smallestLength;
}