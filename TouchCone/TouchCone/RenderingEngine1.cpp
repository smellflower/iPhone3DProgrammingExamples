//
//  RenderingEngine1.cpp
//  TouchCone
//
//  Created by zhangdl on 14/5/14.
//  Copyright (c) 2014 com.*. All rights reserved.
//

#include <iostream>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <vector>
#include "IRenderingEngine.hpp"
#include "Quaternion.hpp"

using namespace std;

struct Vertex {
    
    vec3 Position;
    vec4 Color;
};

struct Animation {
    
    Quaternion Start;
    Quaternion End;
    Quaternion Current;
    float Elapsed;
    float Duration;
};

class RenderingEngine1 : public IRenderingEngine {
    
public:
    RenderingEngine1();
    void Initialize(int width, int height);
    void Render() const;
    void UpdateAnimation(float timeStep);
    void OnRotate(DeviceOrientation newOrientation);
    void OnFingerUp(ivec2 location);
    void OnFingerDown(ivec2 location);
    void OnFingerMove(ivec2 oldLocation, ivec2 newLocation);
private:
    Animation m_animation;
    
    vector<Vertex> m_cone;
    vector<Vertex> m_disk;
    
    GLfloat m_rotationAngle;
    GLfloat m_scale;
    
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    GLuint m_framebuffer;
    
    ivec2 m_pivotPoint;
};

IRenderingEngine* CreateRenderEngine1() {
    
    return new RenderingEngine1();
}

RenderingEngine1::RenderingEngine1() : m_rotationAngle(0), m_scale(1) {
    
    glGenRenderbuffersOES(1, &m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
}

void RenderingEngine1::Initialize(int width, int height) {
    
    m_pivotPoint = ivec2(width / 2, height / 2);
    
    const float coneRadius = 0.5f;
    const float coneHeight = 1.866f;
    const int coneSlices = 40;
    
    {
        //Generate vertices for the disk
        m_disk.resize(coneSlices + 2);
        
        //Initialize the center vertex of the triangle fan.
        vector<Vertex>::iterator vertex = m_disk.begin();
        vertex->Color = vec4(0.75, 0.75, 0.75, 1);
        vertex->Position.x = 0;
        vertex->Position.y = 1 - coneHeight;
        vertex->Position.z = 0;
        vertex++;
        
        //Initialize the rim vertices of the triangle fan.
        const float dtheta = TwoPi / coneSlices;
        for (float theta = 0; vertex != m_disk.end(); theta += dtheta) {
            
            vertex->Color = vec4(0.75, 0.75, 0.75, 1);
            vertex->Position.x = coneRadius * cos(theta);
            vertex->Position.y = 1 - coneHeight;
            vertex->Position.z = coneRadius * sin(theta);
            vertex++;
        }
        
    }
    {
        //Generate vertices for the body of the cone
        m_cone.resize((coneSlices + 1) * 2);
        
        //Initialize the vertices of the triangle strip.
        vector<Vertex>::iterator vertex = m_cone.begin();
        const float dtheta = TwoPi / coneSlices;
        for (float theta = 0; vertex != m_cone.end(); theta += dtheta) {
            
            //Grayscale gradient
            float brightness = abs(sin(theta));
            vec4 color(brightness, brightness, brightness, 1);
            
            //Apex vertex
            vertex->Position = vec3(0, 1, 0);
            vertex->Color = color;
            vertex++;
            
            //Rim vertex
            vertex->Position.x = coneRadius * cos(theta);
            vertex->Position.y = 1 - coneHeight;
            vertex->Position.z = coneRadius * sin(theta);
            vertex->Color = color;
            vertex++;
        }
    }
    
    //Create the depth buffer
    glGenRenderbuffersOES(1, &m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES,
                             GL_DEPTH_COMPONENT16_OES,
                             width,
                             height);
    
    //Create the framebuffer object; attatch the depth and color buffers
    glGenFramebuffersOES(1, &m_framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES,
                                 GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES,
                                 m_colorRenderbuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES,
                                 GL_DEPTH_ATTACHMENT_OES,
                                 GL_RENDERBUFFER_OES,
                                 m_depthRenderbuffer);
    
    //Bind the colorbuffer from rendering
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glFrustumf(-1.6f, 1.6f, -2.4f, 2.4f, 5, 10);
    
    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0, 0, -7);
}

void RenderingEngine1::Render() const {
    
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glRotatef(m_rotationAngle, 0, 0, 1);
    glScalef(m_scale, m_scale, m_scale);
    
    //Draw the cone.
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &m_cone[0].Position.x);
    glColorPointer(4, GL_FLOAT, sizeof(Vertex), &m_cone[0].Color.x);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, m_cone.size());
    
    //Draw the disk that caps off the base of the cone.
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &m_disk[0].Position.x);
    glColorPointer(4, GL_FLOAT, sizeof(Vertex), &m_disk[0].Color.x);
    glDrawArrays(GL_TRIANGLE_FAN, 0, m_disk.size());
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    glPopMatrix();
}

void RenderingEngine1::OnFingerUp(ivec2 location) {

    m_scale = 1.0f;
}

void RenderingEngine1::OnFingerDown(ivec2 location) {

    m_scale = 1.5f;
    OnFingerMove(location, location);
}

void RenderingEngine1::OnFingerMove(ivec2 previous, ivec2 location) {

    vec2 direction = vec2(location - m_pivotPoint).Normalized();
    
    //Flip the y-axis because pixel coords increase toward the bottom.
    direction.y = -direction.y;
    
    m_rotationAngle = std::acos(direction.y) * 180.0f / 3.14159f;
    if (direction.x > 0) {
        m_rotationAngle = -m_rotationAngle;
    }
}

void RenderingEngine1::OnRotate(DeviceOrientation orientation) {
    
    
}

void RenderingEngine1::UpdateAnimation(float timeStep) {
    
    
}

