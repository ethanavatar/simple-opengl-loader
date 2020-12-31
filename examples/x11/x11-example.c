/*********************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2020 Tarek Sherif
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**********************************************************************************/

#define SOGL_MAJOR_VERSION 4
#define SOGL_MINOR_VERSION 6
#define SOGL_IMPLEMENTATION
#include "../../simple-opengl-loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

typedef GLXContext (*glXCreateContextAttribsARBFUNC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

int main(int argc, char const *argv[]) {
    Display* display;
    Window window;
    XEvent event;
    XWindowAttributes xWinAtt;

    // X Windows stuff
    display = XOpenDisplay(NULL);

    if (display == NULL) {
        printf("Unable to connect to X Server\n");
        return 1;
    }

    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 20, 20, 2000, 2000, 0, 0, 0);
    

    XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask);
    XStoreName(display, window, "Simple OpenGL Loader X11 Example");
    XMapWindow(display, window);
    
    int numFBC = 0;
    GLint visualAtt[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT, 
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, 
        GLX_DOUBLEBUFFER, True, 
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DEPTH_SIZE, 1,
        GLX_STENCIL_SIZE, 1,
        None
    };

    GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display), visualAtt, &numFBC);

    if (!fbc) {
        fprintf(stderr, "Unable to get framebuffer\n");
        return -1;        
    }

    glXCreateContextAttribsARBFUNC glXCreateContextAttribsARB = (glXCreateContextAttribsARBFUNC) glXGetProcAddress((const GLubyte *) "glXCreateContextAttribsARB");

    if (!glXCreateContextAttribsARB) {
        fprintf(stderr, "Unable to get proc glXCreateContextAttribsARB\n");
        XFree(fbc);
        return -1; 
    }

    static int contextAttribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 5,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };

    GLXContext ctx = glXCreateContextAttribsARB(display, *fbc, NULL, True, contextAttribs);

    XFree(fbc);

    if (!ctx) {
        fprintf(stderr, "Unable to create OpenGL context\n");
        return -1;
    }

    glXMakeCurrent(display, window, ctx);

    if (!sogl_loadOpenGL()) {
        const char **failures = sogl_getFailures();
        while (*failures) {
            fprintf(stderr, "SOGL X11 EXAMPLE: Failed to load function %s\n", *failures);
            failures++;
        }
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);

    GLuint vertexArray = 0;
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    GLfloat positionData[] = {
        -0.5, -0.5,
        0.5, -0.5,
        0.0, 0.5
    };

    GLuint positions = 0;
    glGenBuffers(1, &positions);
    glBindBuffer(GL_ARRAY_BUFFER, positions);
    glBufferData(GL_ARRAY_BUFFER, 3 * 2 * sizeof(GLfloat), positionData, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    GLubyte colorData[] = {
        255, 0, 0,
        0, 255, 0,
        0, 0, 255
    };

    GLuint colors = 0;
    glGenBuffers(1, &colors);
    glBindBuffer(GL_ARRAY_BUFFER, colors);
    glBufferData(GL_ARRAY_BUFFER, 3 * 3 * sizeof(GLubyte), colorData, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, 0, NULL);
    glEnableVertexAttribArray(1);

    const char* vsSource =
    "#version 450\n"
    "layout(location=0) in vec4 position;\n"
    "layout(location=1) in vec3 color;\n"
    "out vec3 vColor;\n"
    "void main() {\n"
    "    vColor = color;\n"
    "    gl_Position = position;\n"
    "}\n";

    const char* fsSource =
    "#version 450\n"
    "in vec3 vColor;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = vec4(vColor, 1.0);\n"
    "}\n";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSource, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, NULL);
    glCompileShader(fs);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int params = -1;
    glGetProgramiv(program, GL_LINK_STATUS, &params);

    if (params != GL_TRUE) {
        fprintf(stderr, "Program did not link!\n");
    }

    glUseProgram(program);

    // Animation loop
    while (1) {
        if (XCheckWindowEvent(display, window, ExposureMask | KeyPressMask, &event) == True) {

            if (event.type == Expose) {
                XGetWindowAttributes(display, window, &xWinAtt);
                glViewport(0, 0, xWinAtt.width, xWinAtt.height);
            }

            if (event.type == KeyPress) {
                KeySym key = XLookupKeysym(&event.xkey, 0);
                
                if (key == XK_q) {
                    break;
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glXSwapBuffers(display, window);
    };

    // Teardown
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}
