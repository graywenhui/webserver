a : main.cpp ./http/http_conn.cpp
	g++ main.cpp ./http/http_conn.cpp -pthread

clean :
	rm -f a