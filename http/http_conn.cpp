#include "./http_conn.h"

int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

//设置文件描述符非阻塞
void setnonblocking(int fd) {
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd,F_SETFL, new_flag);
}

//添加文件描述符到epoll中
void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    //event.events = EPOLLIN | EPOLLRDHUP;
    event.events = EPOLLIN | EPOLLET |EPOLLRDHUP;  

    if(one_shot) {
        event.events = event.events | EPOLLONESHOT ;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

    //设置文件描述符非阻塞
    setnonblocking(fd);
}

//从epoll中删除文件描述符
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//从epoll中修改文件描述符, 重置epolloneshot事件，确保下次可读时，epollin事件能被触发
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

//初始化连接
void http_conn::init(int sockfd, const sockaddr_in & addr) {
    m_sockfd = sockfd;
    m_address = addr;

    //端口复用
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    //添加到epollfd对象中
    addfd(m_epollfd, m_sockfd, true);
    m_user_count++; //用户数+1

    init();
}

void http_conn::init() {
    m_checked_state = CHECK_STATE_REQUESTLINE; //初始状态解析请求首行
    m_checked_index = 0;
    m_start_line = 0;
    m_read_index = 0;
}

//关闭连接
void http_conn::close_conn() {
    if(m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }

}

//循环读取客户数据，直到无数据可读或对方关闭连接
bool http_conn::read(){      //非阻塞读
    
    if(m_read_index >= READ_BUFFER_SIZE) {
        return false;
    }

    // 读取到的字节
    int bytes_read = 0;
    while (true) {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_index, READ_BUFFER_SIZE - m_read_index, 0);
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                //没有数据
                break;
            }
            return false;
        } else if (bytes_read == 0) {
            //对方关闭连接
            return false;
        }
        m_read_index += bytes_read;
    }
    //printf("一次性读完数据\n");
    printf("读取到了数据：%s\n", m_read_buf);
    return true;
}   

//主状态机
http_conn::HTTP_CODE http_conn::process_read(){    //解析HTTP请求
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;

    char * text = 0;

    while(((m_checked_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) 
    || (line_status = parse_line()) == LINE_OK) {
        //解析到了一行完整的数据，或者解析到了请求体，也是完整的数据
        
        //获取一行数据
        text = get_line();

        m_start_line = m_checked_index;
        printf("got 1 http line: %s\n", text);

        switch (m_checked_state)
        {
        case CHECK_STATE_REQUESTLINE:{
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER: {
            ret = parse_headers(text);
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            } else if (ret == GET_REQUEST) {
                return do_request();
            }
        }
        case CHECK_STATE_CONTENT: {
            ret = parse_content(text);
            if (ret = GET_REQUEST) {
                return do_request();
            }
            line_status =LINE_OPEN;
            break;
        }

        default: {
            return INTERNAL_ERROR;
            //break;
        }
            
        }
        return NO_REQUEST;
    }

    return NO_REQUEST;
}   
http_conn::HTTP_CODE http_conn::parse_request_line(char * text){   //解析请求首行
    
}
http_conn::HTTP_CODE http_conn::parse_headers(char * text){    //解析请求头部

}  
http_conn::HTTP_CODE http_conn::parse_content(char * text){    //解析请求体

}  

http_conn::LINE_STATUS http_conn::parse_line(){    //读取一行
    return LINE_OK;
}

http_conn::HTTP_CODE http_conn::do_request(){    //读取一行
    return GET_REQUEST;
}



bool http_conn::write(){       //非阻塞写
    
    printf("一次性写完数据\n");

    return true;
}   

//由线程池中的工作线程调用，处理HTTP请求的入口函数
void http_conn::process(){
    //解析HTTP请求
    HTTP_CODE read_ret = process_read();
    if(read_ret == NO_REQUEST) {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
    }
    
    printf("parse request, create response\n");

    //生成相应

}

// http_conn::http_conn() {
    
// }