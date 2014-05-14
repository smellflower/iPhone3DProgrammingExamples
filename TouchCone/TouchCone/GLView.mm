//
//  GLView.m
//  TouchCone
//
//  Created by zhangdl on 14/5/14.
//  Copyright (c) 2014 com.*. All rights reserved.
//

#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/gl.h>

#import "GLView.h"
#import "mach/mach_time.h"

const bool ForceES1 = false;

@implementation GLView

+ (Class)layerClass {
    
    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        
        CAEAGLLayer *eaglayer = (CAEAGLLayer *)super.layer;
        eaglayer.opaque = YES;
        
        EAGLRenderingAPI api = kEAGLRenderingAPIOpenGLES2;
        m_context = [[EAGLContext alloc] initWithAPI:api];
        
        if (!m_context || ForceES1) {
            api = kEAGLRenderingAPIOpenGLES1;
            m_context = [[EAGLContext alloc] initWithAPI:api];
        }
        
        if (!m_context || ![EAGLContext setCurrentContext:m_context]) {
            
            return nil;
        }
        
        if(api == kEAGLRenderingAPIOpenGLES2) {
            
            NSLog(@"Using OpenGL ES 2.0");
            m_renderingEngine = CreateRenderEngine2();
        } else {
            
            NSLog(@"Using OpenGL ES 1.1");
            m_renderingEngine = CreateRenderEngine1();
        }
        
        [m_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:eaglayer];
        
        m_renderingEngine->Initialize(CGRectGetWidth(frame), CGRectGetHeight(frame));
        
        [self drawView:nil];
        m_timestamp = CACurrentMediaTime();
        
        CADisplayLink *displayLink;
        displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawView:)];
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(didRotate:)
                                                     name:UIDeviceOrientationDidChangeNotification object:nil];
    }
    return self;
}

- (void)didRotate:(NSNotification *)notification {
    
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    m_renderingEngine->OnRotate((DeviceOrientation)orientation);
    [self drawView:nil];
}

- (void)drawView:(CADisplayLink *)displayLink {
    
    if (displayLink != nil) {
        
        float elapsedSeconds = displayLink.timestamp - m_timestamp;
        m_timestamp = displayLink.timestamp;
        m_renderingEngine->UpdateAnimation(elapsedSeconds);
    }
    
    m_renderingEngine->Render();
    [m_context presentRenderbuffer:GL_RENDERBUFFER];
    
}
