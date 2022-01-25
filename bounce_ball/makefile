bouncer: 
	# compile bouncer.cpp
	g++ bouncer.cpp $(shell pkg-config --cflags --libs libavformat) $(shell pkg-config --cflags --libs libavutil) $(shell pkg-config --cflags --libs libavcodec) $(shell pkg-config --cflags --libs libswscale) -o bouncer


movie: 
	# assemble individual image files into movie file 
	ffmpeg -i ball%03d.cool -vf fps=30 -pix_fmt yuv420p animation.mp4