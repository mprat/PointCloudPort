//
//  AppDelegate.h
//  PointCloudPort
//
//  Created by Michele Pratusevich on 2/7/13.
//  Copyright (c) 2013 Michele Pratusevich. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "HardwareController.h"

@class HardwareController;

@interface AppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
	HardwareController *cameraViewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) HardwareController *cameraViewController;

@end