# DeferredLightingOnAndroid

A demo implementing deferred lighting running on Android devices. I first published this demo on my blog over here: http://wojtsterna.blogspot.com/2015/09/deferred-lighting-on-adroid.html.

![img](https://user-images.githubusercontent.com/37375338/215497209-258dc943-742a-42e9-ad2e-077f591213b5.png)

The demo uses OpenGL ES 2.0. This API allows to barely render into one render target at a time and as far as I know does not provide floating point formats. How do you generate the g-buffer with that limitation? Well, you could go with a few passes but yeah, that is insane in terms of performance for Android devices. That's why I settled with one pass and fit everything into a single RGBA8 buffer. All I need for basic deferred lighting is position and normals. Instead of postition we store (linear) depth using 16 bits of the buffer (position is reconstructed later in the lighting pass). In case of normals I store x and y components in view-space and reconstruct z in the lighting pass, hence only two (8-bit) channels suffice. All fits into a 32-bit buffer. Please examine shaders code to see how it all works.

The demo can use two common optimizations for the lights in the lighting pass. One is using scissor test and the other using stencil buffer. Both can be turned on/off in code using macros `USE_SCISSOR` and `USE_STENCIL`.

The demo renders 18 small-to-medium lights on Tegra 3 with 21 FPS at 850x480 pixels, which I consider not bad.

# Installation
1. Import into Eclipse as Android project.
2. Open cmd, jump into jni directory and call ndk-build (which must first be installed, obviously).
3. From Eclipse run the app.
