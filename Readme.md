### Window Closing Effect

Well, this is just a simple try to add a "window closing effect" to Windows system.

As this is still in the stage of "trying-out", no configuration file is taken. The program is simply using the (ported) "Broken Glass" effect in the famous "Burn My Windows".

To use another effect, you should edit the shaders in the source files, and change the glfw window size and position. And sometimes you may need to change the glViewPort according to the window size and pos.

It still fail to support many frequently-used programs -- some of these programs close their windows before our hooks get notified; and some of them even did not trigger the hook.

From my point of view, there seems no simple way to solve all these problems, and I quickly get exhausted diving into different "undocumented" behaviors and different way of closing the window.

This program is a little slow to react: in worst cases, up to about half a second on my laptop. Usually it is much quicker to respond, but still enough to notice a short delay. I thought that was related to the glfw window creation, but I did not profile that so I'm not sure.

After a week struggling with all these stuff, finally I decided to add some error handling (to prevent it from crashing, at least on circumstances I know it would fail) and let it be.



#### Build requirements

C++/WinRT (for window capturing),which can be installed as a nuget package

boost-scope-exit,glew,glfw3,opengl-registry, which can be installed from vcpkg



#### Run

Simply run the generate exe.

At least it works well with explorer, notepad, visual studio window and etc. (But not with calculator, msedge and etc.)



#### License

GPL v3