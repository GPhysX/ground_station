ground_station
===

ARTSAT ground station software for amateur spacecrafts.<br />
Programmed with C++ language and posix APIs.<br />
This is a daemon software which runs as background task.<br />
User interface is provided as a web interface.<br />
You can access and control this software with Safari or with another web browser.<br />
<br />
Development environment:<br />
o MacOS X 10.10.1<br />
o Xcode 6.1<br />
o boost 1.56.0<br />
o cpp-netlib 0.11.1RC2<br />
<br />
First you have to compile and to install boost and cpp-netlib libraries.<br />
Here are some patch files to do that:<br />
http://github.com/toolbits/boost_1_56_0_xcode610_universal<br />
http://github.com/toolbits/cpp-netlib-0.11.1RC2_xcode610_universal<br />
<br />
Then open artsatd/artsatd.xcodeproj and do compile it.<br />
Access http://localhost:16780 with Safari.<br />
<br />
Have fun!<br />
<br />
Screen shots:<br />
http://github.com/ARTSAT/ground_station/blob/master/artsatd/%7Escreenshot/invader_cw.png<br />
http://github.com/ARTSAT/ground_station/blob/master/artsatd/%7Escreenshot/invader_fm.png<br />
http://github.com/ARTSAT/ground_station/blob/master/artsatd/%7Escreenshot/prism_fm.png<br />
http://github.com/ARTSAT/ground_station/blob/master/artsatd/%7Escreenshot/webcam.png<br />
