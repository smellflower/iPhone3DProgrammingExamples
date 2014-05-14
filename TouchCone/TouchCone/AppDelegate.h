//
//  AppDelegate.h
//  TouchCone
//
//  Created by zhangdl on 14/5/14.
//  Copyright (c) 2014 com.*. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "GLView.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate> {

    GLView *m_view;
}

@property (strong, nonatomic) UIWindow *m_window;

@end
