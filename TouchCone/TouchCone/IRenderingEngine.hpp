//
//  IRenderEngine.hpp
//  TouchCone
//
//  Created by zhangdl on 14/5/14.
//  Copyright (c) 2014 com.*. All rights reserved.
//

#ifndef HelloArrow_IRenderingEngine_hpp
#define HelloArrow_IRenderingEngine_hpp

#include "Vector.hpp"

enum DeviceOrientation {
    
    DeviceOrientationUnknown,
    DeviceOrientationPortrait,
    DeviceOrientationPortraitUpsideDown,
    DeviceOrientationLandscapeLeft,
    DeviceOrientationLandscapeRight,
    DeviceOrientationFaceUp,
    DeviceOrientationFaceDown,
};

struct IRenderingEngine* CreateRenderEngine1();
struct IRenderingEngine* CreateRenderEngine2();

// Interface to the OpenGL ES renderer; consumed by GLView.
struct IRenderingEngine {
    virtual void Initialize(int width, int height) = 0;
    virtual void Render() const = 0;
    virtual void UpdateAnimation(float timeStep) = 0;
    virtual void OnRotate(DeviceOrientation newOrientation) = 0;
    virtual void OnFingerUp(ivec2 location) = 0;
    virtual void OnFingerDown(ivec2 location) = 0;
    virtual void OnFingerMove(ivec2 oldLocation, ivec2 newLocation) = 0;
    virtual ~IRenderingEngine() {}
};

#endif
