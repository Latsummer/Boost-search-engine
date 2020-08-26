FLAG=-std=c++11 -I ~/third_part/include -l pthread

http_server:http_server.cpp libsearcher.so
			g++ $^ -o $@ $(FLAG)

.PHONY:clean
clean:
		rm http_server