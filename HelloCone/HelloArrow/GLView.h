//
//  GLView.h
//  HelloArrow
//
//  Created by zhangdl on 7/5/14.
//  Copyright (c) 2014 com.*. All rights reserved.
//

#import <OpenGLES/EAGL.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#import "IRenderingEngine.hpp"

@interface GLView : UIView {

@private
    EAGLContext *m_context;
    IRenderingEngine *m_renderingEngine;
    float m_timestamp;
}

- (void)drawView:(CADisplayLink *)displayLink;
- (void)didRotate:(NSNotification *)notification;

@end
