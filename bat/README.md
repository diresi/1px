The 1px battery level widget.

A simple battery level widget for X11 showing a 1px bar at the bottom of the
screen that is scaled according to the current battery capacity.
Although ... I found that I like it more with some 5 to 10 px height that are
painted over my i3 status bar but YMMV.

It uses an X11 overlay window that is ideally translucent with the help of any
X11 compositor (e.g. https://cgit.freedesktop.org/xorg/app/xcompmgr/).

Here's a screenshot what this looks like on my laptop, pay attention to the yellowish-green bar at the very bottom 'cause this is it :)
![1pxbat on i3](https://github.com/diresi/1px/blob/main/bat/screenshots/1pxbat-on-i3.png?raw=true)
