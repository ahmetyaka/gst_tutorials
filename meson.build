project('screen-share-app', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17'])

gst_dep = dependency('gstreamer-1.0')
gst_video_dep = dependency('gstreamer-video-1.0')
gst_webrtc_dep = dependency('gstreamer-webrtc-1.0')

gio_dep = dependency('gio-2.0')
gio_unix_dep = dependency('gio-unix-2.0') # Required for DBus on Linux

src_files = ['main.cpp']

executable('screen_share',
  src_files,
  dependencies : [gst_dep, gst_video_dep, gst_webrtc_dep, gio_dep, gio_unix_dep],
  install : true)