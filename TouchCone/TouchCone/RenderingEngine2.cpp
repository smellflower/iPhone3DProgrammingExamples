//
//  RenderingEngine2.cpp
//  TouchCone
//
//  Created by zhangdl on 17/5/14.
//  Copyright (c) 2014 com.*. All rights reserved.
//

#include "IRenderingEngine.hpp"

#include <cmath>
#include <iostream>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <vector>
#include "Quaternion.hpp"
#include "IRenderingEngine.hpp"

#define STRINGIFY(A) #A
#include "../Shaders/Simple.vert"
#include "../Shaders/Simple.frag"

using namespace std;

static const float AnimationDuration = 0.25f;

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

class RenderingEngine2 : public IRenderingEngine {
    
public:
    RenderingEngine2();
    void Initialize(int width, int height);
    void Render() const;
    void UpdateAnimation(float timeStep);
    void OnRotate(DeviceOrientation newOrientation);
    void OnFingerUp(ivec2 location);
    void OnFingerDown(ivec2 location);
    void OnFingerMove(ivec2 oldLocation, ivec2 newLocation);
    
private:
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    GLuint BuildProgram(const char* vShader, const char* fShader) const;
    
    Animation m_animation;
    
    vector<Vertex> m_coneVertices;
    vector<GLubyte> m_coneIndices;

    GLfloat m_rotationAngle;
    GLfloat m_scale;
    
    ivec2 m_pivotPoint;
    
    GLuint m_bodyIndexCount;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    GLuint m_diskIndexCount;
    GLuint m_framebuffer;
    GLuint m_simpleProgram;
};

IRenderingEngine* CreateRenderEngine2() {
    
    return new RenderingEngine2();
}

RenderingEngine2::RenderingEngine2() : m_rotationAngle(0), m_scale(1) {
    
    //Create & bind the color buffer so that the caller can allocate its space.
    glGenRenderbuffers(1, &m_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
}

void RenderingEngine2::Initialize(int width, int height) {
    
    m_pivotPoint = ivec2(width / 2, height / 2);
    
    const float coneRadius = 0.5f;
    const float coneHeight = 1.866f;
    const int coneSlices = 40;
    const float dtheta = TwoPi / coneSlices;
    const int vertexCount = coneSlices * 2 + 1;

    m_coneVertices.resize(vertexCount);
    vector<Vertex>::iterator vertex = m_coneVertices.begin();
    
    // Cone body
    for (float theta = 0; vertex != m_coneVertices.end() - 1; theta += dtheta) {
        
        float brightness = abs(sin(theta));
        vec4 color(brightness, brightness, brightness, 1);
        
        vertex->Position = vec3(0, 1, 0);
        vertex->Color = color;
        vertex++;
        
        vertex->Position.x = coneRadius * cos(theta);
        vertex->Position.y = 1 - coneHeight;
        vertex->Position.z = coneRadius * sin(theta);
        vertex->Color = color;
        vertex++;
    }
    
    // Cone disk center
    vertex->Position = vec3(0, 1- coneHeight, 0);
    vertex->Color = vec4(1, 1, 1, 1);
    
    // Indices
    m_bodyIndexCount = coneSlices * 3;
    m_diskIndexCount = coneSlices * 3;
    
    m_coneIndices.resize(m_bodyIndexCount + m_diskIndexCount);
    vector<GLubyte>::iterator index = m_coneIndices.begin();
    
    // Body index
    for (int i = 0; i < coneSlices * 2; i += 2) {
        
        *index++ = i;
        *index++ = (i + 1) % (coneSlices * 2);
        *index++ = (i + 3) % (coneSlices * 2);
    }
    
    // Disk index
    const int diskCenterIndex = vertexCount - 1;
    for (int i = 1; i < coneSlices * 2 + 1; i += 2) {
        
        *index++ = diskCenterIndex;
        *index++ = i;
        *index++ = (i + 2) % (coneSlices * 2);
    }
    
    // Create depth buffer
    glGenRenderbuffers(1, &m_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH_COMPONENT16,
                          width,
                          height);
    
    // Create framebuffer object
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER,
                              m_colorRenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              m_depthRenderbuffer);
    
    //Bind renderbuffer from rendering
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    
    m_simpleProgram = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
    glUseProgram(m_simpleProgram);
    
    //Set the projection matrix
    GLint projectionUniform = glGetUniformLocation(m_simpleProgram, "Projection");
    
    mat4 projectionMatrix = mat4::Frustum(-1.6f, 1.6f, -2.4, 2.4, 5, 10);
    glUniformMatrix4fv(projectionUniform, 1, 0, projectionMatrix.Pointer());
}

void RenderingEngine2::Render() const {
    
    GLuint positionSlot = glGetAttribLocation(m_simpleProgram, "Position");
    GLuint colorSlot = glGetAttribLocation(m_simpleProgram, "SourceColor");
    
    mat4 rotation = mat4::Rotate(m_rotationAngle);
    mat4 scale = mat4::Scale(m_scale);
    mat4 translation = mat4::Translate(0, 0, -7);
    
    GLint modelviewUniform = glGetUniformLocation(m_simpleProgram, "ModelView");
    mat4 modelviewMatrix = scale * rotation * translation;
    
    GLsizei stride = sizeof(Vertex);
    const GLvoid* pCoords = &m_coneVertices[0].Position.x;
    const GLvoid* pColors = &m_coneVertices[0].Color.x;
    
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniformMatrix4fv(modelviewUniform, 1, 0, modelviewMatrix.Pointer());
    glVertexAttribPointer(positionSlot, 3, GL_FLOAT, GL_FALSE, stride, pCoords);
    glVertexAttribPointer(colorSlot, 4, GL_FLOAT, GL_FALSE, stride, pColors);
    glEnableVertexAttribArray(positionSlot);
    
    const GLvoid* bodyIndices = &m_coneIndices[0];
    const GLvoid* diskIndices = &m_coneIndices[m_bodyIndexCount];
    
    glEnableVertexAttribArray(colorSlot);
    glDrawElements(GL_TRIANGLES, m_bodyIndexCount, GL_UNSIGNED_BYTE, bodyIndices);
    glDisableVertexAttribArray(colorSlot);
    glVertexAttrib4f(1, 1, 1, 1, 1);
    glDrawElements(GL_TRIANGLES, m_diskIndexCount, GL_UNSIGNED_BYTE, diskIndices);
    
    glDisableVertexAttribArray(positionSlot);
}

void RenderingEngine2::OnFingerUp(ivec2 location) {

    m_scale = 1.0f;
}

void RenderingEngine2::OnFingerDown(ivec2 location) {

    m_scale = 1.5f;
}

void RenderingEngine2::OnFingerMove(ivec2 previous, ivec2 location) {

    vec2 direction = vec2(location - m_pivotPoint).Normalized();
    
    direction.y = -direction.y;

    m_rotationAngle = std::acos(direction.y) * 180.0f / 3.14159f;
    
    if (direction.x > 0) {
        m_rotationAngle = -m_rotationAngle;
    }
}

void RenderingEngine2::UpdateAnimation(float timeStep) {
    
}

void RenderingEngine2::OnRotate(DeviceOrientation newOrientation) {
    
}

GLuint RenderingEngine2::BuildShader(const char* source, GLenum shaderType) const {
    
    GLuint shaderHandle = glCreateShader(shaderType);
    glShaderSource(shaderHandle, 1, &source, 0);
    glCompileShader(shaderHandle);
    
    GLint compileSuccess;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
    
    if (compileSuccess == GL_FALSE) {
        
        GLchar messages[256];
        glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
        std::cout << messages;
        exit(1);
    }
    return shaderHandle;
}

GLuint RenderingEngine2::BuildProgram(const char* vertexShaderSource, const char* fragmentShaderSource) const {
    
    GLuint vertexShader = BuildShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = BuildShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vertexShader);
    glAttachShader(programHandle, fragmentShader);
    glLinkProgram(programHandle);
    
    GLint linkSuccess;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess == GL_FALSE) {
        
        GLchar messages[256];
        glGetProgramInfoLog(programHandle, sizeof(messages), 0, &messages[0]);
        std::cout << messages;
        exit(1);
    }
    return programHandle;
}