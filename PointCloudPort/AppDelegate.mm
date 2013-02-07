//
//  AppDelegate.m
//  PointCloudPort
//
//  Created by Michele Pratusevich on 2/7/13.
//  Copyright (c) 2013 Michele Pratusevich. All rights reserved.
//

#import "AppDelegate.h"

@implementation AppDelegate

@synthesize window;
@synthesize cameraViewController;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
	window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	cameraViewController = [[HardwareController alloc] initWithNibName:nil bundle:nil];
	[window setRootViewController:cameraViewController];
	[window makeKeyAndVisible];
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
}

- (void)applicationWillResignActive:(UIApplication *)application {
	[cameraViewController stopGraphics];
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	[cameraViewController startGraphics];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
}

- (void)dealloc {
	[cameraViewController release];
    [window release];
    [super dealloc];
}


@end

